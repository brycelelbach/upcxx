#pragma once

#include "globals.h"

class useful_array {
private:
  ndarray<double, 2, global> _data;
  int _processor;

public:
  useful_array() {}

  useful_array(RectDomain<2> region, int p) :
    _data(region), _processor(p) {}

  inline double op[](Point<2> p) {
    return _data[p];
  }
  
  inline int owner() {
    return _processor;
  }

  inline RectDomain<2> domain() {
    return _data.domain();
  }
};
