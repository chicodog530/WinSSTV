#include "logging.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QStringList>
#include <QTextStream>

// Removed ui_loggingform.h and GUI classes

logFile::logFile() : maskBA(NUMDEBUGLEVELS, false) {
#ifdef ENABLELOGGING
  lf = new QFile;
#ifdef ENABLEAUX
  auxFile = new QFile;
#endif
#endif
  logCount = 0;
  savedLogEntry = "";
  savedPosMask = 0;
}

#ifdef ENABLELOGGING
bool logFile::open(QString logname) {
  QString desktopPath =
      QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
  lf->setFileName(desktopPath + "/" + logname);
#ifdef ENABLEAUX
  auxFile->setFileName(desktopPath + "/aux_" + logname);
#endif
  return reopen();
}
#else
bool logFile::open(QString) { return true; }
#endif

logFile::~logFile() { close(); }

void logFile::close() {
#ifdef ENABLELOGGING
  errorOut() << "closing logfile";
  add("End of logfile", LOGALL);
  add("....,", LOGALL);
  if (ts)
    delete ts; // Check if ts exists
  lf->close();
#ifdef ENABLEAUX
  if (auxTs)
    delete auxTs;
  auxFile->close();
#endif
#endif
}

void logFile::reset() {
  close();
  reopen();
}

bool logFile::reopen() {
#ifdef ENABLELOGGING
  setEnabled(false);
  QFileInfo finf(*lf);
#ifdef ENABLEAUX
  QFileInfo finfaux(*auxFile);
#endif
  errorOut() << "opening logfile--: " << finf.absoluteFilePath();
  if (!lf->open(QIODevice::WriteOnly)) {
    errorOut() << "logfile creation failed";
    return false;
  }
#ifdef ENABLEAUX
  errorOut() << "opening logfile: " << finfaux.absoluteFilePath();
  if (!auxFile->open(QIODevice::WriteOnly)) {
    errorOut() << "auxiliary file creation failed";
    lf->close();
    return false;
  }
#endif
  setEnabled(true);
  ts = new QTextStream(lf);
#ifdef ENABLEAUX
  auxTs = new QTextStream(auxFile);
#endif
  savedLogEntry = "";
  logCount = 0;
  timer.start();
  *ts << "Time \tElapsed  \t  Level  \t  Count\t          Info\n";
  ts->flush();
#endif
  return true;
}

#ifdef ENABLELOGGING
void logFile::add(QString message, short unsigned int posMask) {
  QString tempStr, tempStr_qd;
  if (!(posMask == LOGALL)) // always show messages with DBALL
  {
    if (!maskBA[posMask])
      return;
  }
  if (!enabled)
    return;
  mutex.lock();
  if (logCount == 0) {
    logCount = 1;
    savedLogEntry = message;
    timer.restart();
    tmp = QString("%1 ").arg(timer.elapsed(), 5);
    tmp2 = QTime::currentTime().toString("HH:mm:ss:zzz ");
    savedPosMask = posMask;
  }
  if ((message == savedLogEntry) && (deduplicate))
    logCount++;
  else {
    if (!deduplicate) {
      savedLogEntry = message;
      tmp = QString("%1 ").arg(timer.elapsed(), 5);
      tmp2 = QTime::currentTime().toString("HH:mm:ss:zzz ");
      savedPosMask = posMask;
    }
    if (savedPosMask == LOGALL) {
      tempStr = QString("%1\t%2\tALL     \t%3\t%4\n")
                    .arg(tmp2)
                    .arg(tmp)
                    .arg(logCount)
                    .arg(savedLogEntry);
      if (timestamp)
        tempStr_qd = QString("%1 %2 ALL      %3 %4")
                         .arg(tmp2)
                         .arg(tmp)
                         .arg(logCount)
                         .arg(savedLogEntry);
      else
        tempStr_qd = QString("ALL      %3 %4").arg(logCount).arg(savedLogEntry);
    } else {
      tempStr = QString("%1\t%2\t%3\t%4\t%5\n")
                    .arg(tmp2)
                    .arg(tmp)
                    .arg(levelStr[savedPosMask])
                    .arg(logCount)
                    .arg(savedLogEntry);
      if (timestamp)
        tempStr_qd = QString("%1 %2 %3 %4 %5")
                         .arg(tmp2)
                         .arg(tmp)
                         .arg(levelStr[savedPosMask])
                         .arg(logCount)
                         .arg(savedLogEntry);
      else
        tempStr_qd = QString("%3 %4 %5")
                         .arg(levelStr[savedPosMask])
                         .arg(logCount)
                         .arg(savedLogEntry);
    }
    *ts << tempStr;
    if (outputDebug)
      qDebug() << tempStr_qd;

    timer.restart();
    ;
    savedLogEntry = message;
    savedPosMask = posMask;
    logCount = 1;
  }
  ts->flush();
  lf->flush();
  mutex.unlock();
}
#else
void logFile::add(QString, short unsigned int) {}
#endif

void logFile::add(const char *fileName, const char *functionName, int line,
                  QString t, short unsigned int posMask) {
  QString s;
  QFileInfo finfo(fileName);

  if (debugRef) {
    s = QString(finfo.fileName()) + ":" + QString(functionName) + ":" +
        QString::number(line) + " ";
  }
  s += t;
  if (s[0] == '\t') {
    s.remove(0, 1);
  }
  add(s, posMask);
}

#ifdef ENABLEAUX
void logFile::addToAux(QString t) {
  if (!enabled)
    return;
  mutex.lock();
  *auxTs << t << "\n";
  auxTs->flush();
  auxFile->flush();
  mutex.unlock();
}
#else
void logFile::addToAux(QString) {}
#endif

bool logFile::setEnabled(bool enable) {
  bool t = enabled;
  enabled = enable;
  return t;
}

void logFile::setLogMask(QBitArray logMask) { maskBA = logMask; }

void logFile::maskSelect(QWidget *wPtr) {
  (void)wPtr;
  // GUI Disabled
}

void logFile::readSettings() {
  QBitArray ba;
  QSettings qSettings;
  qSettings.beginGroup("logging");
  maskBA =
      qSettings.value("maskBA", QBitArray(NUMDEBUGLEVELS, false)).toBitArray();
  if (maskBA.size() < NUMDEBUGLEVELS) {
    maskBA = QBitArray(NUMDEBUGLEVELS, false);
  }
  outputDebug = qSettings.value("outputDebug", false).toBool();
  debugRef = qSettings.value("debugRef", false).toBool();
  deduplicate = qSettings.value("deduplicate", true).toBool();
  timestamp = qSettings.value("timestamp", false).toBool();
  qSettings.endGroup();
}

void logFile::writeSettings() {
  QSettings qSettings;
  qSettings.beginGroup("logging");
  qSettings.setValue("maskBA", maskBA);
  qSettings.setValue("outputDebug", outputDebug);
  qSettings.setValue("debugRef", debugRef);
  qSettings.setValue("deduplicate", deduplicate);
  qSettings.setValue("timestamp", timestamp);
  qSettings.endGroup();
}
