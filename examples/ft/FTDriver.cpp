#include "FTDriver.h"
#include "Test.h"
#include "FT.h"

int FTDriver::nx;
int FTDriver::ny;
int FTDriver::nz;
int FTDriver::maxdim;
int FTDriver::numIterations;
int FTDriver::xDimSlabSize;
int FTDriver::yDimSlabSize;
int FTDriver::zDimSlabSize;

FTDriver::FTDriver(char paramClassType) {
  classType = paramClassType;

  // establish grid size and number of iterations
  // initializations of parameters based on class S
  // NOTE: THESE ARRAYS ARE THE TRANSPOSE OF THE FORTRAN ARRAYS,
  // SO [X,Y,Z] IS NOW [Z,Y,X]
  nx = ny = nz = 64;
  int localNumIterations = 6;
  switch (classType) {
  case 'W':
    nx = 32;
    ny = nz = 128;
    localNumIterations = 6;
    break;
  case 'A':
    nx = 128;
    ny = nz = 256;
    localNumIterations = 6;
    break;
  case 'B':
    nx = ny = 256;
    nz = 512;
    localNumIterations = 20;
    break;
  case 'C':
    nx = ny = nz = 512;
    localNumIterations = 20;
    break;
  case 'D':
    nx = ny = 1024;
    nz = 2048;
    localNumIterations = 25;
    break;
  }

  numIterations = broadcast(localNumIterations, 0);

  // determine maxdim
  if ((nx >= ny) && (nx >= nz)) {
    maxdim = nx;
  }
  else if (ny >= nz) {
    maxdim = ny;
  }
  else {
    maxdim = nz;
  }

  // determine partitioning slab sizes
  xDimSlabSize = nx/ranks();
  int xDimSlabStartRow = myrank() * xDimSlabSize;
  int xDimSlabEndRow = xDimSlabStartRow + xDimSlabSize;

  yDimSlabSize = ny/ranks();
  int yDimSlabStartRow = myrank() * yDimSlabSize;
  int yDimSlabEndRow = yDimSlabStartRow + yDimSlabSize;

  zDimSlabSize = nz/ranks();
  int zDimSlabStartRow = myrank() * zDimSlabSize;
  int zDimSlabEndRow = zDimSlabStartRow + zDimSlabSize;

  // padded working arrays (to avoid cache-thrashing)
  ndarray<Complex, 3 UNSTRIDED> myArray1(RECTDOMAIN((xDimSlabStartRow, 0, 0),
                                                    (xDimSlabEndRow, ny,
                                                     nz+PADDING)));
  array1 =
    ndarray<ndarray<Complex, 3, global GUNSTRIDED>, 1>(RECTDOMAIN((0),
                                                                  (ranks())));
  array1.exchange(myArray1);

#ifdef SLABS
  ndarray<Complex, 3 UNSTRIDED> myArray2(RECTDOMAIN((0, yDimSlabStartRow, 0),
                                                    (nx, yDimSlabEndRow,
                                                     nz+PADDING)));
#else
  ndarray<Complex, 3 UNSTRIDED> myArray2(RECTDOMAIN((yDimSlabStartRow, 0, 0),
                                                    (yDimSlabEndRow, nx,
                                                     nz+PADDING)));
#endif
  array2 =
    ndarray<ndarray<Complex, 3, global GUNSTRIDED>, 1>(RECTDOMAIN((0),
                                                                  (ranks())));
  array2.exchange(myArray2);

  ndarray<Complex, 3 UNSTRIDED> myArray3(RECTDOMAIN((yDimSlabStartRow, 0, 0),
                                                    (yDimSlabEndRow, nz,
                                                     nx+PADDING)));
  array3 =
    ndarray<ndarray<Complex, 3, global GUNSTRIDED>, 1>(RECTDOMAIN((0),
                                                                  (ranks())));
  array3.exchange(myArray3);

  ndarray<Complex, 3 UNSTRIDED> myArray4(RECTDOMAIN((yDimSlabStartRow, 0, 0),
                                                    (yDimSlabEndRow, nz,
                                                     nx+PADDING)));
  array4 =
    ndarray<ndarray<Complex, 3, global GUNSTRIDED>, 1>(RECTDOMAIN((0),
                                                                  (ranks())));
  array4.exchange(myArray4);

#ifdef SLABS
  ndarray<Complex, 3 UNSTRIDED> myArray5(RECTDOMAIN((0, zDimSlabStartRow, 0),
                                                    (ny, zDimSlabEndRow,
                                                     nx+PADDING)));
#else
  ndarray<Complex, 3 UNSTRIDED> myArray5(RECTDOMAIN((zDimSlabStartRow, 0, 0),
                                                    (zDimSlabEndRow, ny,
                                                     nx+PADDING)));
#endif
  array5 =
    ndarray<ndarray<Complex, 3, global GUNSTRIDED>, 1>(RECTDOMAIN((0),
                                                                  (ranks())));
  array5.exchange(myArray5);

  ndarray<Complex, 3 UNSTRIDED> myArray6(RECTDOMAIN((zDimSlabStartRow, 0, 0),
                                                    (zDimSlabEndRow, nx,
                                                     ny+PADDING)));
  array6 =
    ndarray<ndarray<Complex, 3, global GUNSTRIDED>, 1>(RECTDOMAIN((0),
                                                                  (ranks())));
  array6.exchange(myArray6);


  // profiling
  // myTotalTimer = new Timer();
#ifdef TIMERS_ENABLED
  // mySetupTimer = new Timer();
#endif
}

