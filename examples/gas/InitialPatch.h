#pragma once

#include "Quantities.h"
#include "Util.h"

class InitialPatch {
public:
  virtual void initData(rectdomain<SPACE_DIM> D,
                        ndarray<Quantities, SPACE_DIM, global> &U,
                        double dx) = 0;

};
