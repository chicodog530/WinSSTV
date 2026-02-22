#include "dispatcher.h"

void dispatcher::customEvent(QEvent *e) {
  if (e->type() == EVT_STATUS) {
    rxSSTVStatusEvent *se = static_cast<rxSSTVStatusEvent *>(e);
    emit rxStatus(se->status);
  } else if (e->type() == EVT_SYNC) {
    displaySyncEvent *se = static_cast<displaySyncEvent *>(e);
    emit rxSync(se->quality);
  } else if (e->type() == EVT_LINE) {
    lineDisplayEvent *le = static_cast<lineDisplayEvent *>(e);
    emit rxLine(le->line);
  } else if (e->type() == EVT_END_RX) {
    endImageSSTVRXEvent *ee = static_cast<endImageSSTVRXEvent *>(e);
    emit rxEnd((int)ee->mode);
  }
}
