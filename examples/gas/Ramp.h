#pragma once

/**
 * Ramp contains the main program.
 *
 * @version  17 Dec 1999
 * @author   Peter McCorquodale, PWMcCorquodale@lbl.gov
 */

#include "globals.h"
#include "Amr.h"
#include "LevelGodunov.h"
#include "ParmParse.h"
#include "Util.h"

class Ramp {
public:
  /*
    constants
  */

  enum { DENSITY=0, VELOCITY_X=1, VELOCITY_Y=2, PRESSURE=3 };

  /** process used for broadcasts */
  enum { SPECIAL_PROC = 0 };

protected:
  /*
    fields may be used by RampBC, RampInit.
  */

  /** dimension along barrier, e.g. 1 */
  enum { LENGTH_DIM = 1 };

  /** dimension above barrier, e.g. 2 */
  enum { HEIGHT_DIM = 2 };

  /** shock location on the solid boundary */
  static double shockLocBottom;

  /** shock location on the free top */
  static double shockLocTop;

  /** speed of shock along the free top */
  static double shockSpeed;

  /** slope of the shock */
  static double slope;

  /** conserved variables on each side of the shock */
  static Quantities U0, U1;

  /** flux on each side of the shock */
  static ndarray<Quantities, 1> q0flux, q1flux;

  static Amr *amrAll;

  static ndarray<ndarray<double, 1>, 1> ends;

  static double gamma;

  static LevelGodunov *lg0;

public:
  static void main(int argc, char **args);

private:
  /**
   * Get shock data to be used by RampInit and RampBC.
   * For RampInit:  shockLocBottom, slope, U0, U1
   * For RampBC:  q0flux, q1flux, shockLocTop, shockSpeed
   * Need LevelGodunov lg0 for conversions and flux calculations.
   */
  static void getData(ParmParse &pp);

protected:
  /**
   * If q contains primitive variables, then
   * q[velocityIndex(dim)] is velocity in dimension dim.
   */
  static int velocityIndex(int dim) {
    return dim;
  }

  static int velocityIndex(Point<1> pdim) {
    return pdim[1];
  }
};
