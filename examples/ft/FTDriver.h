#pragma once

#include "globals.h"

/**
 * This class contains the main() method.
 */
class FTDriver {
 public:
  // distributed arrays
  ndarray<global_ndarray<Complex, 3>, 1> array1, array2, array3;
  ndarray<global_ndarray<Complex, 3>, 1> array4, array5, array6;

  // global variables
  char classType;
  static int nx, ny, nz;
  static int maxdim;
  static int numIterations;
  static int xDimSlabSize, yDimSlabSize, zDimSlabSize;

  // profiling information
#ifdef TIMERS_ENABLED
  timer myTotalTimer;
  timer mySetupTimer;
  double mySetupTime;
#endif

  /**
   * Constructor- performs many initializations
   */
  FTDriver(char paramClassType);

  void printSetupTiming(int i);

  /**
   * Only need to pass in NAS problem class (S, W, A, B, C, or D)
   */
  static void main (int argc, char **argv);
};
