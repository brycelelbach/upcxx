#include "FT.h"
#include "FTDriver.h"

FT::FT(ndarray<ndarray<Complex, 3, global GUNSTRIDED>, 1> p_array1,
       ndarray<ndarray<Complex, 3, global GUNSTRIDED>, 1> p_array2,
       ndarray<ndarray<Complex, 3, global GUNSTRIDED>, 1> p_array3,
       ndarray<ndarray<Complex, 3, global GUNSTRIDED>, 1> p_array4,
       ndarray<ndarray<Complex, 3, global GUNSTRIDED>, 1> p_array5,
       ndarray<ndarray<Complex, 3, global GUNSTRIDED>, 1> p_array6) {
  // copied from FTDriver
  nx = FTDriver::nx;
  ny = FTDriver::ny;
  nz = FTDriver::nz;
  maxdim = FTDriver::maxdim;
  numIterations  = FTDriver::numIterations;

  // copying from parameters to global variables
  array1 = p_array1;
  array2 = p_array2;
  array3 = p_array3;
  array4 = p_array4;
  array5 = p_array5;
  array6 = p_array6;

  // myArray[num] is the portion of FTDriver.array[num] on this processor
  myArray1 = (ndarray<Complex, 3 UNSTRIDED>) array1[myrank()];
  myArray2 = (ndarray<Complex, 3 UNSTRIDED>) array2[myrank()];
  myArray3 = (ndarray<Complex, 3 UNSTRIDED>) array3[myrank()];
  myArray4 = (ndarray<Complex, 3 UNSTRIDED>) array4[myrank()];
  myArray5 = (ndarray<Complex, 3 UNSTRIDED>) array5[myrank()];
  myArray6 = (ndarray<Complex, 3 UNSTRIDED>) array6[myrank()];

  // temp is used to store one row in the last dimension of array[1,2,3] for in-place FFTW
  temp = ndarray<Complex, 1 UNSTRIDED>(RECTDOMAIN((0), (maxdim)));

  // initialize timers
#ifdef TIMERS_ENABLED
  int numTimers = 6;
#ifdef NONBLOCKING_ARRAYCOPY
  numTimers = 7;
#endif
	
  // set up timers
  // myTimer = new Timer();
  // myTimer2 = new Timer();

  // set up arrays for storing times
  myTimes =
    ndarray<ndarray<double, 1 UNSTRIDED>, 1 UNSTRIDED>(RECTDOMAIN((1),
                                                                  (numTimers+1)));
	
  myTimes[T_X_FFT] =
    ndarray<double, 1 UNSTRIDED>(RECTDOMAIN((0), (numIterations+1)));
  myTimes[T_Y_FOR_FFT] =
    ndarray<double, 1 UNSTRIDED>(RECTDOMAIN((0), (1)));
  myTimes[T_Y_REV_FFT] =
    ndarray<double, 1 UNSTRIDED>(RECTDOMAIN((1), (numIterations+1)));
  myTimes[T_Z_FFT] =
    ndarray<double, 1 UNSTRIDED>(RECTDOMAIN((0), (numIterations+1)));
  myTimes[T_GLOBAL_ARRAYCOPY] =
    ndarray<double, 1 UNSTRIDED>(RECTDOMAIN((0), (numIterations+1)));
  myTimes[T_BARRIER] =
    ndarray<double, 1 UNSTRIDED>(RECTDOMAIN((0), (numIterations+1)));
#ifdef NONBLOCKING_ARRAYCOPY
  myTimes[T_SYNC] =
    ndarray<double, 1 UNSTRIDED>(RECTDOMAIN((0), (numIterations+1)));
#endif
#endif
}

