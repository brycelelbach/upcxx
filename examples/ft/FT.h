#pragma once

#include "globals.h"

// indices in myTimes- first index:
#define T_X_FFT 1
#define T_Y_FOR_FFT 2
#define T_Y_REV_FFT 3
#define T_Z_FFT 4
#define T_GLOBAL_ARRAYCOPY 5
#define T_BARRIER 6
#define T_SYNC 7

/**
 * This class has the 3D Fourier Transform operator.
 * <p>
 * There are several potential optimizations (listed in "globals.h")
 * for parallel FT:
 * <p>
 * Modify the SLABS flag.  Enabling this flag allows for
 * communication of slabs during the all-to-all transpose.
 * Each slab consists of all the data that needs to be sent
 * to one specific processor.  If turned off, pencil-shaped
 * pieces of the array will be sent instead, so a message
 * will be sent after each row in the array is completed.
 * This allows for finer-grained communication.
 * <p>
 * Change the PADDING flag.  This flag indicates how many
 * doubles are added to the unit-stride dimension of each
 * array so as to avoid cache-thrashing.
 * <p>
 * Turn on NONBLOCKING_ARRAYCOPY.  If enabled, this flag
 * allows for communication/computation overlap.  If it is
 * turned off, blocking communication will be used instead.
 */
class FT {
 public:
  // same as in FTDriver
  ndarray<ndarray<Complex, 3, global GUNSTRIDED>, 1> array1, array2;
  ndarray<ndarray<Complex, 3, global GUNSTRIDED>, 1> array3, array4;
  ndarray<ndarray<Complex, 3, global GUNSTRIDED>, 1> array5, array6;
  ndarray<Complex, 3 UNSTRIDED> myArray1, myArray2, myArray3;
  ndarray<Complex, 3 UNSTRIDED> myArray4, myArray5, myArray6;

  ndarray<Complex, 1 UNSTRIDED> temp;    // used for in-place transforms

  // profiling
#ifdef TIMERS_ENABLED
  timer myTimer, myTimer2;
#endif

  // "myTimes" is indexed by (FT Component #) and then timing number
  ndarray<ndarray<double, 1 UNSTRIDED>, 1 UNSTRIDED> myTimes;

 private:
  // the following are copied from FTDriver
  int nx, ny, nz;
  int numIterations;
  int maxdim;

  /* method declarations for interfacing with FFTW */
  static void createPlans(ndarray<Complex, 3> myArray1,
                          ndarray<Complex, 3> myArray2,
                          ndarray<Complex, 3> myArray3,
                          ndarray<Complex, 3> myArray4,
                          ndarray<Complex, 3> myArray5,
                          ndarray<Complex, 3> myArray6,
                          int nx, int ny, int nz,
                          int yDimSlabSize, int zDimSlabSize);

#ifdef SLABS
  static void slabToUnitStrideForwardX(ndarray<Complex, 2> inArray,
                                       ndarray<Complex, 2> outArray);
  static void slabToUnitStrideBackwardX(ndarray<Complex, 2> inArray,
                                        ndarray<Complex, 2> outArray);
#else
  static void pencilToUnitStrideForwardX(ndarray<Complex, 2> inArray,
                                         ndarray<Complex, 2> outArray);
  static void pencilToUnitStrideBackwardX(ndarray<Complex, 2> inArray,
                                          ndarray<Complex, 2> outArray);
#endif
  static void pencilToPencilStrideForwardY(ndarray<Complex, 2> inArray,
                                           ndarray<Complex, 1> outArray);
  static void pencilToPencilStrideBackwardY(ndarray<Complex, 2> inArray,
                                            ndarray<Complex, 2> outArray);
  static void unitToUnitStrideForwardZ(ndarray<Complex, 1> inArray,
                                       ndarray<Complex, 1> outArray);
  static void unitToUnitStrideBackwardZ(ndarray<Complex, 1> inArray,
                                        ndarray<Complex, 1> outArray);

 public:
  /**
   * Constructor- constructs local arrays and initializes timers and
   * counters
   */
  FT(ndarray<ndarray<Complex, 3, global GUNSTRIDED>, 1> p_array1,
     ndarray<ndarray<Complex, 3, global GUNSTRIDED>, 1> p_array2,
     ndarray<ndarray<Complex, 3, global GUNSTRIDED>, 1> p_array3,
     ndarray<ndarray<Complex, 3, global GUNSTRIDED>, 1> p_array4,
     ndarray<ndarray<Complex, 3, global GUNSTRIDED>, 1> p_array5,
     ndarray<ndarray<Complex, 3, global GUNSTRIDED>, 1> p_array6);

  /**
   * Create FFT plans (for FFTW)
   */
  void create_FFT_plans() {
    createPlans(myArray1, myArray2, myArray3, myArray4, myArray5,
                myArray6, nx, ny, nz, ny/THREADS, nz/THREADS);
  }

  /**
   * Performs a full 3D FFT
   *
   * @param direction Indicates forward (+1) or backward (-1) 3D FFT
   * @param iterationNum The iteration number (used for profiling)
   */
  void FFT_3D(int direction, int iter);

  /**
   * Prints a performance profiling summary.
   */
  void printSummary();

  /**
   * Prints a full performance profile.
   */
  void printProfile();
};
