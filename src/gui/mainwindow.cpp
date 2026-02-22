#include "mainwindow.h"
#include "appglobal.h"
#include "configparams.h"
#include "dispatcher.h"
#include "soundbase.h"
#include "soundwindows.h"
#include "sstv/sstvrx.h"
#include "sstv/sstvtx.h"
#include "textoverlaydialog.h" // Added header
#include "txworker.h"
#include "utils/rigcontrol.h"
#include "waterfallwidget.h"
#include "wavio.h" // Added for recording
#include <QApplication>
#include <QDateTime> // Added
#include <QDebug>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QImage>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPixmap>
#include <QProgressDialog> // Added
#include <QSettings>
#include <QStandardPaths> // Added
#include <QThread>
#include <hamlib/rig.h>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  qDebug() << "MainWindow::MainWindow start";

  // Initialize pointers to NULL to prevent crashes if accessed before setup
  rxModeCombo = nullptr;
  txModeCombo = nullptr;
  bandCombo = nullptr;
  freqSpin = nullptr;
  ptrRx = nullptr;
  ptrTx = nullptr;

  setupUi();
  qDebug() << "setupUi finished.";

  loadSettings();
  qDebug() << "loadSettings finished. LastImportDir:" << lastImportDir;
}

MainWindow::~MainWindow() {
  saveSettings();
  delete ptrTx;
}

void MainWindow::setupUi() {
  // Central Widget
  QWidget *centralWidget = new QWidget(this);
  setCentralWidget(centralWidget);

  // Main Layout
  QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

  ptrRx = new sstvRx();
  ptrRx->init();

  ptrTx = new sstvTx();
  ptrTx->init();

  // --- CAT Top Bar ---
  QHBoxLayout *catBar = new QHBoxLayout();
  QLabel *bandLabel = new QLabel(tr("Band:"));
  bandCombo = new QComboBox();
  bandCombo->addItem(tr("Manual"), 0.0);
  bandCombo->addItem(tr("80m (3.845 MHz)"), 3.845);
  bandCombo->addItem(tr("40m (7.171 MHz)"), 7.171);
  bandCombo->addItem(tr("20m (14.230 MHz)"), 14.230);
  bandCombo->addItem(tr("15m (21.340 MHz)"), 21.340);
  bandCombo->addItem(tr("10m (28.680 MHz)"), 28.680);

  QLabel *freqLabel = new QLabel(tr("Frequency (MHz):"));
  freqSpin = new QDoubleSpinBox();
  freqSpin->setRange(0.0, 3000.0);
  freqSpin->setDecimals(6);
  freqSpin->setSingleStep(0.001);
  freqSpin->setSuffix(" MHz");

  catBar->addWidget(bandLabel);
  catBar->addWidget(bandCombo);
  catBar->addWidget(freqLabel);
  catBar->addWidget(freqSpin);
  catBar->addStretch();
  mainLayout->addLayout(catBar);

  rigPollTimer = new QTimer(this);
  connect(rigPollTimer, &QTimer::timeout, this, &MainWindow::rigPollSlot);
  rigPollTimer->start(1000); // Poll every second

  connect(freqSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
          &MainWindow::onFreqUIChanged);
  connect(bandCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &MainWindow::onBandUIChanged);

  if (dispatcherPtr) {
    connect(dispatcherPtr, &dispatcher::rxStatus, this,
            &MainWindow::onRxStatus);
    connect(dispatcherPtr, &dispatcher::rxSync, this, &MainWindow::onRxSync);
    connect(dispatcherPtr, &dispatcher::rxLine, this,
            &MainWindow::rxImageUpdateSlot);
    connect(dispatcherPtr, &dispatcher::rxEnd, this, &MainWindow::onRxEnd);
  }

  rxTimer = new QTimer(this);
  connect(rxTimer, &QTimer::timeout, this, &MainWindow::rxTimerSlot);
  rxTimer->start(20);

  txProgressTimer = new QTimer(this);
  connect(txProgressTimer, &QTimer::timeout, this,
          &MainWindow::updateTxProgress);

  // Live RX image preview — repaints label at 200 ms
  rxImageUpdateTimer = new QTimer(this);
  connect(rxImageUpdateTimer, &QTimer::timeout, this,
          &MainWindow::rxImageUpdateSlot);
  rxImageUpdateTimer->start(200);

  // Tab Widget
  tabWidget = new QTabWidget(this);
  mainLayout->addWidget(tabWidget);

  // Setup Tabs
  setupReceiveTab();
  setupTransmitTab();
  setupSettingsTab();

  // Add Tabs
  tabWidget->addTab(receiveTab, tr("Receive"));
  tabWidget->addTab(transmitTab, tr("Transmit"));
  tabWidget->addTab(settingsTab, tr("Settings"));

  setWindowTitle(tr("WinSSTV"));
  resize(800, 600);
}

