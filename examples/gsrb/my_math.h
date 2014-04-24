#pragma once

#include "globals.h"

class my_math {
public:
  my_math() {}

  static void resid(ndarray<double, 2> resid, ndarray<double, 2> phi,
                    ndarray<double, 2> rhs, double hsquared) {
    foreach (p, resid.domain()*rhs.domain()*phi.domain().shrink(1)) {
      resid[p] = (4.0*phi[p] - (phi[p + POINT(1, 0)]
				+ phi[p + POINT(-1, 0)]
				+ phi[p + POINT(0, 1)]
				+ phi[p + POINT(0, -1)]))/(hsquared) +
        rhs[p];
    }
  }

  static double sumsquared(ndarray<double, 2> a) {
    double sum = 0.;
    foreach (p, a.domain()) {
      sum += a[p]*a[p];
    }
    return sum;
  }
};
