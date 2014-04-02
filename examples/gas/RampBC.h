#pragma once

#include "PhysBC.h"
#include "Ramp.h"

/**
 * Ramp problem:  extends Ramp
 *
 * Left side:  flux[e] = q1flux[LENGTH_DIM]
 * Right side:  flux[e] = q0flux[LENGTH_DIM]
 * Bottom side:  flux[e] = flux from free boundary, if e[LENGTH_DIM] <= maxOffWall
 *                         flux from solid boundary, if e[LENGTH_DIM] > maxOffWall
 * Top side:  flux[e] = q1flux[HEIGHT_DIM], if e[LENGTH_DIM] * dx <= sloc
 *                      q0flux[HEIGHT_DIM], if e[LENGTH_DIM] * dx > sloc
 * sloc = shockLocTop + shockSpeed * time
 * maxOffWall = shockLocBottom / dx
 *
 * need LevelGodunov object to get fluxes and to convert quantities
 *
 * @version  27 Jan 2000
 * @author   Peter McCorquodale, PWMcCorquodale@lbl.gov
 */

class RampBC : public Ramp, public PhysBC {
public:
  /**
   * Constructor.
   * If you write it so that there is a different RampBC object
   * for each level, then you can set lg, dx, maxOffWall here.
   */
  RampBC() : Ramp() {}

  virtual void f(ndarray<Quantities, SPACE_DIM> state,
                 ndarray<double, SPACE_DIM, global> difCoefs,
                 int sdim,
                 ndarray<Quantities, SPACE_DIM> flux,
                 double dx);

  virtual void slope(ndarray<Quantities, SPACE_DIM> q,
                     const domain<SPACE_DIM> &domainSide,
                     int sdim,
                     ndarray<Quantities, SPACE_DIM> dq,
                     double dx);

  void slope1(ndarray<Quantities, SPACE_DIM> q,
              const domain<SPACE_DIM> &domainSide,
              int sdim,
              ndarray<Quantities, SPACE_DIM> dq);


  void slopeWall(ndarray<Quantities, SPACE_DIM> q,
                 const domain<SPACE_DIM> &domainSide,
                 int sdim,
                 ndarray<Quantities, SPACE_DIM> dq);
};
