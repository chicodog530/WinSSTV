#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "overlaywidget.h"
#include "sstv/sstvrx.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QMainWindow>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QSerialPortInfo>
#include <QSettings>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>


class WaterfallWidget; // Forward declaration
class sstvTx;
class sstvRx;
class wavIO; // Forward declaration

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

  void loadSettings();
  void saveSettings();

private:
  void setupUi();
  void setupReceiveTab();
  void setupTransmitTab();
  void setupSettingsTab();

  QTabWidget *tabWidget;
  QWidget *receiveTab;
  QWidget *transmitTab;
  QWidget *settingsTab;

  // RX Widgets
  WaterfallWidget *rxWaterfall;
  QLabel *rxImageLabel;
  QLabel *rxStatusLabel; // shows "Receiving Scottie 1", sync quality etc.
  QPushButton *saveImageButton;

  // TX Widgets
  QPushButton *importImageButton;
  QPushButton *addTextButton;
  QPushButton *transmitButton;
  QPushButton *stopTxButton;       // Added Stop TX Button
  QPushButton *refreshPortsButton; // Added Refresh Ports Button
  QProgressBar *txProgressBar;
  QComboBox *txModeCombo;
  OverlayWidget *txOverlayWidget;
  QPushButton *generateWavButton;
  void populateRigModels();

  // CAT Top Bar
  QComboBox *bandCombo;
  QDoubleSpinBox *freqSpin;
  QTimer *rigPollTimer;

  // Audio Recording
  QPushButton *recordButton;
  wavIO *audioRecorder;
  bool isRecording;

  // Deep Decode
  QPushButton *deepDecodeButton;
  QCheckBox *denoiseCheckBox; // Added
  QComboBox *rxModeCombo;     // Added for RX Mode selection
  void processDeepDecode(const QString &filename);
  QImage applyMedianFilter(const QImage &source); // Added

private slots:
  void onToggleRecording();
  void onDeepDecode();
  void onProcessAudio(const QVector<double> &data);
  void onRxModeComboChanged(int index); // Added

  // Existing slots...
  void onImportImage();

  // SSTV Cores
private:
  sstvTx *ptrTx;
  sstvRx *ptrRx;
  QTimer *rxTimer;
  QTimer *txProgressTimer;
  QTimer *rxImageUpdateTimer; // refreshes the received image label live

  // Settings Tab Widgets
  QLineEdit *callsignEdit;
  QComboBox *rxInputCombo;
  QComboBox *rxDeviceCombo;
  QComboBox *txOutputCombo;
  QComboBox *txDeviceCombo;
  QDoubleSpinBox *rxClockSpin;
  QDoubleSpinBox *txClockSpin;
  QCheckBox *useCWCheck;
  QLineEdit *cwTextEdit;

  // CAT Settings
  QCheckBox *catEnabledCheck;
  QComboBox *catRigCombo;
  QComboBox *catPortCombo; // Changed from QLineEdit to QComboBox
  QComboBox *catBaudCombo;
  QComboBox *catPTTCombo;
  QPushButton *pttTestButton; // Added PTT Test Button
  QImage originalTxImage;     // Store unscaled source image

private slots:

  void onAddText();
  void onSaveImage();
  void onPTTTestToggled(bool checked); // Added PTT Test Slot
  void onRefreshSerialPorts();         // Added Refresh Ports Slot
  void onCWFinished();
  void restoreTxUI();
  void onTransmit();
  void onStopTx();    // Added Stop TX Slot
  void rxTimerSlot(); // Polling slot for RX
  // Dispatcher Slots
  void onRxStatus(QString status);
  void onRxSync(double quality);
  void onRxEnd(int mode);
  void onTxFinished();
  void updateTxProgress();
  void rxImageUpdateSlot(); // live refresh of received image

  // Persistence
private:
  QString lastSaveDir;
  QString lastRecordDir; // Added
  QString lastOverlayText;
  QColor lastOverlayColor;
  int lastOverlaySize;

  QList<QCheckBox *> modeCheckBoxes;
  void refreshModeCombos();

private slots:
  void onSettingsChanged();
  void onModeChanged(int);
  void onGenerateTestWav();
  void onGenFinished(bool success);

  // CAT Slots
  void onFreqUIChanged(double freq);
  void onBandUIChanged(int index);
  void rigPollSlot();
};

#endif // MAINWINDOW_H
