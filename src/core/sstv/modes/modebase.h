#ifndef MODEBASE_H
#define MODEBASE_H
#include "../sstvparam.h"
#include "appdefs.h"
#include <QImage>
#include <QObject>
#include <QString>
#include <math.h>

#define MODESTATESCALER 10

#define stHUNT (0 * MODESTATESCALER)
#define stColorLine0 (10 * MODESTATESCALER)
#define stColorLine1 (20 * MODESTATESCALER)
#define stColorLine2 (30 * MODESTATESCALER)
#define stColorLine3 (40 * MODESTATESCALER)
#define stColorLine4 (50 * MODESTATESCALER)
#define stG1 (60 * MODESTATESCALER)
#define stG1a (65 * MODESTATESCALER)
#define stG2 (70 * MODESTATESCALER)
#define stG2a (75 * MODESTATESCALER)
#define stSync (80 * MODESTATESCALER)
#define stWaitVIS (90 * MODESTATESCALER)
#define stSTART (100 * MODESTATESCALER)
#define stWaitSync (110 * MODESTATESCALER)
#define stFP (120 * MODESTATESCALER)
#define stBP (130 * MODESTATESCALER)
#define stRepTone (140 * MODESTATESCALER)
#define st1900B (150 * MODESTATESCALER)
#define st1900E (160 * MODESTATESCALER)
#define stWAIT (170 * MODESTATESCALER)
#define stHALF (180 * MODESTATESCALER)
#define stFULL (190 * MODESTATESCALER)
#define stBITS (200 * MODESTATESCALER)

#define AVGFRQOFFSET 20

class modeBase {
public:
  enum embState {
    MBERROR,
    MBSETUPLINE,
    MBPIXELS,
    MBSYNC,
    MBENDOFLINE,
    MBEOIMAGE,
    MBRXWAIT,
    MB1500,
    MB2300,
    MBTXGAP,
    MBTXGAPROBOT
  };
  enum eModeBase { MBRUNNING, MBENDOFIMAGE, MBABORTED };
  modeBase(esstvMode m, unsigned int len, bool tx, bool narrowMode);
  virtual ~modeBase();
  /*!
    \brief initialize mode specific items
    This function is called by modeInit(), and can overwrite some or all of the
    local parameters. At least it should set the visible length.
    \param[in] clock  can be the rxClock or the txClock
*/
  virtual void setupParams(double clock) = 0;

  virtual bool getPixels();
  virtual unsigned long adjustSyncPosition(unsigned long syncPos0,
                                           bool isRetrace) {
    if (isRetrace)
      return syncPos0;
    else
      return (syncPos0 + 10);
  }
  void redrawFast(bool r);
  virtual eModeBase process(quint16 *demod, unsigned int syncPos, bool goToSync,
                            unsigned int rxPos);
  void init(DSPFLOAT clk);
  unsigned int *debugStatePtr;
  void abort();
  esstvMode getMode() { return mode; }
  eModeBase transmitImage(const QImage &img);
  void setRxTargetImage(QImage *img) { rxImage = img; }
  void setRxSampleCounter(int sc) { rxSampleCounter = sc; }
  void saveImage();
  int receivedLines() { return displayLineCounter; }
  int imageLines() { return activeSSTVParam->numberOfDisplayLines; }
  int imagePixels() { return activeSSTVParam->numberOfPixels; }
  bool aborted() { return !abortRun; }
  bool isNarrow() { return narrow; }
  int getPercentComplete() {
    if (activeSSTVParam && activeSSTVParam->numberOfDataLines > 0)
      return (lineCounter * 100) / activeSSTVParam->numberOfDataLines;
    return 0;
  }

protected:
  DSPFLOAT visibleLineLength;
  esstvMode mode;
  bool transmit;
  bool fastRedraw;
  DSPFLOAT fp;
  DSPFLOAT bp;
  DSPFLOAT blank;
  DSPFLOAT syncDuration;
  DSPFLOAT localClock;

  quint16 sample;
  DSPFLOAT start;
  sSSTVParam *activeSSTVParam;

  unsigned int marker;
  DSPFLOAT markerFloat;
  unsigned int syncPosition;
  unsigned int syncEndPosition;
  unsigned int lineCounter;
  unsigned int displayLineCounter;
  unsigned int pixelCounter;
  unsigned int sampleCounter;
  embState state;
  unsigned int subLine;
  unsigned int length;

  unsigned char *greenArrayPtr;
  unsigned char *blueArrayPtr;
  unsigned char *redArrayPtr;
  unsigned char *yArrayPtr;
  unsigned char *pixelArrayPtr;
  unsigned int *pixelPositionTable;
  unsigned int debugState;
  uint avgFreqGap;
  uint avgFreqGapCounter;

  DSPFLOAT txFreq;
  unsigned int txDur;
  void deleteBuffers();
  virtual void showLine();
  void combineColors();
  void yuvConversion(unsigned char *array);
  void grayConversion();
  /*!
      \brief setup the rx mode timing for one subline  at a time.

      Each line is subdivided in subLines. A subLine can be a pixel line, a
     delay or a sync. This function is called from the receive function in the
     modebase.
  */
  virtual embState rxSetupLine() = 0;

  /**
        \brief setup the tx mode timing for one subline  at a time.

        Each line is subdivided in subLines. A subLine can be a pixel line, a
     delay or a sync. This function is called from transmit function in the
     modebase.
    */
  virtual embState txSetupLine() = 0;

  virtual void getLine();
  void getLineY(bool evenodd);
  void getLineBW();
  int rxSampleCounter;
  DSPFLOAT syncFreq;
  DSPFLOAT lowerFreq;

private:
  void switchState(embState newState);
  void sendPixelBuffer();
  bool abortRun;
  bool isRunning;
  const QImage *txImPtr; // Source for transmission
  QImage *rxImage;       // Target for reception
  int avgSample;
  int avgSampleCounter;
  bool narrow;
  uint avgOddEvenFreq;
};

#endif