void MainWindow::setupReceiveTab() {
  receiveTab = new QWidget();
  QVBoxLayout *rxLayout = new QVBoxLayout(receiveTab);

  // --- Waterfall ---
  rxWaterfall = new WaterfallWidget(receiveTab);
  rxWaterfall->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  rxWaterfall->setMinimumHeight(150);

  // --- Status label (mode name / sync quality) ---
  rxStatusLabel = new QLabel(tr("Hunting..."), receiveTab);
  rxStatusLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  rxStatusLabel->setStyleSheet("font-weight: bold; padding: 2px;");
  rxStatusLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

  // --- Received image ---
  rxImageLabel = new QLabel(tr("No image received yet"), receiveTab);
  rxImageLabel->setAlignment(Qt::AlignCenter);
  rxImageLabel->setStyleSheet("background-color: #1a1a1a; color: #888;");
  rxImageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  rxImageLabel->setMinimumHeight(220);

  // --- Save button ---
  saveImageButton = new QPushButton(tr("Save Received Image"), receiveTab);
  saveImageButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  connect(saveImageButton, &QPushButton::clicked, this,
          &MainWindow::onSaveImage);

  rxLayout->addWidget(rxWaterfall, 2);   // waterfall gets 2 shares
  rxLayout->addWidget(rxStatusLabel, 0); // status bar: fixed
  rxLayout->addWidget(rxImageLabel, 3);  // image gets 3 shares
  rxLayout->addWidget(saveImageButton, 0);

  if (soundIOPtr) {
    connect(soundIOPtr, &soundBase::processAudio, rxWaterfall,
            &WaterfallWidget::processAudio);
  }

  // --- Audio Recording & Deep Decode ---
  QHBoxLayout *toolsLayout = new QHBoxLayout();

  recordButton = new QPushButton(tr("Record Audio"), receiveTab);
  recordButton->setCheckable(true);
  connect(recordButton, &QPushButton::clicked, this,
          &MainWindow::onToggleRecording);

  deepDecodeButton = new QPushButton(tr("Deep Decode..."), receiveTab);
  connect(deepDecodeButton, &QPushButton::clicked, this,
          &MainWindow::onDeepDecode);

  denoiseCheckBox = new QCheckBox(tr("Denoise"), receiveTab);
  denoiseCheckBox->setToolTip(tr("Apply 3x3 Median Filter to remove noise"));

  rxModeCombo = new QComboBox(receiveTab);
  rxModeCombo->setToolTip(
      tr("Select specific mode to skip VIS. Auto detects any."));
  rxModeCombo->addItem(tr("Auto"), QVariant::fromValue((int)NOTVALID));
  for (int i = STARTWIDE; i <= ENDWIDE; i++) {
    rxModeCombo->addItem(getSSTVModeNameLong((esstvMode)i),
                         QVariant::fromValue(i));
  }
  rxModeCombo->setCurrentIndex(
      rxModeCombo->findData(QVariant::fromValue((int)sstvModeIndexRx)));
  connect(rxModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &MainWindow::onRxModeComboChanged);

  toolsLayout->addWidget(new QLabel(tr("Mode:"), receiveTab));
  toolsLayout->addWidget(rxModeCombo);
  toolsLayout->addWidget(recordButton);
  toolsLayout->addWidget(deepDecodeButton);
  toolsLayout->addWidget(denoiseCheckBox);
  rxLayout->addLayout(toolsLayout);

  // Init recorder
  audioRecorder = new wavIO(12000); // Record at 12kHz (filtered rate)
  isRecording = false;
}

void MainWindow::setupTransmitTab() {
  transmitTab = new QWidget();
  QVBoxLayout *txLayout = new QVBoxLayout(transmitTab);

  importImageButton = new QPushButton(tr("Import Image"), transmitTab);
  addTextButton = new QPushButton(tr("Add Text"), transmitTab);
  transmitButton = new QPushButton(tr("Transmit"), transmitTab);
  stopTxButton = new QPushButton(tr("Stop"), transmitTab);
  stopTxButton->setEnabled(false); // Disabled by default
  generateWavButton = new QPushButton(tr("Produce WAV File..."), transmitTab);
  generateWavButton->setToolTip(
      tr("Generate an SSTV audio file from the current image."));

  txOverlayWidget = new OverlayWidget(transmitTab);
  txOverlayWidget->setStyleSheet(
      "background-color: #f0f0f0; min-height: 300px;");

  txProgressBar = new QProgressBar(transmitTab);
  txProgressBar->setRange(0, 100);
  txProgressBar->setValue(0);

  txModeCombo = new QComboBox(transmitTab);
  // Dynamically populate SSTV modes from table
  for (int i = 0; i < NUMSSTVMODES; ++i) {
    QString name = getSSTVModeNameLong((esstvMode)i);
    if (!name.isEmpty()) {
      txModeCombo->addItem(name, i);
    }
  }
  // Default to Scottie 1 if found
  int s1Index = txModeCombo->findData(S1);
  if (s1Index != -1)
    txModeCombo->setCurrentIndex(s1Index);

  QHBoxLayout *prepLayout = new QHBoxLayout();
  prepLayout->addWidget(new QLabel(tr("Mode:"), transmitTab));
  prepLayout->addWidget(txModeCombo);
  prepLayout->addWidget(importImageButton);
  prepLayout->addWidget(addTextButton);
  prepLayout->addStretch();

  QHBoxLayout *actionLayout = new QHBoxLayout();
  actionLayout->addWidget(transmitButton);
  actionLayout->addWidget(stopTxButton);
  actionLayout->addWidget(generateWavButton);
  actionLayout->addStretch();

  txLayout->addWidget(txOverlayWidget);
  txLayout->addWidget(txProgressBar);
  txLayout->addLayout(prepLayout);
  txLayout->addLayout(actionLayout);

  connect(importImageButton, &QPushButton::clicked, this,
          &MainWindow::onImportImage);
  connect(addTextButton, &QPushButton::clicked, this, &MainWindow::onAddText);
  connect(transmitButton, &QPushButton::clicked, this, &MainWindow::onTransmit);
  connect(stopTxButton, &QPushButton::clicked, this, &MainWindow::onStopTx);
  connect(generateWavButton, &QPushButton::clicked, this,
          &MainWindow::onGenerateTestWav);
  connect(txModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &MainWindow::onModeChanged);
}

