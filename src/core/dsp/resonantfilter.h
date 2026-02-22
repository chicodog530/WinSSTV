#ifndef RESONANTFILTER_H
#define RESONANTFILTER_H

#include "appdefs.h"
#include <cmath>


#ifndef PI
#define PI 3.14159265358979323846
#endif

class resonantFilter {
private:
  double z1, z2;
  double a0;
  double b1, b2;

public:
  resonantFilter() {
    z1 = 0;
    z2 = 0;
    a0 = 0;
    b1 = 0;
    b2 = 0;
  }

  void setFreq(double f, double smp, double bw) {
    double lb1, lb2, la0;
    lb1 = 2 * std::exp(-PI * bw / smp) * std::cos(2 * PI * f / smp);
    lb2 = -std::exp(-2 * PI * bw / smp);
    if (bw > 0) {
      la0 = std::sin(2 * PI * f / smp) / ((smp / 6.0) / bw);
    } else {
      la0 = std::sin(2 * PI * f / smp);
    }
    b1 = lb1;
    b2 = lb2;
    a0 = la0;
  }

  double process(double d) {
    d *= a0;
    d += (z1 * b1);
    d += (z2 * b2);
    z2 = z1;
    if (std::abs(d) < 1e-37)
      d = 0.0;
    z1 = d;
    return d;
  }

  void reset() {
    z1 = 0;
    z2 = 0;
  }
};

#endif // RESONANTFILTER_H