void FT::FFT_3D(int direction, int iter) {
  int new_nx, new_ny, new_nz;

  // the "new" variables allow the first dimension to be "nx", the
  // second to be "ny", and the contiguous (third) dimension to be
  // "nz"
  if (direction == 1) {
    new_nx = nx;
    new_ny = ny;
    new_nz = nz;
  }
  else {
    new_nx = ny;
    new_ny = nz;
    new_nz = nx;
  }

  int xDimSlabSize = new_nx/ranks();
  int yDimSlabSize = new_ny/ranks();
  RectDomain<1> xDimLocalSlabRD(POINT(myrank() * xDimSlabSize),
                                POINT((myrank()+1) * xDimSlabSize));
  RectDomain<1> yDimLocalSlabRD(POINT(myrank() * yDimSlabSize),
                                POINT((myrank()+1) * yDimSlabSize));

  // Y-TRANSFORM (FORWARD IS IN-PLACE, BACKWARD IS OUT-OF-PLACE)
  TIMER_START(myTimer);

  foreach (i, xDimLocalSlabRD) {
    if (direction == 1) {
      pencilToPencilStrideForwardY(myArray1.slice(1, i[1]), temp);
    }
    else {
      pencilToPencilStrideBackwardY(myArray3.slice(1, i[1]),
                                    myArray4.slice(1, i[1]));
    }
  };

  if (direction == 1) {
    TIMER_STOP_READ(myTimer, myTimes[T_Y_FOR_FFT][iter]);
  }
  else {
    TIMER_STOP_READ(myTimer, myTimes[T_Y_REV_FFT][iter]);
  }

  TIMER_START(myTimer2);

  // IN-PLACE Z-TRANSFORM AND GLOBAL ARRAYCOPY
  foreach (i, xDimLocalSlabRD) {
    for (int relProc=0; relProc<ranks(); relProc++) {
      int actualProc = (myrank()+relProc)%ranks();
      int jSlab = actualProc * yDimSlabSize;
#ifdef SLABS
      RectDomain<3> rd(POINT(i[1], jSlab, 0),
                       POINT(i[1]+1, jSlab+yDimSlabSize, new_nz+PADDING));
      if (direction == 1) {
        TIMER_START(myTimer);

        unitToUnitStrideForwardZ(myArray1.slice(2, jSlab).slice(1, i[1]), temp);

        TIMER_STOP(myTimer);

        // send off the completed slab to a staggered processor
        array2[actualProc].COPY(myArray1.constrict(rd));
      }
      else {
        TIMER_START(myTimer);

        unitToUnitStrideBackwardZ(myArray4.slice(2, jSlab).slice(1, i[1]), temp);

        TIMER_STOP(myTimer);

        // send off the completed slab to a staggered processor
        array5[actualProc].COPY(myArray4.constrict(rd));
      }
#else
      foreach (jPencil, RECTDOMAIN((jSlab), (jSlab+yDimSlabSize))) {
        RectDomain<3> rd(POINT(i[1], jPencil[1], 0),
                         POINT(i[1]+1, jPencil[1]+1, new_nz+PADDING));
        rd = rd.permute(POINT(2,1,3));
        if (direction == 1) {
          TIMER_START(myTimer);

          unitToUnitStrideForwardZ(myArray1.slice(2, jPencil[1]).slice(1, i[1]),
                                   temp);

          TIMER_STOP(myTimer);

          // send off the completed slab to a staggered processor
          array2[actualProc].COPY(myArray1.constrict(rd));
        }
        else {
          TIMER_START(myTimer);

          unitToUnitStrideBackwardZ(myArray4.slice(2, jPencil[1]).slice(1, i[1]),
                                    temp);

          TIMER_STOP(myTimer);

          // send off the completed slab to a staggered processor
          array5[actualProc].COPY(myArray4.constrict(rd));
        }
      };
#endif
    }
  };

#ifdef TIMERS_ENABLED
  myTimer2.stop();
  myTimes[T_Z_FFT][iter] = myTimer.secs();
  myTimes[T_GLOBAL_ARRAYCOPY][iter] = myTimer2.secs() - myTimer.secs();
  myTimer.reset();
  myTimer2.reset();
#endif

#ifdef NONBLOCKING_ARRAYCOPY
  TIMER_START(myTimer);

  async_wait();

  TIMER_STOP_READ(myTimer, myTimes[T_SYNC][iter]);
#endif
  TIMER_START(myTimer);

  barrier();

  TIMER_STOP_READ(myTimer, myTimes[T_BARRIER][iter]);

  // OUT-OF-PLACE X-TRANSFORM

  TIMER_START(myTimer);

#ifdef SLABS
  foreach (j, yDimLocalSlabRD) {
    if (direction == 1) {
      slabToUnitStrideForwardX(myArray2.slice(2, j[1]),
                               myArray3.slice(1, j[1]));
    }
    else {
      slabToUnitStrideBackwardX(myArray5.slice(2, j[1]),
                                myArray6.slice(1, j[1]));
    }
  };
#else
  foreach (j, yDimLocalSlabRD) {
    if (direction == 1) {
      pencilToUnitStrideForwardX(myArray2.slice(1, j[1]),
                                 myArray3.slice(1, j[1]));
    }
    else {
      pencilToUnitStrideBackwardX(myArray5.slice(1, j[1]),
                                  myArray6.slice(1, j[1]));
    }
  };
#endif

  TIMER_STOP_READ(myTimer, myTimes[T_X_FFT][iter]);
}

