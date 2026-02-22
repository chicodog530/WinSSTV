#ifndef DISPATCHER_H
#define DISPATCHER_H

#include "sstvparam.h"
#include <QEvent>
#include <QObject>
#include <QSize>
#include <QString>

// Event Types
const QEvent::Type EVT_START_RX = (QEvent::Type)(QEvent::User + 1);
const QEvent::Type EVT_SYNC = (QEvent::Type)(QEvent::User + 2);
const QEvent::Type EVT_LINE = (QEvent::Type)(QEvent::User + 3);
const QEvent::Type EVT_STATUS = (QEvent::Type)(QEvent::User + 4);
const QEvent::Type EVT_MBOX = (QEvent::Type)(QEvent::User + 5);
const QEvent::Type EVT_END_RX = (QEvent::Type)(QEvent::User + 6);

class dispatcher : public QObject {
  Q_OBJECT
public:
  explicit dispatcher(QObject *parent = nullptr) : QObject(parent) {}

protected:
  void customEvent(QEvent *e) override;

signals:
  void rxStatus(QString status);
  void rxSync(double quality);
  void rxEnd(int mode);
  void rxLine(int line);
};

class startImageRXEvent : public QEvent {
public:
  startImageRXEvent(QSize size) : QEvent(EVT_START_RX) { (void)size; }
  void waitFor(bool *d) {
    if (d)
      *d = true;
  }
};

class displaySyncEvent : public QEvent {
public:
  double quality;
  displaySyncEvent(double q) : QEvent(EVT_SYNC), quality(q) {}
};

class lineDisplayEvent : public QEvent {
public:
  int line;
  lineDisplayEvent(int l) : QEvent(EVT_LINE), line(l) {}
};

class rxSSTVStatusEvent : public QEvent {
public:
  QString status;
  rxSSTVStatusEvent(const QString &s) : QEvent(EVT_STATUS), status(s) {}
};

class displayMBoxEvent : public QEvent {
public:
  displayMBoxEvent(const QString &title, const QString &text)
      : QEvent(EVT_MBOX) {
    (void)title;
    (void)text;
  }
};

class endImageSSTVRXEvent : public QEvent {
public:
  esstvMode mode;
  endImageSSTVRXEvent(esstvMode m) : QEvent(EVT_END_RX), mode(m) {}
  void waitFor(bool *d) {
    if (d)
      *d = true;
  }
};

#endif // DISPATCHER_H