void MainWindow::onImportImage() {
  QString initialDir =
      lastImportDir.isEmpty() ? QDir::homePath() : lastImportDir;
  QString fileName =
      QFileDialog::getOpenFileName(this, tr("Open Image"), initialDir,
                                   tr("Images (*.png *.jpg *.jpeg *.bmp)"));
  if (!fileName.isEmpty()) {
    lastImportDir = QFileInfo(fileName).absolutePath();
    saveSettings(); // Persist directory immediately
    originalTxImage = QImage(fileName);
    if (!originalTxImage.isNull()) {
      onModeChanged(txModeCombo->currentIndex());
      txOverlayWidget->clearOverlays();
    }
  }
}

void MainWindow::onAddText() {
  qDebug() << "onAddText called. lastOverlayText:" << lastOverlayText;
  TextOverlayDialog dlg(this, lastOverlayText, lastOverlayColor,
                        lastOverlaySize);
  if (dlg.exec() == QDialog::Accepted) {
    lastOverlayText = dlg.getText();
    lastOverlayColor = dlg.getColor();
    lastOverlaySize = dlg.getFontSize();

    saveSettings(); // Persist immediately

    // Scale position? For now center or strict position
    // Let's just put it at 50,50 for now as requested, or maybe center?
    // OverlayWidget handles drawing.
    txOverlayWidget->addTextOverlay(lastOverlayText, QPoint(10, 50),
                                    lastOverlayColor, lastOverlaySize);
  }
}

void MainWindow::onSaveImage() {
  if (ptrRx) {
    QImage img = ptrRx->getLastReceivedImage();
    if (!img.isNull()) {
      QString initialPath =
          lastSaveDir.isEmpty() ? QDir::homePath() : lastSaveDir;
      // Suggest a filename with timestamp?
      // initialPath += "/SSTV_" +
      // QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss") + ".png";

      QString fileName = QFileDialog::getSaveFileName(
          this, tr("Save Image"), initialPath,
          tr("Images (*.png *.jpg);;All Files (*)"));
      if (!fileName.isEmpty()) {
        img.save(fileName);
        lastSaveDir = QFileInfo(fileName).absolutePath();
        saveSettings();
      }
    }
  }
}

void MainWindow::onStopTx() {
  if (ptrTx) {
    ptrTx->abort();
  }
  // Force audio stop immediately
  if (soundIOPtr) {
    soundIOPtr->stopImmediate(); // Kill audio without waiting
  }
  txProgressBar->setValue(0);
  txProgressTimer->stop();
  onTxFinished(); // Reset UI
}

