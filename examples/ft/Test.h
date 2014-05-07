#pragma once

#include "globals.h"

// timer index for "myTimes" array and "myCounts" array:
#define T_EVOLVE 1
#define T_CHECKSUM 2

/**
 * This class sets the initial conditions, evolves the PDE in time,
 * and performs checksum verifications.
 */
class Test {
 private:
  ndarray<ndarray<double, 3, global GUNSTRIDED>, 1> expArray;    // used in the evolve() method

  // check sum array (only on proc 0)
  ndarray<Complex, 1 UNSTRIDED> checkSumArray;
  ndarray<Complex, 1 UNSTRIDED> localComplexesOnProc0;
  ndarray<Complex, 1, global GUNSTRIDED> myRemoteComplexesOnProc0;

  // the following are copied from FTDriver
  int nx, ny, nz;
  int numIterations;
  int maxdim;

 public:
  // profiling
#ifdef TIMERS_ENABLED
  timer myTimer;
  // "myTimes" is indexed by (Test Component #) and then timing number
  ndarray<ndarray<double, 1 UNSTRIDED>, 1 UNSTRIDED> myTimes;
#endif
#ifdef COUNTERS_ENABLED
  PAPICounter myCounter;
  // "myCounts" is indexed by (Test Component #) and then counting number
  ndarray<ndarray<long, 1 UNSTRIDED>, 1 UNSTRIDED> myCounts;
#endif

  /**
   * Constructor- performs many initializations
   *
   * @param array The array that expArray will do an element-wise multiply
   * with (so they need to be the same size)
   */
  Test(ndarray<ndarray<Complex, 3, global GUNSTRIDED>, 1> array);

  /**
   * Initializes "array" with random values
   */
  void initialConditions(ndarray<ndarray<Complex, 3, global GUNSTRIDED>, 1> array);

  /**
   * Initializes expArray, which is used in the evolve() method
   */
  void initializeExponentialArray();

  /**
   * Multiplies each element in "array" by corresponding expArray
   * element.  This does the time evolution of the PDE.
   *
   * @param array The array that will be advanced in time
   * @param iterationNum The iteration number (used for profiling)
   */
  void evolve(ndarray<ndarray<Complex, 3, global GUNSTRIDED>, 1> array,
              int iterationNum);

  /**
   * Prints a checkSum of "array"
   *
   * @param array The array for which the checksum is being calculated
   * @param iterationNum The iteration number
   */
  void checkSum(ndarray<ndarray<Complex, 3, global GUNSTRIDED>, 1> array,
                int iterationNum);

  /**
   * Verifies that the checksum is correct.
   *
   * @param classType The problem class (S, W, A, B, C, D)
   */
  void verifyCheckSum(char classType);

  /**
   * Prints a performance profiling summary.
   */
  void printSummary();

  /**
   * Prints a full performance profile.
   */
  void printProfile();
};
