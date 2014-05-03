#pragma once

#include "globals.h"

class nodal_infdmn {
private:
  nodal_infdmn() {}

public:
  static void solve(ndarray<double, 2> rhs, ndarray<double, 2> soln,
                    double h, int report_level);

  static void solve95(ndarray<double, 2> rhs, ndarray<double, 2> soln,
                      double h);

  /* This is the same, but it calls a five-point multigrid method and has
     matching reduced accuracy elsewhere. */
  static void solve5(ndarray<double, 2> rhs, ndarray<double, 2> soln,
                     double h);

  static void solve59(ndarray<double, 2> rhs, ndarray<double, 2> soln,
                      double h);
};
