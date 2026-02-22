#include "appglobal.h"
#include "configparams.h"
#include "dispatcher.h"
#include "dsp/synthes.h"
#include "soundwindows.h"
#include "utils/rigcontrol.h"
#include <QSettings>
#include <iostream>

const QString MAJORVERSION = "9";
const QString CONFIGVERSION = "1";
const QString MINORVERSION = "0";
const QString ORGANIZATION = "WinSSTV";
const QString APPLICATION = "WinSSTV";
const QString qsstvVersion = "9.0.0";
const QString APPNAME = "WinSSTV";

MainWindow *mainWindowPtr = nullptr;
soundBase *soundIOPtr = nullptr;
dispatcher *dispatcherPtr = nullptr;
logFile *logFilePtr = nullptr;
RigControl *rigControlPtr = nullptr;

bool useHybrid = false;
bool inStartup = false;
QString myCallsign = "N0CALL";
double rxClock = 48000.0;
double txClock = 48000.0;
QString rxDRMImagesPath = "";
QString audioPath = "";
QString lastImportDir = "";

// Config Params Stubs
QString hybridFtpRemoteHost = "";
QString hybridFtpLogin = "";
QString hybridFtpPassword = "";
QString hybridFtpRemoteDirectory = "";
int soundRoutingInput = 0;
int soundRoutingOutput = 0;
QString inputDeviceName = "";
QString outputDeviceName = "";
int recordingSize = 0;
int cwWPM = 20;
int cwTone = 800;
bool useCW = false;
QString cwText = "CQ CQ CQ DE %m %m %m K";
bool useVOX = false;
QColor imageBackGroundColor = Qt::black;
bool pttToneOtherChannel = false;
bool swapChannel = false;

// CAT Globals
bool catEnabled = false;
int catRigModel = 0; // RIG_MODEL_DUMMY or similar
QString catSerialPort = "";
int catBaudRate = 4800;
int catPTTMethod = 0; // RIG_PTT_NONE

// includes moved to top of file

void globalInit(void) {
  loadGlobals();

  soundIOPtr = new soundWindows();
  std::cout << "SoundIO initialized" << std::endl;
  soundIOPtr->init(48000); // Hardware runs at 48kHz
  std::cout << "SoundIO init called" << std::endl;
  dispatcherPtr = new dispatcher(); // Init dispatcher
  std::cout << "Dispatcher initialized" << std::endl;
  synthesPtr = new synthesizer(48000.0); // Init synthesizer with 48kHz clock
  std::cout << "Synthesizer initialized" << std::endl;

  // Initialize CAT Control
  rigControlPtr = new RigControl();
  std::cout << "RigControl initialized" << std::endl;
  if (catEnabled) {
    std::cout << "Opening RigControl..." << std::endl;
    rigControlPtr->open();
    std::cout << "RigControl Open returned." << std::endl;
  }

  // Use configured routing or default to soundcard
  if (soundRoutingInput == 0)
    soundRoutingInput = soundBase::SNDINCARD;

  soundIOPtr->start();        // Start the thread
  soundIOPtr->startCapture(); // Begin audio capture
  std::cout << "Sound capture started" << std::endl;
}

void loadGlobals() {
  QSettings s(QSettings::IniFormat, QSettings::UserScope, ORGANIZATION,
              APPLICATION);
  myCallsign = s.value("callsign", "N0CALL").toString();
  soundRoutingInput =
      s.value("soundRoutingInput", (int)soundBase::SNDINCARD).toInt();
  inputDeviceName = s.value("inputDeviceName", "").toString();
  soundRoutingOutput =
      s.value("soundRoutingOutput", (int)soundBase::SNDOUTCARD).toInt();
  outputDeviceName = s.value("outputDeviceName", "").toString();
  rxClock = s.value("rxClock", 48000.0).toDouble();
  txClock = s.value("txClock", 48000.0).toDouble();
  useCW = s.value("useCW", false).toBool();
  cwText = s.value("cwText", "CQ CQ CQ DE %m %m %m K").toString();
  lastImportDir = s.value("lastImportDir", "").toString();

  catEnabled = s.value("catEnabled", false).toBool();
  catRigModel = s.value("catRigModel", 0).toInt();
  catSerialPort = s.value("catSerialPort", "").toString();
  catBaudRate = s.value("catBaudRate", 4800).toInt();
  catPTTMethod = s.value("catPTTMethod", 0).toInt();

  qDebug() << "Globals Loaded: " << s.fileName();
  qDebug() << "Callsign:" << myCallsign << "InDevice:" << inputDeviceName;
}

void saveGlobals() {
  QSettings s(QSettings::IniFormat, QSettings::UserScope, ORGANIZATION,
              APPLICATION);
  s.setValue("callsign", myCallsign);
  s.setValue("soundRoutingInput", soundRoutingInput);
  s.setValue("inputDeviceName", inputDeviceName);
  s.setValue("soundRoutingOutput", soundRoutingOutput);
  s.setValue("outputDeviceName", outputDeviceName);
  s.setValue("rxClock", rxClock);
  s.setValue("txClock", txClock);
  s.setValue("useCW", useCW);
  s.setValue("cwText", cwText);

  s.setValue("catEnabled", catEnabled);
  s.setValue("catRigModel", catRigModel);
  s.setValue("catSerialPort", catSerialPort);
  s.setValue("catBaudRate", catBaudRate);
  s.setValue("catPTTMethod", catPTTMethod);
  s.setValue("lastImportDir", lastImportDir);
  s.sync();
  qDebug() << "Globals Saved to: " << s.fileName();
}