void MainWindow::setupSettingsTab() {
  settingsTab = new QWidget();
  QVBoxLayout *settingsTabLayout = new QVBoxLayout(settingsTab);

  QScrollArea *mainScrollArea = new QScrollArea(settingsTab);
  mainScrollArea->setWidgetResizable(true);
  mainScrollArea->setFrameShape(QFrame::NoFrame);

  QWidget *scrollContentWidget = new QWidget();
  QVBoxLayout *settingsLayout = new QVBoxLayout(scrollContentWidget);
  QFormLayout *formLayout = new QFormLayout();

  // Callsign
  callsignEdit = new QLineEdit(myCallsign, settingsTab);
  formLayout->addRow(tr("My Callsign:"), callsignEdit);

  // RX Input
  rxInputCombo = new QComboBox(settingsTab);
  rxInputCombo->addItem(tr("Soundcard"), soundBase::SNDINCARD);
  rxInputCombo->addItem(tr("File"), soundBase::SNDINFROMFILE);
  rxInputCombo->setCurrentIndex(rxInputCombo->findData(soundRoutingInput));
  formLayout->addRow(tr("RX Audio Input Source:"), rxInputCombo);

  rxDeviceCombo = new QComboBox(settingsTab);
  for (const auto &device : soundWindows::availableInputDevices()) {
    rxDeviceCombo->addItem(device.deviceName());
  }
  rxDeviceCombo->setCurrentText(inputDeviceName);
  formLayout->addRow(tr("RX Audio Device:"), rxDeviceCombo);

  // TX Output
  txOutputCombo = new QComboBox(settingsTab);
  txOutputCombo->addItem(tr("Soundcard"), soundBase::SNDOUTCARD);
  txOutputCombo->addItem(tr("File"), soundBase::SNDOUTTOFILE);
  txOutputCombo->setCurrentIndex(txOutputCombo->findData(soundRoutingOutput));
  formLayout->addRow(tr("TX Audio Output Source:"), txOutputCombo);

  txDeviceCombo = new QComboBox(settingsTab);
  for (const auto &device : soundWindows::availableOutputDevices()) {
    txDeviceCombo->addItem(device.deviceName());
  }
  txDeviceCombo->setCurrentText(outputDeviceName);
  formLayout->addRow(tr("TX Audio Device:"), txDeviceCombo);

  // Clocks
  rxClockSpin = new QDoubleSpinBox(settingsTab);
  rxClockSpin->setRange(1000.0, 192000.0);
  rxClockSpin->setValue(rxClock);
  formLayout->addRow(tr("RX Clock (Hz):"), rxClockSpin);

  txClockSpin = new QDoubleSpinBox(settingsTab);
  txClockSpin->setRange(1000.0, 192000.0);
  txClockSpin->setValue(txClock);
  formLayout->addRow(tr("TX Clock (Hz):"), txClockSpin);

  // CW ID
  useCWCheck = new QCheckBox(tr("Enable CW ID at end"), settingsTab);
  useCWCheck->setChecked(useCW);
  formLayout->addRow(useCWCheck);

  cwTextEdit = new QLineEdit(cwText, settingsTab);
  formLayout->addRow(tr("CW Text:"), cwTextEdit);

  // Mode Management
  formLayout->addRow(
      new QLabel("<b>" + tr("SSTV Modes Enabled (Toggle)") + "</b>"));
  QScrollArea *modeScrollArea = new QScrollArea(settingsTab);
  QWidget *modeScrollContent = new QWidget();
  QVBoxLayout *modeScrollLayout = new QVBoxLayout(modeScrollContent);

  modeCheckBoxes.clear();
  for (int i = 0; i < NUMSSTVMODES; ++i) {
    QCheckBox *cb =
        new QCheckBox(getSSTVModeNameLong((esstvMode)i), modeScrollContent);
    cb->setChecked(SSTVTable[i].enabled);
    connect(cb, &QCheckBox::toggled, this, [this, i](bool checked) {
      SSTVTable[i].enabled = checked;
      refreshModeCombos();
      saveSettings(); // Save mode change without restarting sound/rig
    });
    modeScrollLayout->addWidget(cb);
    modeCheckBoxes.append(cb);
  }
  modeScrollArea->setWidget(modeScrollContent);
  modeScrollArea->setWidgetResizable(true);
  modeScrollArea->setMinimumHeight(200);
  formLayout->addRow(modeScrollArea);

  // CAT Control Group
  formLayout->addRow(
      new QLabel("<b>" + tr("CAT Control (via Hamlib)") + "</b>"));

  catEnabledCheck = new QCheckBox(tr("Enable CAT Control"), settingsTab);
  catEnabledCheck->setChecked(catEnabled);
  formLayout->addRow(catEnabledCheck);

  catRigCombo = new QComboBox(settingsTab);
  populateRigModels();
  catRigCombo->setCurrentIndex(catRigCombo->findData(catRigModel));
  formLayout->addRow(tr("Rig Model:"), catRigCombo);

  catPortCombo = new QComboBox(settingsTab);
  catPortCombo->setEditable(true); // Allow manual entry if needed
  catPortCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

  refreshPortsButton = new QPushButton(tr("Refresh"), settingsTab);
  connect(refreshPortsButton, &QPushButton::clicked, this,
          &MainWindow::onRefreshSerialPorts);

  QHBoxLayout *portLayout = new QHBoxLayout();
  portLayout->addWidget(catPortCombo);
  portLayout->addWidget(refreshPortsButton);

  onRefreshSerialPorts(); // Populate initially
  catPortCombo->setCurrentText(catSerialPort);

  formLayout->addRow(tr("Serial Port:"), portLayout);

  catBaudCombo = new QComboBox(settingsTab);
  const int bauds[] = {1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200};
  for (int b : bauds)
    catBaudCombo->addItem(QString::number(b), b);
  catBaudCombo->setCurrentIndex(catBaudCombo->findData(catBaudRate));
  formLayout->addRow(tr("Baud Rate:"), catBaudCombo);

  catPTTCombo = new QComboBox(settingsTab);
  catPTTCombo->addItem(tr("CAT"), RIG_PTT_RIG);
  catPTTCombo->addItem(tr("DTR"), RIG_PTT_SERIAL_DTR);
  catPTTCombo->addItem(tr("RTS"), RIG_PTT_SERIAL_RTS);
  catPTTCombo->addItem(tr("NONE"), RIG_PTT_NONE);
  catPTTCombo->setCurrentIndex(catPTTCombo->findData(catPTTMethod));
  formLayout->addRow(tr("PTT Method:"), catPTTCombo);

  pttTestButton = new QPushButton(tr("Test PTT (Toggle)"), settingsTab);
  pttTestButton->setCheckable(true);
  connect(pttTestButton, &QPushButton::toggled, this,
          &MainWindow::onPTTTestToggled);
  formLayout->addRow(pttTestButton);

  settingsLayout->addLayout(formLayout);
  settingsLayout->addStretch();

  mainScrollArea->setWidget(scrollContentWidget);
  settingsTabLayout->addWidget(mainScrollArea);

  // Connections
  connect(callsignEdit, &QLineEdit::textChanged, this,
          &MainWindow::saveSettings);
  connect(rxInputCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &MainWindow::onSettingsChanged);
  connect(txOutputCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &MainWindow::onSettingsChanged);
  connect(rxDeviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &MainWindow::onSettingsChanged);
  connect(txDeviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &MainWindow::onSettingsChanged);
  connect(rxClockSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onSettingsChanged);
  connect(txClockSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &MainWindow::onSettingsChanged);
  connect(useCWCheck, &QCheckBox::stateChanged, this,
          &MainWindow::onSettingsChanged);
  connect(cwTextEdit, &QLineEdit::textChanged, this, &MainWindow::saveSettings);
  connect(catEnabledCheck, &QCheckBox::stateChanged, this,
          &MainWindow::onSettingsChanged);
  connect(catRigCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &MainWindow::onSettingsChanged);
  connect(catPortCombo, &QComboBox::currentTextChanged, this,
          &MainWindow::onSettingsChanged);
  connect(catBaudCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &MainWindow::onSettingsChanged);
  connect(catPTTCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &MainWindow::onSettingsChanged);
}

