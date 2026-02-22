#ifndef APPDEFS_H
#define APPDEFS_H
#include <complex>
#include <stdint.h>
#include <string>
#include <vector>
using std::abs;
using std::complex;
using std::string;
using std::vector;

#define SOUNDFRAME int32_t

#define BASESAMPLERATE 48000
#define SUBSAMPLINGFACTOR 4
#define MONOCHANNEL 1
#define STEREOCHANNEL 2
#define RXSTRIPE 1024
#define TXSTRIPE 1024
#define FILTERPARAMTYPE double
#define DOWNSAMPLESIZE (SUBSAMPLINGFACTOR * RXSTRIPE)
#define SAMPLERATE (BASESAMPLERATE / SUBSAMPLINGFACTOR)
#define SILENCEDELAY 0.2

#undef DISABLENARROW

typedef double DSPFLOAT;

/* Define the application specific data-types ------------------------------- */
typedef double _REAL;
typedef complex<_REAL> _COMPLEX;
typedef short _SAMPLE;
typedef unsigned char _BYTE;
typedef bool _BOOLEAN;
typedef unsigned char _BINARY;

#endif // APPDEFS_H
