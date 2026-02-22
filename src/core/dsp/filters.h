#ifndef FILTERS_H
#define FILTERS_H
#include "filter.h"
#include "resonantfilter.h"

class syncFilter {
public:
  syncFilter(uint maxLength);
  ~syncFilter();
  void process(FILTERPARAMTYPE *dataPtr);
  void init();
  FILTERPARAMTYPE *detect1200Ptr;
  FILTERPARAMTYPE *detect1900Ptr;
  FILTERPARAMTYPE *detect1100Ptr;
  FILTERPARAMTYPE *detect1300Ptr;

private:
  filter sync1200;
  filter sync1900;
  filter sync1200lp;
  filter sync1900lp;

  // Resonant filters for VIS decoding (Ported from MMSSTV logic)
  resonantFilter tank1100;
  resonantFilter tank1300;
  resonantFilter tank1200; // Also use resonant for sync detection
  resonantFilter tank1900; // Also use resonant for sync detection

  FILTERPARAMTYPE energy1100[RXSTRIPE];
  FILTERPARAMTYPE energy1300[RXSTRIPE];
  FILTERPARAMTYPE energy1200[RXSTRIPE];
  FILTERPARAMTYPE energy1900[RXSTRIPE];
};

class videoFilter {
public:
  videoFilter(uint maxLength);
  ~videoFilter();
  void process(FILTERPARAMTYPE *dataPtr);
  void init();
  quint16 *demodPtr;

private:
  filter videoFltr;
  filter lpFltr;
};

class wfFilter {
public:
  wfFilter(uint maxLength);
  ~wfFilter();
  void process(FILTERPARAMTYPE *dataPtr, uint dataLength = RXSTRIPE);
  void init();

private:
  filter wfFltr;
};

class drmHilbertFilter {
public:
  drmHilbertFilter(uint maxLength);
  ~drmHilbertFilter();
  //  void process(FILTERPARAMTYPE *dataPtr, uint =RXSTRIPE);
  void process(FILTERPARAMTYPE *dataPtr, float *outputPtr, uint dataLength);
  void init();

private:
  filter drmFltr;
};

#endif // FILTERS_H