void MainWindow::onSettingsChanged() {
  bool audioChanged =
      (soundRoutingInput != rxInputCombo->currentData().toInt()) ||
      (inputDeviceName != rxDeviceCombo->currentText()) ||
      (soundRoutingOutput != txOutputCombo->currentData().toInt()) ||
      (outputDeviceName != txDeviceCombo->currentText()) ||
      (rxClock != rxClockSpin->value()) || (txClock != txClockSpin->value());

  bool catChanged = (catEnabled != catEnabledCheck->isChecked()) ||
                    (catRigModel != catRigCombo->currentData().toInt()) ||
                    (catSerialPort != catPortCombo->currentText()) ||
                    (catBaudRate != catBaudCombo->currentData().toInt()) ||
                    (catPTTMethod != catPTTCombo->currentData().toInt());

  myCallsign = callsignEdit->text();
  soundRoutingInput = rxInputCombo->currentData().toInt();
  inputDeviceName = rxDeviceCombo->currentText();
  soundRoutingOutput = txOutputCombo->currentData().toInt();
  outputDeviceName = txDeviceCombo->currentText();
  rxClock = rxClockSpin->value();
  txClock = txClockSpin->value();
  useCW = useCWCheck->isChecked();
  cwText = cwTextEdit->text();

  catEnabled = catEnabledCheck->isChecked();
  catRigModel = catRigCombo->currentData().toInt();
  catSerialPort = catPortCombo->currentText();
  catBaudRate = catBaudCombo->currentData().toInt();
  catPTTMethod = catPTTCombo->currentData().toInt();

  saveGlobals();  // Saves global settings (callsign, clocks, etc)
  saveSettings(); // Saves local window settings (modes, dirs)

  if (rigControlPtr && catChanged) {
    if (catEnabled) {
      rigControlPtr->open();
    } else {
      rigControlPtr->close();
    }
  }

  // Re-initialize sound driver ONLY if audio settings changed
  if (soundIOPtr && audioChanged) {
    qDebug() << "Audio settings changed, re-initializing sound driver...";
    soundIOPtr->init(48000);
    soundIOPtr->startCapture();
  }

  qDebug() << "Settings updated. Audio changed:" << audioChanged
           << "CAT changed:" << catChanged;
}

void MainWindow::onModeChanged(int index) {
  if (originalTxImage.isNull())
    return;

  int modeValue = txModeCombo->itemData(index).toInt();
  esstvMode mode = (esstvMode)modeValue;

  if (mode < NUMSSTVMODES) {
    int targetWidth = SSTVTable[mode].numberOfPixels;
    int targetHeight = SSTVTable[mode].numberOfDisplayLines;

    qDebug() << "Resizing image for mode" << getSSTVModeNameShort(mode) << "to"
             << targetWidth << "x" << targetHeight;

    // Use SmoothTransformation for best quality
    // We use IgnoreAspectRatio as standard SSTV formats often force a specific
    // fit
    QImage resized =
        originalTxImage.scaled(targetWidth, targetHeight, Qt::IgnoreAspectRatio,
                               Qt::SmoothTransformation);

    txOverlayWidget->setImage(resized);
  }
}

void MainWindow::loadSettings() {
  loadGlobals();
  QSettings settings("WinSSTV", "WinSSTV");
  lastImportDir = settings.value("lastImportDir").toString();
  lastSaveDir = settings.value("lastSaveDir").toString(); // Load last save dir
  lastRecordDir =
      settings.value("lastRecordDir").toString(); // Load last record dir

  // Load text overlay settings
  lastOverlayText = settings.value("lastOverlayText", "").toString();
  if (lastOverlayText.isEmpty()) {
    lastOverlayText = myCallsign; // Default to callsign if no history
  }
  lastOverlayColor =
      settings.value("lastOverlayColor", QColor(Qt::yellow)).value<QColor>();
  lastOverlaySize = settings.value("lastOverlaySize", 24).toInt();

  // Load enabled modes
  for (int i = 0; i < NUMSSTVMODES; ++i) {
    // Default to the hardcoded value (only Martin 1 enabled) if no setting
    // exists
    SSTVTable[i].enabled =
        settings.value(QString("mode_enabled_%1").arg(i), SSTVTable[i].enabled)
            .toBool();
  }
  refreshModeCombos();
}

void MainWindow::saveSettings() {
  saveGlobals();
  QSettings settings("WinSSTV", "WinSSTV");
  settings.setValue("lastImportDir", lastImportDir);
  settings.setValue("lastSaveDir", lastSaveDir);
  settings.setValue("lastRecordDir", lastRecordDir);

  // Save text overlay settings
  settings.setValue("lastOverlayText", lastOverlayText);
  settings.setValue("lastOverlayColor", lastOverlayColor);
  settings.setValue("lastOverlaySize", lastOverlaySize);

  // Save enabled modes
  for (int i = 0; i < NUMSSTVMODES; ++i) {
    settings.setValue(QString("mode_enabled_%1").arg(i), SSTVTable[i].enabled);
  }

  qDebug() << "saveSettings finished";
}

void MainWindow::refreshModeCombos() {
  // Preserve current selections
  int currentRx = -1;
  if (rxModeCombo->currentIndex() != -1)
    currentRx = rxModeCombo->currentData().toInt();

  int currentTx = -1;
  if (txModeCombo->currentIndex() != -1)
    currentTx = txModeCombo->currentData().toInt();

  // Refresh RX combo
  rxModeCombo->clear();
  rxModeCombo->addItem(tr("Auto"), QVariant::fromValue((int)NOTVALID));
  for (int i = STARTWIDE; i <= ENDWIDE; i++) {
    if (SSTVTable[i].enabled) {
      rxModeCombo->addItem(getSSTVModeNameLong((esstvMode)i),
                           QVariant::fromValue(i));
    }
  }
  int rxIdx = rxModeCombo->findData(QVariant::fromValue(currentRx));
  if (rxIdx != -1)
    rxModeCombo->setCurrentIndex(rxIdx);
  else
    rxModeCombo->setCurrentIndex(0);

  // Refresh TX combo
  txModeCombo->clear();
  for (int i = 0; i < NUMSSTVMODES; ++i) {
    if (SSTVTable[i].enabled) {
      txModeCombo->addItem(getSSTVModeNameLong((esstvMode)i), i);
    }
  }
  int txIdx = txModeCombo->findData(currentTx);
  if (txIdx != -1)
    txModeCombo->setCurrentIndex(txIdx);
  else if (txModeCombo->count() > 0)
    txModeCombo->setCurrentIndex(0);
}

