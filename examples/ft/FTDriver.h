#pragma once

#include "globals.h"

/**
 * This class contains the main() method.
 */
class FTDriver {
 public:
  // distributed arrays
  ndarray<ndarray<Complex, 3, global GUNSTRIDED>, 1> array1, array2;
  ndarray<ndarray<Complex, 3, global GUNSTRIDED>, 1> array3, array4;
  ndarray<ndarray<Complex, 3, global GUNSTRIDED>, 1> array5, array6;

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
