#pragma once

#include "globals.h"

/** This is a very simple class to hold data about a charge field. */
class charge {
private:
  ndarray<double, 1> center_;
  double r0_;
  int m_;
  int s_;

public:
  charge() {}
  
  charge(double c1, double c2, double r, int M, int S) :
    center_(RECTDOMAIN((1), (3))), r0_(r), m_(M), s_(S) {
    center_[1] = c1;
    center_[2] = c2;
  }

  ndarray<double, 1> center() {
    return center_;
  }

  double r0() {
    return r0_;
  }

  int m() {
    return m_;
  }

  int s() {
    return s_;
  }
};
