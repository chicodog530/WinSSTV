#ifndef SSTVTX_H
#define SSTVTX_H
#include "appdefs.h"
#include "sstvparam.h"
#include "testpatternselection.h"
#include <QImage>
#include <QObject>
#include <QString>

class modeBase;
class imageViewer;

class sstvTx {
public:
  sstvTx();
  ~sstvTx();
  void init();
  double calcTxTime(int overheadTime);
  bool sendImage(const QImage &img);
  void abort();
  bool aborted();
  // void applyTemplate(QString templateFilename, bool useTemplate, imageViewer
  // *ivPtr); // Removed: GUI logic
  bool create(esstvMode m, DSPFLOAT clock);
  void createTestPattern(QImage *img, etpSelect sel);
  int getPercentComplete();
  bool generateToWav(const QImage &img, esstvMode mode, DSPFLOAT clock,
                     const QString &wavPath);

private:
  modeBase *currentMode;
  void sendPreamble();
  void sendVIS();
  esstvMode oldMode;
  unsigned long sampleCounter;
  double FSKIDTime();
};

#endif // SSTVTX_H
