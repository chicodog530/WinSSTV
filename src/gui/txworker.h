#ifndef TXWORKER_H
#define TXWORKER_H

#include "appdefs.h"
#include "sstv/sstvtx.h"
#include <QImage>
#include <QObject>
#include <QString>

class TxWorker : public QObject {
  Q_OBJECT
public:
  explicit TxWorker(sstvTx *tx, const QImage &img, esstvMode mode, double clock,
                    QObject *parent = nullptr);

public slots:
  void process();

signals:
  void finished();
  void error(QString message);

private:
  sstvTx *ptrTx;
  QImage image;
  esstvMode txMode;
  double txClock;
};

class TxCWWorker : public QObject {
  Q_OBJECT
public:
  explicit TxCWWorker(sstvTx *tx, QObject *parent = nullptr);

public slots:
  void process();

signals:
  void finished();

private:
  sstvTx *ptrTx;
};

class GenWorker : public QObject {
  Q_OBJECT
public:
  explicit GenWorker(sstvTx *tx, const QImage &img, esstvMode mode,
                     double clock, const QString &path,
                     QObject *parent = nullptr);

public slots:
  void process();

signals:
  void finished(bool success);

private:
  sstvTx *ptrTx;
  QImage image;
  esstvMode txMode;
  double txClock;
  QString wavPath;
};

#endif // TXWORKER_H