void MainWindow::onTransmit() {
  if (txOverlayWidget && ptrTx) {
    QImage img = txOverlayWidget->getFinalImage();
    esstvMode mode = (esstvMode)txModeCombo->currentData().toInt();

    transmitButton->setEnabled(false);
    transmitButton->setText(tr("Transmitting..."));
    stopTxButton->setEnabled(true);

    if (catEnabled && rigControlPtr) {
      rigControlPtr->setPTT(RIG_PTT_ON);
    }

    // Simplex: Stop RX before TX
    if (soundIOPtr->isCapturing()) {
      soundIOPtr->idleRX();
    }

    // Start playback first so buffer begins to be consumed
    soundIOPtr->startPlayback();

    // Use a background thread for synthesis to avoid freezing the UI
    QThread *thread = new QThread;
    TxWorker *worker = new TxWorker(ptrTx, img, mode, txClock);
    worker->moveToThread(thread);

    connect(thread, &QThread::started, worker, &TxWorker::process);
    connect(worker, &TxWorker::finished, thread, &QThread::quit);
    connect(worker, &TxWorker::finished, worker, &TxWorker::deleteLater);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    connect(worker, &TxWorker::finished, this, &MainWindow::onTxFinished);

    thread->start();

    // Start progress timer
    txProgressTimer->start(100);

    qDebug() << "Transmission thread started for mode" << mode;
  }
}

void MainWindow::onTxFinished() {
  // CW ID (if enabled) was already played inline inside sendImage(),
  // before the drain loop. Nothing special needed here.
  if (soundIOPtr) {
    soundIOPtr->drainPlayback();
    soundIOPtr->idleTX();
  }
  if (catEnabled && rigControlPtr) {
    rigControlPtr->setPTT(RIG_PTT_OFF);
  }
  restoreTxUI();
}

void MainWindow::onCWFinished() {
  // No longer used — kept to avoid linker errors from remaining connections.
}

void MainWindow::restoreTxUI() {
  transmitButton->setEnabled(true);
  transmitButton->setText(tr("Transmit"));
  stopTxButton->setEnabled(false);

  if (txOverlayWidget) {
    // txOverlayWidget->setImage(QImage());      // UX: Keep image loaded
    // txOverlayWidget->clearOverlays();         // UX: Keep overlays
  }
  txProgressBar->setValue(0);
  txProgressTimer->stop();

  // Resume RX
  if (soundIOPtr) {
    soundIOPtr->init(48000);
    soundIOPtr->startCapture();
  }

  qDebug() << "Transmission UI restored";
}

void MainWindow::updateTxProgress() {
  if (ptrTx) {
    int progress = ptrTx->getPercentComplete();
    txProgressBar->setValue(progress);
  }
}

void MainWindow::rxTimerSlot() {
  if (ptrRx && soundIOPtr) {
    while (soundIOPtr->rxBuffer.count() >= RXSTRIPE) {
      DSPFLOAT *data = soundIOPtr->rxBuffer.readPointer();
      DSPFLOAT *vol = soundIOPtr->rxVolumeBuffer.readPointer();

      // Recording Logic
      if (isRecording && audioRecorder) {
        quint16 tempBuf[RXSTRIPE];
        for (int i = 0; i < RXSTRIPE; i++) {
          double val = data[i];
          if (val > 32767)
            val = 32767;
          if (val < -32768)
            val = -32768;
          tempBuf[i] = (quint16)(qint16)val;
        }
        audioRecorder->write(tempBuf, RXSTRIPE, false);
      }

      ptrRx->run(data, vol);

      soundIOPtr->rxBuffer.skip(RXSTRIPE);
      soundIOPtr->rxVolumeBuffer.skip(RXSTRIPE);
    }
  }
}

void MainWindow::onToggleRecording() {
  if (!isRecording) {
    // Starting to record
    QString initialPath =
        lastRecordDir.isEmpty()
            ? QStandardPaths::writableLocation(QStandardPaths::MusicLocation)
            : lastRecordDir;
    QString defaultFileName =
        initialPath + "/WinSSTV_" +
        QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".wav";

    QString fileName = QFileDialog::getSaveFileName(
        this, tr("Save Recording"), defaultFileName, tr("WAV Files (*.wav)"));

    if (fileName.isEmpty()) {
      recordButton->setChecked(false); // Untoggle button if cancelled
      return;
    }

    lastRecordDir = QFileInfo(fileName).absolutePath();
    saveSettings();

    if (!audioRecorder->openFileForWrite(fileName, false, false)) {
      qDebug() << "Failed to open recorder file:" << fileName;
      recordButton->setChecked(false);
      return;
    }

    isRecording = true;
    recordButton->setText(tr("Stop Recording"));
  } else {
    // Stopping recording
    isRecording = false;
    recordButton->setText(tr("Record Audio"));
    audioRecorder->closeFile();
  }
}

void MainWindow::onDeepDecode() {
  QString fileName = QFileDialog::getOpenFileName(this, tr("Open Audio File"),
                                                  "", tr("WAV Files (*.wav)"));
  if (!fileName.isEmpty()) {
    processDeepDecode(fileName);
  }
}

