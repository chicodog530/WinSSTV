#include "sstvtx.h"
#include "appglobal.h"
#include "configparams.h"
#include "cw.h"
#include "dispatcher.h"
#include "modes/modes.h"
#include "soundbase.h"
#include "synthes.h"
#include <QDebug>

sstvTx::sstvTx() {
  currentMode = 0;
  oldMode = NOTVALID;
}
sstvTx::~sstvTx() {
  if (currentMode)
    delete currentMode;
}

void sstvTx::init() { sampleCounter = 0; }

void sstvTx::sendPreamble() {
  addToLog("txFunc:sendPreamble", LOGTXFUNC);

  synthesPtr->sendTone(0.1, 1900., 0, true);
  synthesPtr->sendTone(0.1, 1500., 0, true);
  synthesPtr->sendTone(0.1, 1900., 0, true);
  synthesPtr->sendTone(0.1, 1500., 0, true);
  synthesPtr->sendTone(0.1, 2300., 0, true);
  synthesPtr->sendTone(0.1, 1500., 0, true);
  synthesPtr->sendTone(0.1, 2300., 0, true);
  synthesPtr->sendTone(0.1, 1500., 0, true);
  synthesPtr->sendTone(0.3, 1900., 0, true);
  synthesPtr->sendTone(0.01, 1200., 0, true);
  synthesPtr->sendTone(0.3, 1900., 0, true);
}

void sstvTx::sendVIS() {
  int i, l;
  int t = txSSTVParam.VISCode;
  addToLog("txFunc:sendVis", LOGTXFUNC);
  if (currentMode->isNarrow()) {
    l = 24;
    synthesPtr->sendTone(0.300, 1900, 0, true);
    synthesPtr->sendTone(0.100, 2100, 0, true);
    synthesPtr->sendTone(0.022, 1900, 0, true); // startbit
    for (i = 0; i < l; i++) {
      if ((t & 1) == 1)
        synthesPtr->sendTone(0.022, 1900, 0, true);
      else
        synthesPtr->sendTone(0.022, 2100, 0, true);
      t >>= 1;
    }
  } else {
    if ((t & 0xFF) == 0x23)
      l = 16;
    else
      l = 8;
    synthesPtr->sendTone(0.030, 1200, 0, true); // startbit
    for (i = 0; i < l; i++) {
      if ((t & 1) == 1)
        synthesPtr->sendTone(0.030, 1100, 0, true);
      else
        synthesPtr->sendTone(0.030, 1300, 0, true);
      t >>= 1;
    }
    synthesPtr->sendTone(0.030, 1200, 0, true); // stopbit
  }
}

bool sstvTx::create(esstvMode m, DSPFLOAT clock) {
  if ((oldMode == m) && (currentMode != NULL)) {
    currentMode->init(clock);
    return true;
  }
  oldMode = m;
  if (currentMode)
    delete currentMode;
  currentMode = 0;
  switch (m) {
  case M1:
  case M2:
    currentMode = new modeGBR(m, TXSTRIPE, true, false);
    break;
  case S1:
  case S2:
  case SDX:
    currentMode = new modeGBR2(m, TXSTRIPE, true, false);
    break;
  case R36:
    currentMode = new modeRobot1(m, TXSTRIPE, true, false);
    break;
  case R24:
  case R72:
  case MR73:
  case MR90:
  case MR115:
  case MR140:
  case MR175:
  case ML180:
  case ML240:
  case ML280:
  case ML320:
    currentMode = new modeRobot2(m, TXSTRIPE, true, false);
    break;
  case SC2_60:
  case SC2_120:
  case SC2_180:
  case P3:
  case P5:
  case P7:
  case MC110N:
  case MC140N:
  case MC180N:
    currentMode = new modeRGB(m, TXSTRIPE, true, false);
    break;
  case FAX480:
  case BW8:
  case BW12:
    currentMode = new modeBW(m, TXSTRIPE, true, false);
    break;
  case AVT24:
  case AVT90:
  case AVT94:
    currentMode = new modeAVT(m, TXSTRIPE, true, false);
    break;
  case PD50:
  case PD90:
  case PD120:
  case PD160:
  case PD180:
  case PD240:
  case PD290:
  case MP73:
  case MP115:
  case MP140:
  case MP175:
    currentMode = new modePD(m, TXSTRIPE, true, false);
    break;

  case MP73N:
  case MP110N:
  case MP140N:
    currentMode = new modePD(m, TXSTRIPE, true, true);
    break;
  default:
    m = NOTVALID;
    break;
  }
  if (m != NOTVALID) {
    initializeSSTVParametersIndex(m, true);
    QString s = getSSTVModeNameLong(m);
    addToLog("create: create TX mode", LOGTXFUNC);
    if (synthesPtr) {
      synthesPtr->setTxClock(clock);
    }
    currentMode->init(clock);
    return true;
  }
  return false;
}

