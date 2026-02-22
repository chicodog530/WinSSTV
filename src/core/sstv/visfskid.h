#ifndef VISFSKID_H
#define VISFSKID_H
#include "appglobal.h"
#include "sstvparam.h"
#include <QObject>

#define FSKMIN1500 ((SAMPLERATE * 100) / 1000)
#define FSKMIN2100 ((SAMPLERATE * 70) / 1000)
#define FSKMIN1900 ((SAMPLERATE * 18) / 1000)
#define FSKBIT ((SAMPLERATE * 22) / 1000)
#define FSKAVGCOUNT 25
#define VISAVGCOUNT 25
#define RETRACEAVGCOUNT 100

#define FREQAVG 0.05

#define VISMIN1900 ((SAMPLERATE * 200) / 1000)
#define VISMINWIDE1200 ((SAMPLERATE * 25) / 1000)
#define VISBITWIDE ((SAMPLERATE * 30) / 1000)

#define VISMINNARROW2100 ((SAMPLERATE * 80) / 1000)
#define VISBITNARROW ((SAMPLERATE * 22) / 1000)

#define MINRETRACEWIDTH ((SAMPLERATE * 290) / 1000)

class fskDecoder : public QObject {
  Q_OBJECT
public:
  fskDecoder();
  virtual void extract(unsigned int syncSampleCtr, bool narrow) = 0;
  void setDataPtr(DSPFLOAT *dataPtr) { freqPtr = dataPtr; }
  virtual void reset() = 0;

protected:
  unsigned char bitCounter;
  quint32 symbol;
  quint32 bit;

  unsigned char checksum;
  unsigned int code;
  unsigned int sampleCounter;
  unsigned int startSampleCounter;
  unsigned int syncSampleCounter;
  unsigned int timeoutCounter;
  bool validCode;
  DSPFLOAT *freqPtr;
  DSPFLOAT avgFreq;
  unsigned int avgCounter;
  unsigned int avgCount;
  unsigned int count1500;
  unsigned int count2100;
  unsigned int count1900;

  bool waitStartFreq(unsigned int freqL, unsigned int freqH);
  bool waitEndFreq(unsigned int freqL, unsigned int freqH);
  bool waitStartFreq(unsigned int freqL, unsigned int freqH,
                     unsigned long maxWait, bool &timeout);
  bool waitEndFreq(unsigned int freqL, unsigned int freqH,
                   unsigned long maxWait, bool &timeout);
  void init();
};

class fskIdDecoder : public fskDecoder {
  Q_OBJECT
public:
  enum efskState {
    FSKINIT,
    WAITSTART1500,
    WAITEND1500,
    WAITSTART1900,
    WAITEND1900,
    WAITSTART2100,
    WAITEND2100,
    GETID
  };
  fskIdDecoder();
  void extract(unsigned int syncSampleCtr, bool narrow);
  void reset();
  QString getFSKId();

signals:
  void callReceived(QString);

private:
  void switchState(efskState newState, unsigned int i);
  bool assemble(bool reset);
  unsigned int fskIDChar;
  QString fskStr;

  unsigned int fskAVGCounter;
  QString fskIDStr;
  bool headerFound;
  bool endFound;
  efskState fskState;
};

class visDecoder : public fskDecoder {
  Q_OBJECT
public:
  enum evisState {
    VISINIT,
    WAITSTART1200,
    WAITEND1200,
    WAITSTART1900,
    WAITEND1900,
    WAITSTART2100,
    WAITEND2100,
    WAITSTARTBIT,
    GETCODE
  };
  visDecoder();
  void extract(unsigned int syncSampleCtr, bool narrow);
  void extractNarrow();
  void extractWide();
  void reset();
  unsigned int getCode();
  void setEnergyPtrs(FILTERPARAMTYPE *e1100, FILTERPARAMTYPE *e1300) {
    energy1100Ptr = e1100;
    energy1300Ptr = e1300;
  }
  esstvMode mode;
signals:
  void visCodeNarrowDetected(int, unsigned int);
  void visCodeWideDetected(int, unsigned int);

private:
  void switchState(evisState newState, unsigned int i);
  evisState visState;
  FILTERPARAMTYPE *energy1100Ptr;
  FILTERPARAMTYPE *energy1300Ptr;
};

// class retraceDetector: public fskDecoder
//{
// public:
//   retraceDetector();
//   enum eretraceState  {RETRACEINIT,RETRACEWAITSTART,RETRACEWAITEND};
//   void extract(unsigned int syncSampleCtr);
//   void reset();
// private:
//   void switchState(eretraceState   newState, unsigned int i);
//   eretraceState  retraceState;
// };

class streamDecoder {
public:
  streamDecoder(bool narrow);
  void reset();
  void process(quint16 *freqPtr, FILTERPARAMTYPE *e1100, FILTERPARAMTYPE *e1300,
               unsigned int syncSampleCtr);
  unsigned int getVisCode() { return visCoder.getCode(); }
  fskIdDecoder *getFskDecoderPtr() { return &fskCoder; }
  visDecoder *getVisDecoderPtr() { return &visCoder; }

private:
  DSPFLOAT avgFreq;
  DSPFLOAT avgBuffer[RXSTRIPE];
  fskIdDecoder fskCoder;
  visDecoder visCoder;
  //  retraceDetector retracer;
  quint16 freqPtr();
  bool isNarrow;
};

#endif // VISFSKID_H
