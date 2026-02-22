#ifndef APPGLOBAL_H
#define APPGLOBAL_H

#include "appdefs.h"
#include "logging.h"
#include <QObject>
#include <QString>

// Version Stubs
extern const QString MAJORVERSION;
extern const QString CONFIGVERSION;
extern const QString MINORVERSION;
extern const QString ORGANIZATION;
extern const QString APPLICATION;
extern const QString qsstvVersion;
extern const QString APPNAME;

// Global Pointers Stubs (Initialized to nullptr usually, ensuring we don't
// crash if they are checked) In the long run, we want to remove these
// dependencies.

class MainWindow;
class soundBase;
class dispatcher;
class RigControl;

extern MainWindow *mainWindowPtr;
extern soundBase *soundIOPtr;
extern dispatcher *dispatcherPtr;
extern logFile *logFilePtr;
extern RigControl *rigControlPtr;

// Other globals used by core
extern bool useHybrid;
extern bool inStartup;
extern QString myCallsign;
extern double rxClock;
extern double txClock;
extern QString rxDRMImagesPath;
extern QString audioPath;
extern QString lastImportDir;
extern QString inputDeviceName;
extern QString outputDeviceName;

// CAT Control
extern bool catEnabled;
extern int catRigModel;
extern QString catSerialPort;
extern int catBaudRate;
extern int catPTTMethod;

// Init and Persistence
void globalInit(void);
void loadGlobals();
void saveGlobals();

#endif // APPGLOBAL_H
