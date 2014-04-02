#pragma once

#include "Util.h"
#include "Quantities.h"

class PhysBC {
public:
  /**
   * Calculate flux on physical boundary.
   * @param q (cell) primitive variables
   * @param coeffs (edge) artificial viscosity coefficients
   * @param sdim signed dimension
   * @param flux (edge) flux, output
   */
  virtual void f(ndarray<Quantities, SPACE_DIM> q,
                 ndarray<double, SPACE_DIM, global> coeffs,
                 int sdim,
                 ndarray<Quantities, SPACE_DIM> flux,
                 double dx) = 0;

  virtual void slope(ndarray<Quantities, SPACE_DIM> q,
                     const domain<SPACE_DIM> &D,
                     int sdim,
                     ndarray<Quantities, SPACE_DIM> dq,
                     double dx) = 0;
};
