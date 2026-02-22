#ifndef SYNCPROCESSOR_H
#define SYNCPROCESSOR_H

#include "../dsp/filters.h"
#include "visfskid.h"
#include <QList>
#include <QObject>
#include <sstvparam.h>

#define MAXSYNCENTRIES 2048
#define STATESCALER 100

class modeBase;

struct sSensitivity {
  unsigned int minMatchedLines;
  unsigned int maxLineDistanceModeDetect;
  unsigned int maxLineDistanceInSync;
  DSPFLOAT onRatio;
  DSPFLOAT offRatio;
  int minVolume;
  int maxTempOutOfSyncLines;
  int maxOutOfSyncLines;
};

struct ssyncEntry {
  ssyncEntry() { init(); }

  void init() {
    start = 0;
    end = 0;
    startVolume = 0;
    maxVolume = 0;
    width = 0;
    inUse = false;
    retrace = false;
    lineNumber = 0;
    length = 0;
  }
  int diffStartEnd() {
    width = end - start;
    return width;
  }
  unsigned int start;
  unsigned int startVolume;
  unsigned int end;
  unsigned int maxVolume;
  unsigned int width;
  bool inUse;
  bool retrace;
  unsigned int lineNumber;
  unsigned int length;
};

struct smatchEntry {
  smatchEntry() { init(); }

  smatchEntry(unsigned int fromIdx, unsigned int toIdx, unsigned int lineSpace,
              double fract, unsigned int endFromSample,
              unsigned int endToSample) {
    from = fromIdx;
    to = toIdx;
    lineSpacing = lineSpace;
    fraction = fract;
    endFrom = endFromSample;
    endTo = endToSample;
  }
  void init() {
    from = 0;
    to = 0;
    lineSpacing = 0;
    fraction = 0;
    endFrom = 0;
    endTo = 0;
  }
  unsigned int from; /**< the from index pointing to the syncArray */
  unsigned int to;   /**< the to index pointing to  the syncArray */
  unsigned int lineSpacing;
  double fraction;
  unsigned int endFrom; /**< sampleCounter From */
  unsigned int endTo;   /**< sampleCounter To */
};

struct sslantXY {
  DSPFLOAT x;
  DSPFLOAT y;
};

typedef QList<smatchEntry *> modeMatchList;
typedef QList<modeMatchList *> modeMatchChain;

class syncProcessor : public QObject {
  Q_OBJECT
public:
  //  enum esyncState
  //  {SYNCOFF,SYNCUP,SYNCSTART,SYNCON,SYNCDOWN,SYNCEND,SYNCVALID};
  enum esyncState { SYNCOFF, SYNCACTIVE, SYNCVALID };
  enum esyncProcessState {
    MODEDETECT,
    INSYNC,
    SYNCLOSTNEWMODE,
    SYNCLOSTFALSESYNC,
    SYNCLOSTMISSINGLINES,
    SYNCLOST,
    RETRACEWAIT
  };
  explicit syncProcessor(bool narrow, QObject *parent = 0);
  ~syncProcessor();
  void init(bool preserveImage = false);
  void reset(bool preserveImage = false);
  void process();
  esyncProcessState getSyncState(quint32 &syncPos) {
    syncPos = syncPosition;
    return syncProcesState;
  }
  esstvMode getMode() { return currentMode; }
  void resetRetraceFlag();
  bool hasNewClock() {
    bool nc = newClock;
    newClock = false;
    return nc;
  }
  void clear();
  void recalculateMatchArray();
  DSPFLOAT getNewClock() { return modifiedClock; }
  void setSyncDetectionEnabled(bool enable) { enableSyncDetection = enable; }
  void setModeRange(esstvMode start, esstvMode end);

  quint32 sampleCounter;
  quint32 syncPosition;
  quint32 lastValidSyncCounter;
  //  DSPFLOAT trackMax;
  int syncQuality;
  modeBase *currentModePtr;

  quint16 *freqPtr;
  FILTERPARAMTYPE *syncVolumePtr;
  FILTERPARAMTYPE *inputVolumePtr;
  FILTERPARAMTYPE *energy1100Ptr;
  FILTERPARAMTYPE *energy1300Ptr;
  FILTERPARAMTYPE *energy1200Ptr;
  FILTERPARAMTYPE *energy1900Ptr;
  videoFilter *videoFilterPtr;
  bool retraceFlag;
  bool tempOutOfSync;
  void setRxTargetImage(QImage *img) { rxTargetImage = img; }

#ifndef QT_NO_DEBUG
  void setOffset(unsigned int dataScopeOffset);
  unsigned int xOffset;
  unsigned int syncStateBuffer[RXSTRIPE];
#endif

public slots:
  void slotNewCall(QString call);
  void slotVisCodeDetected(int, unsigned int visSampleCounter);

signals:
  void callReceived(QString);

private:
  quint32 maxLineSamples;
  quint16 syncArrayIndex;
  ssyncEntry syncArray[MAXSYNCENTRIES];
  modeMatchChain matchArray[NOTVALID];
  quint16 slantAdjustLine;
  esstvMode currentMode;
  esyncState syncState;
  esyncProcessState syncProcesState;
  bool newClock;
  esstvMode idxStart;
  esstvMode idxEnd;
  esstvMode preferredMode; // For eager matching fallback
  DSPFLOAT modifiedClock;
  DSPFLOAT samplesPerLine;
  streamDecoder streamDecode;
  DSPFLOAT lineTolerance;
  modeMatchList *activeChainPtr;

  bool currentModeMatchChanged;
  unsigned int lastSyncTest;
  unsigned int lastUpdatedSync;
  sslantXY slantXYArray[MAXSYNCENTRIES];
  esstvMode visMode;

  void extractSync();
  bool validateSync();
  bool findMatch();
  bool addToMatch(esstvMode mode);
  bool addToChain(esstvMode mode, unsigned int fromIdx);
  bool lineCompare(DSPFLOAT samPerLine, int srcIdx, int dstIdx,
                   quint16 &lineNumber, double &fraction);
  void switchSyncState(esyncState newState, quint32 sampleCntr);
  void switchProcessState(esyncProcessState newState);
  unsigned int calcTotalLines(modeMatchList *mlPtr);
  double calcTotalFract(modeMatchList *mlPtr);
  void clearMatchArray();
  void removeMatchArrayChain(esstvMode mode, int chainIdx);
  void cleanupMatchArray();
  void dropTop();
  void deleteSyncArrayEntry(unsigned int entry);
  bool createModeBase();
  void checkSyncArray();
  void trackSyncs();
  void calcSyncQuality();
  void calculateLineNumber(unsigned int fromIdx, unsigned int toIdx);
  void regression(DSPFLOAT &a, DSPFLOAT &b, bool initial);
  bool slantAdjust(bool initial);
  DSPFLOAT syncWidth;

  // signal quality
  quint16 falseSlantSync;
  quint16 unmatchedSyncs;
  quint16 falseSyncs;
  quint16 avgLineSpacing;
  quint16 missingLines;

  bool detectNarrow;
  bool enableSyncDetection;
  unsigned int minMatchedLines;
  unsigned int visEndCounter;
  //  DSPFLOAT syncAvgPtr[RXSTRIPE];
  //  DSPFLOAT syncAvg;
  QImage *rxTargetImage = nullptr;
};

#endif // SYNCPROCESSOR_H
