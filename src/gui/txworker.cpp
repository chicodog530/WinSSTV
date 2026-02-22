#include "txworker.h"
#include <QDebug>

TxWorker::TxWorker(sstvTx *tx, const QImage &img, esstvMode mode, double clock,
                   QObject *parent)
    : QObject(parent), ptrTx(tx), image(img), txMode(mode), txClock(clock) {}

void TxWorker::process() {
  if (!ptrTx) {
    emit error("SSTV TX core not initialized");
    emit finished();
    return;
  }

  qDebug() << "TxWorker: Starting transmission for mode" << txMode;
  if (ptrTx->create(txMode, txClock) && ptrTx->sendImage(image)) {
    qDebug() << "TxWorker: Transmission finished successfully";
  } else {
    if (ptrTx->aborted()) {
      qDebug() << "TxWorker: Transmission aborted";
    } else {
      qDebug() << "TxWorker: Transmission failed";
      emit error("Transmission failed to initialize or was interrupted");
    }
  }

  emit finished();
}

TxCWWorker::TxCWWorker(sstvTx *tx, QObject *parent)
    : QObject(parent), ptrTx(tx) {}

void TxCWWorker::process() {
  // No longer used — CW is now inline inside sstvTx::sendImage().
  emit finished();
}

GenWorker::GenWorker(sstvTx *tx, const QImage &img, esstvMode mode,
                     double clock, const QString &path, QObject *parent)
    : QObject(parent), ptrTx(tx), image(img), txMode(mode), txClock(clock),
      wavPath(path) {}

void GenWorker::process() {
  bool ok = false;
  if (ptrTx) {
    ok = ptrTx->generateToWav(image, txMode, txClock, wavPath);
  }
  emit finished(ok);
}
