#pragma once

#include "InitialPatch.h"
#include "Ramp.h"

class RampInit : public Ramp, public InitialPatch {

  enum { nsub = 10 };  // number of subintervals
  static double dnsubinv;
  static double rn2;

public:
  virtual void initData(rectdomain<SPACE_DIM> D,
                        ndarray<Quantities, SPACE_DIM, global> &U,
                        double dx);
};