double sstvTx::FSKIDTime() {
  double tim;
  double charTime = 0.022 * 6;
  tim = (myCallsign.size() + 3) * charTime;
  tim += 0.422;
  return tim;
}

double sstvTx::calcTxTime(int overheadTime) {
  double tim = 0;
  //  tim= soundIOPtr->getPlaybackStartupTime();
  tim += SILENCEDELAY;
  initializeSSTVParametersIndex(sstvModeIndexTx, true);
  int t = txSSTVParam.VISCode;
  tim += 1.41; // preamble;
  if ((t & 0xFF) == 0x23)
    tim += 18. * 0.03;
  else
    tim += 10. * 0.03;
  tim += txSSTVParam.imageTime;

  //  if(enableCW)
  if (useCW) {
    tim += 0.5; // CW silence gap
    initCW(cwText);
    tim += getCWDuration();
    tim += 0.3; // trailer;
  } else {
    tim += FSKIDTime();
  }
  tim += overheadTime;
  return tim;
}

bool sstvTx::sendImage(const QImage &img) {
  modeBase::eModeBase mb;
  if (useVOX)
    synthesPtr->sendTone(1., 1700., 0, false);

  // Reset synthesizer state to prevent phase discontinuity spikes
  if (synthesPtr) {
    synthesPtr->reset();
  }

  if (txSSTVParam.mode == FAX480) {
    for (int i = 0; i < 1220; i++) {
      synthesPtr->sendTone(0.00205, 1500, 0, true);
      synthesPtr->sendTone(0.00205, 2300, 0, true);
    }
  } else {
    sendPreamble();
    sendVIS();
  }
  addToLog("txFunc: sendImage", LOGTXFUNC);
  qDebug() << "sstvTx::sendImage: Calling transmitImage";

  if (img.isNull()) {
    qDebug() << "sstvTx::sendImage: Image is null! Aborting.";
    return false;
  }

  if (!currentMode) {
    qDebug() << "sstvTx::sendImage: currentMode is null! Aborting.";
    return false;
  }

  mb = currentMode->transmitImage(img);
  qDebug() << "sstvTx::sendImage: transmitImage returned" << (int)mb;

  if (mb == modeBase::MBABORTED)
    return false;

  if (useCW) {
    // CW is generated inline here, while the audio session is still PBRUNNING.
    // soundBase.run() never leaves PBRUNNING automatically (we removed the
    // auto-stop), so isPlaying() returns true and synthesizer::write() will
    // patiently wait for buffer space rather than bailing.
    QString expandedCWText = cwText;
    expandedCWText.replace("%m", myCallsign, Qt::CaseInsensitive);
    qDebug() << "CW ID: Sending:" << expandedCWText;
    synthesPtr->sendSilence(0.5); // 500ms gap between SSTV and CW
    initCW(expandedCWText);
    float tone, duration;
    while (sendTextCW(tone, duration)) {
      if (tone > 0)
        synthesPtr->sendTone(duration, tone, 0, true);
      else
        synthesPtr->sendSilence(duration);
    }
    qDebug() << "CW ID: Done, draining buffer.";
  }

  // Drain everything (SSTV tail + CW) — PBRUNNING stays alive until MainWindow
  // calls idleTX(), so this loop will run to completion.
  int loopCount = 0;
  while (soundIOPtr->txBuffer.count() > 0) {
    QThread::msleep(10);
    loopCount++;
    if (loopCount % 100 == 0) {
      qDebug() << "sstvTx::sendImage: Draining buffer... count="
               << soundIOPtr->txBuffer.count();
    }
    if (aborted())
      return false;
    if (loopCount > 2000) {
      qWarning() << "sstvTx::sendImage: Drain timeout!";
      break;
    }
  }

  return true;
}

// sendCWOnly is no longer used - CW is now generated inline in sendImage().

