#pragma once

#include "globals.h"
#include "Grid.h"

/**
 * This class contains the main() method.
 */
class MGDriver {
 private:
  /* These grids store the residual grids the correction grids.  
     They are arrays to represent the various levels of multigrid.  */
  ndarray<Grid *, 1> residualGrids, correctionGrids;

  /* right-hand-side Grid object */
  Grid *rhsGrid;

  // used to verify correctness
  double verifyValue;
  const double epsilon;

 public:
  // global variables
  char classType;               // class type of multigrid
  static int startLevel;        // starting level of the grid
  static int numIterations;     // number of iterations

  // profiling tools
#ifdef TIMERS_ENABLED
  Timer myTotalTimer;
#endif
#ifdef COUNTERS_ENABLED
  PAPICounter myTotalCounter;
#endif

  /**
   * Constructor- performs many initializations
   */
  MGDriver(char paramClassType);

  /**
   * Only need to pass in NAS problem class (S, W, A, B, C, or D)
   */
  static void main(int argc, char **argv);
};