void FT::printSummary() {
#ifdef TIMERS_ENABLED
  int timerMinIdx = broadcast(myTimes.domain().min()[1], 0);
  int timerMaxIdx = broadcast(myTimes.domain().max()[1], 0);
  for (int timerIdx=timerMinIdx; timerIdx<=timerMaxIdx; timerIdx++) {
    if (myrank() == 0) {
      switch (timerIdx) {
      case T_X_FFT:
#ifdef SLABS
        println("X-DIRECTION FFT (OUT-OF-PLACE SLAB-TO-UNIT STRIDED): ");
#else
        println("X-DIRECTION FFT (OUT-OF-PLACE PENCIL-TO-UNIT STRIDED): ");
#endif
        break;
      case T_Y_FOR_FFT:
        println("Y-DIRECTION FFT (IN-PLACE PENCIL-TO-PENCIL STRIDED): ");
        break;
      case T_Y_REV_FFT:
        println("Y-DIRECTION FFT (OUT-OF-PLACE PENCIL-TO-PENCIL STRIDED): ");
        break;
      case T_Z_FFT:
#ifdef SLABS
        println("Z-DIRECTION FFT (IN-PLACE UNIT-TO-UNIT STRIDED) PERFORMED IN DISCRETE SLABS: ");
#else
        println("Z-DIRECTION FFT (IN-PLACE UNIT-TO-UNIT STRIDED) PERFORMED IN DISCRETE PENCILS: ");
#endif
        break;
      case T_GLOBAL_ARRAYCOPY:
#ifdef SLABS
        println("GLOBAL SLAB ARRAYCOPY: ");
#else
        println("GLOBAL PENCIL ARRAYCOPY: ");
#endif
        break;
      case T_BARRIER:
        println("BARRIER IN FFT: ");
        break;
#ifdef NONBLOCKING_ARRAYCOPY
      case T_SYNC:
        println("SYNC IN FFT: ");
        break;
#endif
      }
    }

    double totalComponentTime = 0.0;
    foreach (timing, myTimes[timerIdx].domain()) {
      totalComponentTime += myTimes[timerIdx][timing];
    };

    double minTotalComponentTime = reduce::min(totalComponentTime);
    double sumTotalComponentTime = reduce::add(totalComponentTime);
    double maxTotalComponentTime = reduce::max(totalComponentTime);
	    
    if (myrank() == 0) {
      println("Time: " << minTotalComponentTime << ", " <<
              (sumTotalComponentTime/ranks()) << ", " <<
              maxTotalComponentTime);
    }
  }
#endif
}

void FT::printProfile() {
#ifdef TIMERS_ENABLED
  int timerMinIdx = broadcast(myTimes.domain().min()[1], 0);
  int timerMaxIdx = broadcast(myTimes.domain().max()[1], 0);
  for (int timerIdx=timerMinIdx; timerIdx<=timerMaxIdx; timerIdx++) {
    if (myrank() == 0) {
      switch (timerIdx) {
      case T_X_FFT:
#ifdef SLABS
        println("X-DIRECTION FFT (OUT-OF-PLACE SLAB-TO-UNIT STRIDED): ");
#else
        println("X-DIRECTION FFT (OUT-OF-PLACE PENCIL-TO-UNIT STRIDED): ");
#endif
        break;
      case T_Y_FOR_FFT:
        println("Y-DIRECTION FFT (IN-PLACE PENCIL-TO-PENCIL STRIDED): ");
        break;
      case T_Y_REV_FFT:
        println("Y-DIRECTION FFT (OUT-OF-PLACE PENCIL-TO-PENCIL STRIDED): ");
        break;
      case T_Z_FFT:
#ifdef SLABS
        println("Z-DIRECTION FFT (IN-PLACE UNIT-TO-UNIT STRIDED) PERFORMED IN DISCRETE SLABS: ");
#else
        println("Z-DIRECTION FFT (IN-PLACE UNIT-TO-UNIT STRIDED) PERFORMED IN DISCRETE PENCILS: ");
#endif
        break;
      case T_GLOBAL_ARRAYCOPY:
#ifdef SLABS
        println("GLOBAL SLAB ARRAYCOPY: ");
#else
        println("GLOBAL PENCIL ARRAYCOPY: ");
#endif
        break;
      case T_BARRIER:
        println("BARRIER IN FFT: ");
        break;
#ifdef NONBLOCKING_ARRAYCOPY
      case T_SYNC:
        println("SYNC IN FFT: ");
        break;
#endif
      }
    }

    // print timer info
    double lminTime = myTimes[timerIdx][myTimes[timerIdx].domain().min()];
    double lmaxTime = lminTime;
    double lsumTime = 0;
    int numReadingsPerProc = 0;
	    
    foreach (timing, myTimes[timerIdx].domain()) {
      double value = myTimes[timerIdx][timing];
      if (value < lminTime) {
        lminTime = value;
      }
      if (value > lmaxTime) {
        lmaxTime = value;
      }
      lsumTime += value;
      numReadingsPerProc++;
    };
	    
    double gminTime = reduce::min(lminTime);
    double gmaxTime = reduce::max(lmaxTime);
    double gsumTime = reduce::add(lsumTime);
	    
    if (myrank() == 0) {
      println("Num Readings Per Proc:\t" << numReadingsPerProc);
      println("Min Time Over Procs:\t" << gminTime);
      double gmeanTime = gsumTime/(numReadingsPerProc*ranks());
      println("Mean Time Over Procs:\t" << gmeanTime);
      println("Max Time Over Procs:\t" << gmaxTime << "\n");
    }
  }
#endif
}