// applyTemplate removed

void sstvTx::abort() {
  if (currentMode) {
    currentMode->abort();
  }
}

bool sstvTx::aborted() {
  if (currentMode)
    return currentMode->aborted();
  else
    return true;
}

int sstvTx::getPercentComplete() {
  if (currentMode)
    return currentMode->getPercentComplete();
  return 0;
}

void sstvTx::createTestPattern(QImage *img, etpSelect sel) {
  int i, j;
  QRgb *pixelPtr;
  int nb = txSSTVParam.numberOfPixels;
  int nl = txSSTVParam.numberOfDisplayLines;

  if (img) {
    if (img->size() != QSize(nb, nl)) {
      *img = QImage(nb, nl, QImage::Format_RGB32);
    }
    img->fill(imageBackGroundColor);
  } else {
    return;
  }

  // ivPtr->createImage(QSize(nb,nl),imageBackGroundColor,false);
  switch (sel) {
  case TPRASTER:
    for (i = 0; i < nl; i++) {
      pixelPtr = (QRgb *)img->scanLine(i);
      if (i < 2) {
        int val = 0;
        ;
        for (j = 0; j < nb; j++)
          pixelPtr[j] = qRgb(val, val, val);
      } else if (i >= (nl - 2)) {
        {
          int val = 0;
          ;
          for (j = 0; j < nb; j++)
            pixelPtr[j] = qRgb(val, val, val);
        }
      }

      else {
        for (j = 0; j < nb / 4; j++) {
          int val = (j % 2) * 255;
          pixelPtr[j] = qRgb(val, val, val);
        }
        for (; j < nb / 2; j++) {
          int val = ((j / 2) % 2) * 255;
          pixelPtr[j] = qRgb(val, val, val);
        }
        for (; j < 3 * nb / 4; j++) {
          int val = 0;
          pixelPtr[j] = qRgb(val, val, val);
        }
        for (; j < nb; j++) {
          int val = 255;
          pixelPtr[j] = qRgb(val, val, val);
        }
      }
    }
    break;
  case TPWHITE:
    for (i = 0; i < nl; i++) {
      pixelPtr = (QRgb *)img->scanLine(i);
      for (j = 0; j < nb; j++) {
        int val = 255;
        pixelPtr[j] = qRgb(val, val, val);
      }
    }
    break;
  case TPBLACK:
    for (i = 0; i < nl; i++) {
      pixelPtr = (QRgb *)img->scanLine(i);
      for (j = 0; j < nb; j++) {
        int val = 0;
        pixelPtr[j] = qRgb(val, val, val);
      }
    }
    break;
  case TPGRAY:
    for (i = 0; i < nl; i++) {
      pixelPtr = (QRgb *)img->scanLine(i);
      for (j = 0; j < nb; j++) {
        int val = 128;
        pixelPtr[j] = qRgb(val, val, val);
      }
    }
    break;
  }
  // ivPtr->setValidImage(true);
  // ivPtr->displayImage();
}

bool sstvTx::generateToWav(const QImage &img, esstvMode mode,
                           DSPFLOAT /*clock*/, const QString &wavPath) {
  if (img.isNull())
    return false;

  // Deep Decode requires 12000Hz Mono.
  // We force SAMPLERATE (12000) here to ensure compatibility.
  if (!create(mode, SAMPLERATE))
    return false;

  wavIO wavFile(SAMPLERATE);
  if (!wavFile.openFileForWrite(wavPath, false, false)) { // false = Mono
    return false;
  }

  if (synthesPtr) {
    synthesPtr->setWavOutput(&wavFile);
    synthesPtr->reset();
  }

  // Write silence/preamble/VIS
  sendPreamble();
  sendVIS();

  // Transmit image
  currentMode->transmitImage(img);

  // CW ID if enabled
  if (useCW) {
    QString expandedCWText = cwText;
    expandedCWText.replace("%m", myCallsign, Qt::CaseInsensitive);
    synthesPtr->sendSilence(0.5);
    initCW(expandedCWText);
    float tone, duration;
    while (sendTextCW(tone, duration)) {
      if (tone > 0)
        synthesPtr->sendTone(duration, tone, 0, true);
      else
        synthesPtr->sendSilence(duration);
    }
  }

  if (synthesPtr) {
    synthesPtr->setWavOutput(nullptr);
  }

  wavFile.closeFile();
  return true;
}