void FTDriver::printSetupTiming(int i) {
#ifdef TIMERS_ENABLED
  if (myrank() == 0) {
    println("");
    println("SETUP: ");
  }

  double gminTime = reduce::min(mySetupTime);
  double gmaxTime = reduce::max(mySetupTime);
  double gsumTime = reduce::add(mySetupTime);

  if (myrank() == 0) {
    if (i == 0) {
      println("Time: " << gminTime << ", " << (gsumTime/ranks()) <<
              ", " << gmaxTime);
    }
    else if (i == 1) {
      println("Num Readings Per Proc:\t1\nMin Time Over Procs:\t" <<
              gminTime << "\nMean Time Over Procs:\t" <<
              (gsumTime/ranks()) << "\nMax Time Over Procs:\t" <<
              gmaxTime << "\n");
    }
  }
#endif
}

void FTDriver::main(int argc, char **argv) {
  // read and check input
  char classArg;
  bool invalidInput = false;
  if (argc < 1) {
    invalidInput = true;
  }
  else {
    classArg = argv[0][0];
    if (classArg != 'A' && classArg != 'B' && classArg != 'C' &&
        classArg != 'D' && classArg != 'S' && classArg != 'W') {
      invalidInput = true;
    }
  }

  if (invalidInput) {
    if (myrank() == 0) {
      println("Allowed problem classes are A, B, C, D, S, and W.");
    }
    exit(1);
  }

  FTDriver Driver(classArg);
  FT myFT(Driver.array1, Driver.array2, Driver.array3,
          Driver.array4, Driver.array5, Driver.array6);
  Test myTest(Driver.array3);

  if (myrank() == 0) {
    println("Titanium NAS FT Benchmark- Parallel\n");
    println("Problem class = " << Driver.classType);
    println("Number of processors = " << ranks());
    println("Grid size = " << nx << " x " << ny << " x " << nz);
    println("Number of iterations = " << numIterations);
    println("");
  }

  // run the forward FFT part of the benchmark to warm the cache
  myFT.create_FFT_plans();
  myTest.initialConditions(Driver.array1);
  myTest.initializeExponentialArray();
  myFT.FFT_3D(1, 0);

  // start timer for actual FT benchmark
  barrier();
  TIMER_START(Driver.myTotalTimer);
#ifdef TIMERS_ENABLED
  TIMER_START(Driver.mySetupTimer);
#endif

  // (timed) setup before any FFTs performed
  myTest.initialConditions(Driver.array1);
  myTest.initializeExponentialArray();
	
#ifdef TIMERS_ENABLED
  TIMER_STOP(Driver.mySetupTimer);
  Driver.mySetupTime = Driver.mySetupTimer.secs();
#endif

  // (timed) FFT portion of benchmark
  myFT.FFT_3D(1, 0);   // forward 3D FFT
  for (int iter=1; iter <= Driver.numIterations; iter++) {
    myTest.evolve(Driver.array3, iter);
    myFT.FFT_3D(-1, iter);   // reverse 3D FFT
    myTest.checkSum(Driver.array6, iter);
  }
  myTest.verifyCheckSum(Driver.classType);

  TIMER_STOP(Driver.myTotalTimer);
  // end timer

  // print profiling info
  if (myrank() == 0) {
    println("\nSUMMARY- min, mean, max for each component");
    println("All times in seconds.");
  }
  Driver.printSetupTiming(0);
  myFT.printSummary();
  myTest.printSummary();

#ifdef TIMERS_ENABLED
  double myTotalTime = Driver.myTotalTimer.secs();
  double maxTotalTime = reduce::max(myTotalTime);

  if (myrank() == 0) {
    println("");
    println("Max Time Over All Procs: " << maxTotalTime << "\n\n");
  }
#endif

  Driver.printSetupTiming(1);
  myFT.printProfile();
  myTest.printProfile();
}

int main(int argc, char **argv) {
  init(&argc, &argv);
  FTDriver::main(argc-1, argv+1);
  finalize();
  return 0;
}