void MainWindow::processDeepDecode(const QString &filename) {
  wavIO inputFile(12000);
  if (!inputFile.openFileForRead(filename, false)) {
    QMessageBox::critical(this, tr("Deep Decode Error"),
                          tr("Could not open file or unsupported format.\n"
                             "Note: Deep Decode currently only supports 12kHz "
                             "mono WAV files recorded by WinSSTV."));
    return;
  }

  uint totalSamples = inputFile.getNumberOfSamples();
  if (totalSamples == 0) {
    QMessageBox::warning(this, tr("Deep Decode"), tr("File is empty."));
    inputFile.closeFile();
    return;
  }

  // Ask to start
  if (QMessageBox::question(this, tr("Deep Decode"),
                            tr("Ready to decode %1.\nTotal samples: %2\nStart "
                               "processing?")
                                .arg(filename)
                                .arg(totalSamples)) != QMessageBox::Yes) {
    inputFile.closeFile();
    return;
  }

  // Force 48kHz reference for the offline decoder FIRST.
  // The SSTV filters use (rxClock / 4) as their internal sampling rate.
  // Since the WAV is 12kHz, we need (rxClock / 4) to be exactly 12000.
  double oldRxClock = rxClock;
  rxClock = 48000.0;

  // Set up offline RX (Must happen AFTER rxClock is set so the filters init
  // correctly)
  sstvRx offlineRx;
  offlineRx.init();

  // Progress dialog
  QProgressDialog progress(tr("Deep Decoding..."), tr("Cancel"), 0,
                           totalSamples, this);
  progress.setWindowModality(Qt::WindowModal);
  progress.setMinimumDuration(0);
  progress.setValue(0);

  const int CHUNK_SIZE = RXSTRIPE; // 1024
  qint16 rawBuf[CHUNK_SIZE];
  DSPFLOAT dspBuf[CHUNK_SIZE];
  DSPFLOAT volBuf[CHUNK_SIZE];

  uint processedSamples = 0;

  while (true) {
    int readCount = inputFile.read(rawBuf, CHUNK_SIZE);
    if (readCount <= 0)
      break;

    // Convert and fill buffers
    for (int i = 0; i < readCount; i++) {
      // Boost amplitude: generated Test WAVs max out at 8000.
      // We multiply by 3 to reach robust detection bounds (~24000)
      dspBuf[i] = (DSPFLOAT)rawBuf[i] * 3.0;
      volBuf[i] =
          20000.0; // High mock volume to exceed rigorous sync thresholds
    }

    // Pad if readCount < CHUNK_SIZE
    for (int i = readCount; i < CHUNK_SIZE; i++) {
      dspBuf[i] = 0;
      volBuf[i] = 0;
    }

    offlineRx.run(dspBuf, volBuf);

    processedSamples += readCount;
    progress.setValue(processedSamples);

    // Update main window live preview so user sees it working
    rxImageLabel->setPixmap(QPixmap::fromImage(offlineRx.getLastReceivedImage())
                                .scaled(rxImageLabel->size(),
                                        Qt::KeepAspectRatio,
                                        Qt::SmoothTransformation));

    QApplication::processEvents();

    if (progress.wasCanceled())
      break;
  }
  inputFile.closeFile();

  // Restore global rxClock from settings/calibration
  rxClock = oldRxClock;

  if (!progress.wasCanceled()) {
    QImage result = offlineRx.getLastReceivedImage();
    if (!result.isNull()) {
      if (denoiseCheckBox->isChecked()) {
        rxStatusLabel->setText(tr("Deep Decode Complete - Denoising..."));
        QApplication::processEvents();
        result = applyMedianFilter(result);
      }
      rxImageLabel->setPixmap(QPixmap::fromImage(result).scaled(
          rxImageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
      rxStatusLabel->setText(tr("Deep Decode Complete"));
      QMessageBox::information(this, tr("Deep Decode"),
                               tr("Processing complete."));
    } else {
      rxStatusLabel->setText(tr("Deep Decode Failed (No Image)"));
      QMessageBox::warning(this, tr("Deep Decode"),
                           tr("No image was decoded."));
    }
  } else {
    rxStatusLabel->setText(tr("Deep Decode Canceled"));
  }
}

// Simple 3x3 Median Filter
QImage MainWindow::applyMedianFilter(const QImage &source) {
  if (source.isNull())
    return QImage();
  QImage result = source.convertToFormat(QImage::Format_ARGB32);
  int width = result.width();
  int height = result.height();

  // Arrays to store 3x3 neighborhood
  int r[9], g[9], b[9];

  for (int y = 1; y < height - 1; ++y) {
    for (int x = 1; x < width - 1; ++x) {
      int k = 0;
      for (int j = -1; j <= 1; ++j) {
        for (int i = -1; i <= 1; ++i) {
          QRgb pixel = source.pixel(x + i, y + j);
          r[k] = qRed(pixel);
          g[k] = qGreen(pixel);
          b[k] = qBlue(pixel);
          k++;
        }
      }
      // Sort to find median (index 4)
      std::sort(r, r + 9);
      std::sort(g, g + 9);
      std::sort(b, b + 9);

      result.setPixel(x, y, qRgb(r[4], g[4], b[4]));
    }
  }
  return result;
}

void MainWindow::onProcessAudio(const QVector<double> &data) { Q_UNUSED(data); }

void MainWindow::onRxStatus(QString status) { rxStatusLabel->setText(status); }

void MainWindow::onRxSync(double quality) { (void)quality; }

void MainWindow::onRxEnd(int mode) {
  if (ptrRx) {
    QImage img = ptrRx->getLastReceivedImage();
    if (!img.isNull()) {
      rxImageLabel->setPixmap(QPixmap::fromImage(img).scaled(
          rxImageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
  }
  if (mode == NOTVALID) {
    rxStatusLabel->setText(tr("Receive incomplete — image not saved"));
  } else {
    rxStatusLabel->setText(
        tr("Received: %1").arg(getSSTVModeNameLong((esstvMode)mode)));
  }
}

void MainWindow::rxImageUpdateSlot() {
  if (ptrRx && ptrRx->isBusy()) {
    QImage img = ptrRx->getLastReceivedImage();
    if (!img.isNull()) {
      rxImageLabel->setPixmap(QPixmap::fromImage(img).scaled(
          rxImageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
  }
}

static int populate_callback(const struct rig_caps *caps, void *data) {
  QComboBox *combo = (QComboBox *)data;
  combo->addItem(QString("%1 %2").arg(caps->mfg_name).arg(caps->model_name),
                 (int)caps->rig_model);
  return 1;
}

void MainWindow::populateRigModels() {
  catRigCombo->clear();
  catRigCombo->addItem(tr("None / Manual"), 0);
  rig_load_all_backends();
  rig_list_foreach(populate_callback, (void *)catRigCombo);
}

void MainWindow::onPTTTestToggled(bool checked) {
  if (rigControlPtr && catEnabled) {
    if (checked) {
      pttTestButton->setText(tr("PTT ON (Click to OFF)"));
      // Force PTT ON
      rigControlPtr->setPTT(RIG_PTT_ON);
    } else {
      pttTestButton->setText(tr("Test PTT (Toggle)"));
      // Force PTT OFF
      rigControlPtr->setPTT(RIG_PTT_OFF);
    }
  } else {
    // If CAT not enabled, untoggle immediately
    if (checked) {
      pttTestButton->setChecked(false);
      // Update status label to warn user?
      qDebug() << "PTT Test ignored: CAT disabled or not initialized";
    }
  }
}

void MainWindow::onRefreshSerialPorts() {
  QString current = catPortCombo->currentText();
  catPortCombo->clear();
  for (const QSerialPortInfo &info : QSerialPortInfo::availablePorts()) {
    catPortCombo->addItem(info.portName());
  }
  if (!current.isEmpty()) {
    catPortCombo->setCurrentText(current);
  }
}
void MainWindow::onFreqUIChanged(double freq) {
  if (rigControlPtr && rigControlPtr->isOpen()) {
    rigControlPtr->setFreq(freq * 1000000.0); // Convert MHz to Hz
  }
}

void MainWindow::onBandUIChanged(int index) {
  double freq = bandCombo->itemData(index).toDouble();
  if (freq > 0) {
    freqSpin->setValue(freq);
    // onFreqUIChanged will be triggered automatically
  }
}

void MainWindow::rigPollSlot() {
  if (rigControlPtr && rigControlPtr->isOpen()) {
    double freq = rigControlPtr->getFreq();
    if (freq > 0) {
      double freqMHz = freq / 1000000.0;
      if (qAbs(freqMHz - freqSpin->value()) > 0.000001) {
        freqSpin->blockSignals(true);
        freqSpin->setValue(freqMHz);
        freqSpin->blockSignals(false);
      }
    }
  }
}

void MainWindow::onGenerateTestWav() {
  if (!txOverlayWidget || !ptrTx)
    return;

  QImage img = txOverlayWidget->getFinalImage();
  if (img.isNull()) {
    QMessageBox::warning(this, tr("Generate WAV"),
                         tr("No image loaded in Transmit tab."));
    return;
  }

  esstvMode mode = (esstvMode)txModeCombo->currentData().toInt();

  QString initialPath = lastSaveDir.isEmpty() ? QDir::homePath() : lastSaveDir;
  QString fileName = QFileDialog::getSaveFileName(
      this, tr("Save Test WAV"),
      initialPath + "/SSTV_Test_" + getSSTVModeNameShort(mode) + ".wav",
      tr("WAV Files (*.wav)"));

  if (fileName.isEmpty())
    return;

  lastSaveDir = QFileInfo(fileName).absolutePath();
  saveSettings();

  // Show a simpler progress dialog (or just reuse the bar)
  // Since it's fast but could take seconds for high-res modes, a dialog is nice
  QProgressDialog *progress =
      new QProgressDialog(tr("Generating WAV..."), tr("Abort"), 0, 100, this);
  progress->setWindowModality(Qt::WindowModal);
  progress->setAutoClose(true);
  progress->show();

  QThread *thread = new QThread;
  // Deep Decode requires 12kHz.
  GenWorker *worker = new GenWorker(ptrTx, img, mode, 12000.0, fileName);
  worker->moveToThread(thread);

  connect(thread, &QThread::started, worker, &GenWorker::process);
  connect(worker, &GenWorker::finished, thread, &QThread::quit);
  connect(worker, &GenWorker::finished, progress, &QProgressDialog::close);
  connect(worker, &GenWorker::finished, worker, &GenWorker::deleteLater);
  connect(thread, &QThread::finished, thread, &QThread::deleteLater);
  connect(worker, &GenWorker::finished, this, &MainWindow::onGenFinished);

  // Sync completion bar
  QTimer *syncTimer = new QTimer(this);
  connect(syncTimer, &QTimer::timeout, [this, progress]() {
    int p = ptrTx->getPercentComplete();
    progress->setValue(p);
  });
  connect(worker, &GenWorker::finished, syncTimer, &QTimer::stop);
  connect(worker, &GenWorker::finished, syncTimer, &QTimer::deleteLater);
  syncTimer->start(100);

  thread->start();
}

void MainWindow::onGenFinished(bool success) {
  if (success) {
    QMessageBox::information(this, tr("Generate WAV"),
                             tr("WAV file generated successfully."));
  } else {
    QMessageBox::critical(this, tr("Generate WAV"),
                          tr("Failed to generate WAV file."));
  }
}

void MainWindow::onRxModeComboChanged(int index) {
  Q_UNUSED(index);
  sstvModeIndexRx = (esstvMode)rxModeCombo->currentData().toInt();

  if (ptrRx) {
    ptrRx->init();
  }
}
