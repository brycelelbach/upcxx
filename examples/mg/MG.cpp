#include <cmath>
#include "MG.h"

MG::MG() {
  startLevel = MGDriver::startLevel;
  numIterations = MGDriver::numIterations;
	
  // initialize counters and timers
  numTimers = 8;
#ifdef CONTIGUOUS_ARRAY_BUFFERS
  numTimers = 10;
#endif
#ifdef NONBLOCKING_ARRAYCOPY
  numTimers = 11;
#endif
  numCounters = 5;

  // set up timer
  // myTimer = new Timer();

  myTimes =
    ndarray<ndarray<ndarray<double, 1 UNSTRIDED>, 1 UNSTRIDED>,
            1 UNSTRIDED>(RECTDOMAIN((1), (numTimers+1)));

  // set up arrays for storing times
  myTimes[T_L2NORM] =
    ndarray<ndarray<double, 1 UNSTRIDED>,
            1 UNSTRIDED>(RECTDOMAIN((startLevel), (startLevel+1)));
  myTimes[T_APPLY_SMOOTHER] =
    ndarray<ndarray<double, 1 UNSTRIDED>,
            1 UNSTRIDED>(RECTDOMAIN((1), (startLevel+1)));
  myTimes[T_EVALUATE_RESIDUAL] =
    ndarray<ndarray<double, 1 UNSTRIDED>,
            1 UNSTRIDED>(RECTDOMAIN((2), (startLevel+1)));
  myTimes[T_COARSEN] =
    ndarray<ndarray<double, 1 UNSTRIDED>,
            1 UNSTRIDED>(RECTDOMAIN((2), (startLevel+1)));
  myTimes[T_PROLONGATE] =
    ndarray<ndarray<double, 1 UNSTRIDED>,
            1 UNSTRIDED>(RECTDOMAIN((2), (startLevel+1)));
  myTimes[T_GLOBAL_COMM] =
    ndarray<ndarray<double, 1 UNSTRIDED>,
            1 UNSTRIDED>(RECTDOMAIN((THRESHOLD_LEVEL), (startLevel+1)));
  myTimes[T_THRESHOLD_COMM] =
    ndarray<ndarray<double, 1 UNSTRIDED>,
            1 UNSTRIDED>(RECTDOMAIN((THRESHOLD_LEVEL),
                                    (THRESHOLD_LEVEL+1)));
  myTimes[T_BARRIER] =
    ndarray<ndarray<double, 1 UNSTRIDED>,
            1 UNSTRIDED>(RECTDOMAIN((THRESHOLD_LEVEL), (startLevel+1)));
#ifdef CONTIGUOUS_ARRAY_BUFFERS
  myTimes[T_PACKING] =
    ndarray<ndarray<double, 1 UNSTRIDED>,
            1 UNSTRIDED>(RECTDOMAIN((THRESHOLD_LEVEL), (startLevel+1)));
  myTimes[T_UNPACKING] =
    ndarray<ndarray<double, 1 UNSTRIDED>,
            1 UNSTRIDED>(RECTDOMAIN((THRESHOLD_LEVEL), (startLevel+1)));
#endif
#ifdef NONBLOCKING_ARRAYCOPY
  myTimes[T_SYNC] =
    ndarray<ndarray<double, 1 UNSTRIDED>,
            1 UNSTRIDED>(RECTDOMAIN((THRESHOLD_LEVEL), (startLevel+1)));
#endif

  myTimes[T_L2NORM][startLevel] =
    ndarray<double, 1 UNSTRIDED>(RECTDOMAIN((1), (3)));

  foreach (level, RECTDOMAIN((1), (startLevel+1))) {
    myTimes[T_APPLY_SMOOTHER][level] =
      ndarray<double, 1 UNSTRIDED>(RECTDOMAIN((1), (numIterations+1)));
  }

  foreach (level, RECTDOMAIN((2), (startLevel+1))) {
    if (level[1] != startLevel) {
      myTimes[T_EVALUATE_RESIDUAL][level] =
        ndarray<double, 1 UNSTRIDED>(RECTDOMAIN((1), (numIterations+1)));
    }
    else {
      myTimes[T_EVALUATE_RESIDUAL][level] =
        ndarray<double, 1 UNSTRIDED>(RECTDOMAIN((1), (2*numIterations+2)));
    }
    myTimes[T_COARSEN][level] =
      ndarray<double, 1 UNSTRIDED>(RECTDOMAIN((1), (numIterations+1)));
    myTimes[T_PROLONGATE][level] =
      ndarray<double, 1 UNSTRIDED>(RECTDOMAIN((1), (numIterations+1)));
  }

  myTimes[T_THRESHOLD_COMM][THRESHOLD_LEVEL] =
    ndarray<double, 1 UNSTRIDED>(RECTDOMAIN((1), (2*numIterations+1)));

  foreach (level, RECTDOMAIN((THRESHOLD_LEVEL), (startLevel+1))) {
#if UPDATE_BORDER_DIRECTIONS == 6
    if (level[1] != startLevel) {
      myTimes[T_GLOBAL_COMM][level] =
        ndarray<double, 1 UNSTRIDED>(RECTDOMAIN((1), (9*numIterations+1)));
    }
    else {
      myTimes[T_GLOBAL_COMM][level] =
        ndarray<double, 1 UNSTRIDED>(RECTDOMAIN((-2), (9*numIterations+1)));
    }

    if (level[1] == THRESHOLD_LEVEL) {
      myTimes[T_BARRIER][level] =
        ndarray<double, 1 UNSTRIDED>(RECTDOMAIN((-2*numIterations+1),
                                                (9*numIterations+1)));
    }
    else if (level[1] == startLevel) {
      myTimes[T_BARRIER][level] =
        ndarray<double, 1 UNSTRIDED>(RECTDOMAIN((-2), (9*numIterations+1)));
    }
    else {
      myTimes[T_BARRIER][level] =
        ndarray<double, 1 UNSTRIDED>(RECTDOMAIN((1), (9*numIterations+1)));
    }
#elif UPDATE_BORDER_DIRECTIONS == 26
    if (level[1] != startLevel) {
      myTimes[T_GLOBAL_COMM][level] =
        ndarray<double, 1 UNSTRIDED>(RECTDOMAIN((1), (3*numIterations+1)));
    }
    else {
      myTimes[T_GLOBAL_COMM][level] =
        ndarray<double, 1 UNSTRIDED>(RECTDOMAIN((0), (3*numIterations+1)));
    }

    if (level[1] == THRESHOLD_LEVEL) {
      myTimes[T_BARRIER][level] =
        ndarray<double, 1 UNSTRIDED>(RECTDOMAIN((-2*numIterations+1),
                                                (3*numIterations+1)));
    }
    else if (level[1] == startLevel) {
      myTimes[T_BARRIER][level] =
        ndarray<double, 1 UNSTRIDED>(RECTDOMAIN((-2), (3*numIterations+1)));
    }
    else {
      myTimes[T_BARRIER][level] =
        ndarray<double, 1 UNSTRIDED>(RECTDOMAIN((1), (3*numIterations+1)));
    }
#endif

#ifdef CONTIGUOUS_ARRAY_BUFFERS
#if UPDATE_BORDER_DIRECTIONS == 6
    if (level[1] != startLevel) {
      myTimes[T_PACKING][level] =
        ndarray<double, 1 UNSTRIDED>(RECTDOMAIN((1), (6*numIterations+1)));
      myTimes[T_UNPACKING][level] =
        ndarray<double, 1 UNSTRIDED>(RECTDOMAIN((1), (6*numIterations+1)));
    }
    else {
      myTimes[T_PACKING][level] =
        ndarray<double, 1 UNSTRIDED>(RECTDOMAIN((-1), (6*numIterations+1)));
      myTimes[T_UNPACKING][level] =
        ndarray<double, 1 UNSTRIDED>(RECTDOMAIN((-1), (6*numIterations+1)));
    }
#elif UPDATE_BORDER_DIRECTIONS == 26
    if (level[1] != startLevel) {
      myTimes[T_PACKING][level] =
        ndarray<double, 1 UNSTRIDED>(RECTDOMAIN((1), (3*numIterations+1)));
      myTimes[T_UNPACKING][level] =
        ndarray<double, 1 UNSTRIDED>(RECTDOMAIN((1), (3*numIterations+1)));
    }
    else {
      myTimes[T_PACKING][level] =
        ndarray<double, 1 UNSTRIDED>(RECTDOMAIN((0), (3*numIterations+1)));
      myTimes[T_UNPACKING][level] =
        ndarray<double, 1 UNSTRIDED>(RECTDOMAIN((0), (3*numIterations+1)));
    }
#endif
#endif

#ifdef NONBLOCKING_ARRAYCOPY
    myTimes[T_SYNC][level] =
      ndarray<double, 1 UNSTRIDED>(myTimes[T_GLOBAL_COMM][level].domain());
#ifndef CONTIGUOUS_ARRAY_BUFFERS
    // make empty domains for this case
    myTimes[T_PACKING] =
      ndarray<ndarray<double, 1 UNSTRIDED>, 1 UNSTRIDED>(RECTDOMAIN((0), (0)));
    myTimes[T_UNPACKING] =
      ndarray<ndarray<double, 1 UNSTRIDED>, 1 UNSTRIDED>(RECTDOMAIN((0), (0)));
#endif
  }

#ifdef COUNTERS_ENABLED
  myCounter = new PAPICounter(PAPI_MEASURE);
  myCounts =
    ndarray<ndarray<ndarray<long, 1 UNSTRIDED>, 1 UNSTRIDED>,
            1 UNSTRIDED>(RECTDOMAIN((1), (numCounters+1)));
	
  // set up arrays for storing counts
  myCounts[T_L2NORM] =
    ndarray<ndarray<long, 1 UNSTRIDED>,
            1 UNSTRIDED>(RECTDOMAIN((startLevel), (startLevel+1)));
  myCounts[T_APPLY_SMOOTHER] =
    ndarray<ndarray<long, 1 UNSTRIDED>,
            1 UNSTRIDED>(RECTDOMAIN((1), (startLevel+1)));
  myCounts[T_EVALUATE_RESIDUAL] =
    ndarray<ndarray<long, 1 UNSTRIDED>,
            1 UNSTRIDED>(RECTDOMAIN((2), (startLevel+1)));
  myCounts[T_COARSEN] =
    ndarray<ndarray<long, 1 UNSTRIDED>,
            1 UNSTRIDED>(RECTDOMAIN((2), (startLevel+1)));
  myCounts[T_PROLONGATE] =
    ndarray<ndarray<long, 1 UNSTRIDED>,
            1 UNSTRIDED>(RECTDOMAIN((2), (startLevel+1)));

  myCounts[T_L2NORM][startLevel] =
    ndarray<long, 1 UNSTRIDED>(RECTDOMAIN((1), (3)));

  foreach (level, RECTDOMAIN((1), (startLevel+1))) {
    myCounts[T_APPLY_SMOOTHER][level] =
      ndarray<long, 1 UNSTRIDED>(RECTDOMAIN((1), (numIterations+1)));
  }

  foreach (level, RECTDOMAIN((2), (startLevel+1))) {
    if (level[1] != startLevel) {
      myCounts[T_EVALUATE_RESIDUAL][level] =
        ndarray<long, 1 UNSTRIDED>(RECTDOMAIN((1), (numIterations+1)));
    }
    else {
      myCounts[T_EVALUATE_RESIDUAL][level] =
        ndarray<long, 1 UNSTRIDED>(RECTDOMAIN((1), (2*numIterations+2)));
    }
    myCounts[T_COARSEN][level] =
      ndarray<long, 1 UNSTRIDED>(RECTDOMAIN((1), (numIterations+1)));
    myCounts[T_PROLONGATE][level] =
      ndarray<long, 1 UNSTRIDED>(RECTDOMAIN((1), (numIterations+1)));
  }
#endif
#endif
}

extern int ipow(int base, int exp);

static inline double sqr(double d) {
  return d * d;
}

double MG::getL2Norm(Grid &gridA, int callNumber) {
  double myASquareSum = 0;
  Point<3> myBlockPos = Grid::myBlockPos;
  ndarray<double, 3 UNSTRIDED> myAPoints =
    (ndarray<double, 3 UNSTRIDED>) gridA.points[myBlockPos];

  COUNTER_START(myCounter);
  TIMER_START(myTimer);

#ifdef SPLIT_LOOP
  foreach3 (i, j, k, myAPoints.domain().shrink(1)) {
    myASquareSum += sqr(myAPoints[i][j][k]);
  }
#else
  foreach (p, myAPoints.domain().shrink(1)) {
    myASquareSum += sqr(myAPoints[p]);
  }
#endif

  TIMER_STOP_READ(myTimer, myTimes[T_L2NORM][startLevel][callNumber]);
  COUNTER_STOP_READ(myCounter, myCounts[T_L2NORM][startLevel][callNumber]);

  TIMER_START(myTimer);

  double L2Norm = sqrt(reduce::add(myASquareSum)/ipow(2, 3*startLevel));

  TIMER_STOP_READ(myTimer, myTimes[T_BARRIER][startLevel][-callNumber]);

  return L2Norm;
}

int MG::mod(int a, int b) {
  int quot = (int)floor((double)a/(double)b);
  return a - quot*b;
}

void MG::updateBorder(Grid &gridA, int level, int iterationNum, int callNumber) {
  Point<3> myBlockPos = Grid::myBlockPos;
  Point<3> numBlocksInGridSide = Grid::numBlocksInGridSide;
  ndarray<double, 3 UNSTRIDED> myPoints =
    (ndarray<double, 3 UNSTRIDED>) gridA.points[myBlockPos];
  int numCellsInGridSide = (int)ipow(2, level);

  if (level >= THRESHOLD_LEVEL) {
#if UPDATE_BORDER_DIRECTIONS == 26
    // communicate with all 26 nearest neighbors
#ifdef CONTIGUOUS_ARRAY_BUFFERS
    // use buffers so that arraycopys are contiguous in memory
    ndarray<ndarray<double, 3, global>, 3 UNSTRIDED> myOutBuffer =
      (ndarray<ndarray<double, 3, global>,
       3 UNSTRIDED>) gridA.outBuffers[myBlockPos];
    ndarray<ndarray<double, 3, global>, 3 UNSTRIDED> myInBuffer =
      (ndarray<ndarray<double, 3, global>,
       3 UNSTRIDED>) gridA.inBuffers[myBlockPos];
	    
    TIMER_START(myTimer);

    // packing
    foreach (dir, (myOutBuffer.domain() - RectDomain<3>((0, 0, 0), (1, 1, 1)))) {
      if (dir[1] == 0 || dir[2] == 0) {
        myOutBuffer[dir].copy(myPoints);
      }
    }

    TIMER_STOP_READ(myTimer, myTimes[T_PACKING][level][3*(iterationNum-1) +
                                                       callNumber]);
	    
#ifndef PUSH_DATA
    TIMER_START(myTimer);

    barrier();

    TIMER_STOP_READ(myTimer, myTimes[T_BARRIER][level][3*(iterationNum-1) +
                                                       callNumber]);
#endif

    TIMER_START(myTimer);

#ifdef PUSH_DATA
    ndarray<ndarray<double, 3, global>, 3 UNSTRIDED> myBuffer = myOutBuffer;
#else
    ndarray<ndarray<double, 3, global>, 3 UNSTRIDED> myBuffer = myInBuffer;
#endif
    Point<3> numCellsPerBlock = numCellsInGridSide / numBlocksInGridSide;

    foreach (neighborBlockPos,
             (myBuffer.translate(myBlockPos).domain() -
              RectDomain<3>(myBlockPos, myBlockPos+POINT(1, 1, 1)))) {
      // "actualBlockPos" is the actual block from which we retrieve data, since
      // we need to factor in periodic boundary conditions
      Point<3> actualBlockPos =
        {{mod(neighborBlockPos[1], numBlocksInGridSide[1]),
          mod(neighborBlockPos[2], numBlocksInGridSide[2]),
          mod(neighborBlockPos[3], numBlocksInGridSide[3])}};
      Point<3> myDirection = neighborBlockPos - myBlockPos;

      // NOTE: in order to implement periodic boundary conditions, we move the local
      // block instead of the remote block
      Point<3> pointTranslationVector =
        ((actualBlockPos - neighborBlockPos) / numBlocksInGridSide) *
        numCellsInGridSide;
      ndarray<double, 3, global> myTranslatedBlockBuffers =
        myBuffer[myDirection].translate(pointTranslationVector);
		
      // now copy new info
#ifdef PUSH_DATA
      gridA.inBuffers[actualBlockPos][POINT(0, 0, 0)-myDirection].COPY(myTranslatedBlockBuffers);
#else
      myTranslatedBlockBuffers.COPY(gridA.outBuffers[actualBlockPos][POINT(0, 0, 0)-myDirection]);
#endif
    }

    TIMER_STOP_READ(myTimer, myTimes[T_GLOBAL_COMM][level][3*(iterationNum-1) +
                                                           callNumber]);
	    
#ifdef NONBLOCKING_ARRAYCOPY
    TIMER_START(myTimer);

    // Handle.syncNBI();
    async_copy_fence();

    TIMER_STOP_READ(myTimer, myTimes[T_SYNC][level][3*(iterationNum-1) +
                                                    callNumber]);
#endif

#ifdef PUSH_DATA
    TIMER_START(myTimer);

    barrier();

    TIMER_STOP_READ(myTimer, myTimes[T_BARRIER][level][3*(iterationNum-1) +
                                                       callNumber]);
#endif

    TIMER_START(myTimer);

    // unpacking
    foreach (dir, (myBuffer.domain() - RECTDOMAIN((0, 0, 0), (1, 1, 1)))) {
      if (dir[1] == 0 || dir[2] == 0) {
        myPoints.copy(myInBuffer[dir]);
      }
    }

    TIMER_STOP_READ(myTimer, myTimes[T_UNPACKING][level][3*(iterationNum-1) +
                                                         callNumber]);

    // end of contiguous array buffers case (26 directions)
#else
    // start of no contiguous array buffers case (26 directions)

#ifndef PUSH_DATA
    TIMER_START(myTimer);

    barrier();

    TIMER_STOP_READ(myTimer, myTimes[T_BARRIER][level][3*(iterationNum-1) +
                                                       callNumber]);
#endif
    TIMER_START(myTimer);
	    
    // "surroundingBlocks" is the 3x3x3 cube of blocks around the current block
    RectDomain<3> surroundingBlocks(myBlockPos+POINT(-1, -1, -1),
                                    myBlockPos+POINT(2, 2, 2));
	    
    foreach (neighborBlockPos, surroundingBlocks) {
      if (neighborBlockPos != myBlockPos) {
        // "actualBlockPos" is the actual block from which we retrieve data, since
        // we need to factor in periodic boundary conditions
        Point<3> actualBlockPos =
          {{mod(neighborBlockPos[1], numBlocksInGridSide[1]),
            mod(neighborBlockPos[2], numBlocksInGridSide[2]),
            mod(neighborBlockPos[3], numBlocksInGridSide[3])}};

        // NOTE: in order to implement periodic boundary conditions, we move the local
        // block instead of the remote block
        Point<3> pointTranslationVector =
          ((actualBlockPos - neighborBlockPos) / numBlocksInGridSide) *
          numCellsInGridSide;
        ndarray<double, 3> myTranslatedBlockPoints =
          myBlockPoints.translate(pointTranslationVector); // myBlockPoints???
		    
        // now copy new info
#ifdef PUSH_DATA
        gridA.blocks[actualBlockPos].points.COPY(myTranslatedBlockPoints.constrict(myTranslatedBlockPoints.domain().shrink(1)));
#else
        myTranslatedBlockPoints.COPY(gridA.blocks[actualBlockPos].points.constrict(gridA.blocks[actualBlockPos].points.domain().shrink(1)));
#endif
      }
    }

    TIMER_STOP_READ(myTimer, myTimes[T_GLOBAL_COMM][level][3*(iterationNum-1) + callNumber]);

#ifdef NONBLOCKING_ARRAYCOPY
    TIMER_START(myTimer);

    // Handle.syncNBI();
    async_copy_fence();

    TIMER_STOP_READ(myTimes[T_SYNC][level][3*(iterationNum-1) + callNumber]);
#endif
	    
#ifdef PUSH_DATA
    TIMER_START(myTimer);

    barrier();

    TIMER_STOP_READ(myTimer, myTimes[T_BARRIER][level][3*(iterationNum-1) + callNumber]);
#endif
    // end of no contiguous array buffers case (26 directions)
#endif

#elif UPDATE_BORDER_DIRECTIONS == 6
    // communicate with only 6 nearest face neighbors, but with synchronization
#ifdef CONTIGUOUS_ARRAY_BUFFERS
    // use buffers so that arraycopys are contiguous in memory
    ndarray<ndarray<double, 3, global>, 1 UNSTRIDED> myOutBuffer =
      (ndarray<ndarray<double, 3, global>,
       1 UNSTRIDED>) gridA.outBuffers[myBlockPos];
    ndarray<ndarray<double, 3, global>, 1 UNSTRIDED> myInBuffer =
      (ndarray<ndarray<double, 3, global>,
       1 UNSTRIDED>) gridA.inBuffers[myBlockPos];

    ndarray<RectDomain<3>, 1 UNSTRIDED> surroundingBlocks(RECTDOMAIN((1), (4)));
    surroundingBlocks[1] =
      RectDomain<3>(myBlockPos+POINT(-1, 0, 0), myBlockPos+POINT(2, 1, 1),
                    POINT(2,0,0));
    surroundingBlocks[2] =
      RectDomain<3>(myBlockPos+POINT(0, -1, 0), myBlockPos+POINT(1, 2, 1),
                    POINT(0,2,0));
    surroundingBlocks[3] =
      RectDomain<3>(myBlockPos+POINT(0, 0, -1), myBlockPos+POINT(1, 1, 2),
                    POINT(0,0,2));
    Point<3> numCellsPerBlock = numCellsInGridSide / numBlocksInGridSide;

    // "neighborBlockPos" is the location of a block with periodic boundary conditions (can be outside of grid)
    // "actualBlockPos" is the actual block position of a block w/o periodic boundary conditions

    for (int dim=3; dim>0; dim--) {
		
      // PACK to local buffers
      if (dim != 1) {
        TIMER_START(myTimer);

        myOutBuffer[-dim].copy(myPoints);
        myOutBuffer[dim].copy(myPoints);

        TIMER_STOP_READ(myTimer,
                        myTimes[T_PACKING][level][6*(iterationNum-1) +
                                                  2*(callNumber-1) + (dim-1)]);
      }
		
#ifndef PUSH_DATA
      TIMER_START(myTimer);

      barrier();

      TIMER_STOP_READ(myTimes[T_BARRIER][level][9*(iterationNum-1) +
                                                3*(callNumber-1) + dim]);
#endif
		
      TIMER_START(myTimer);

      // COMMUNICATE
      foreach (neighborBlockPos, surroundingBlocks[dim]) {
        Point<3> actualBlockPos =
          {{mod(neighborBlockPos[1], numBlocksInGridSide[1]),
            mod(neighborBlockPos[2], numBlocksInGridSide[2]),
            mod(neighborBlockPos[3], numBlocksInGridSide[3])}};
        Point<3> actualToNeighborVector =
          (neighborBlockPos - actualBlockPos) * numCellsPerBlock;
        int dimDir = dim * (neighborBlockPos - myBlockPos)[dim];

        ndarray<double, 3, global> buf;
#ifdef PUSH_DATA
        buf = gridA.inBuffers[actualBlockPos][-dimDir];
        buf.translate(actualToNeighborVector).COPY(myOutBuffer[dimDir]);
#else
        buf = gridA.outBuffers[actualBlockPos][-dimDir];
        myInBuffer[dimDir].COPY(buf.translate(actualToNeighborVector));
#endif
      }

      TIMER_STOP_READ(myTimer,
                      myTimes[T_GLOBAL_COMM][level][9*(iterationNum-1) +
                                                    3*(callNumber-1) + dim]);
		
#ifdef NONBLOCKING_ARRAYCOPY
      TIMER_START(myTimer);

      // Handle.syncNBI();
      async_copy_fence();

      TIMER_STOP_READ(myTimer,
                      myTimes[T_SYNC][level][9*(iterationNum-1) +
                                             3*(callNumber-1) + dim]);
#endif

#ifdef PUSH_DATA
      TIMER_START(myTimer);

      barrier();

      TIMER_STOP_READ(myTimer,
                      myTimes[T_BARRIER][level][9*(iterationNum-1) +
                                                3*(callNumber-1) + dim]);
#endif
		
      // UNPACK from local buffers
      if (dim != 1) {
        TIMER_START(myTimer);

        myPoints.copy(myInBuffer[-dim]);
        myPoints.copy(myInBuffer[dim]);

        TIMER_STOP_READ(myTimer,
                        myTimes[T_UNPACKING][level][6*(iterationNum-1) +
                                                    2*(callNumber-1) + (dim-1)]);
      }
    }
    // end of contiguous array buffers case (6 directions)

#else
    // start of the no contiguous array buffers case (6 directions)

    // "neighborBlockPos" is the location of a block with periodic boundary conditions (can be outside of grid)
    // "actualBlockPos" is the actual block position of a block w/o periodic boundary conditions

    ndarray<RectDomain<3>, 1 UNSTRIDED> surroundingBlocks(POINT(1), POINT(4));
    surroundingBlocks[1] =
      RectDomain<3>(myBlockPos+POINT(-1, 0, 0), myBlockPos+POINT(2, 1, 1),
                    POINT(2,0,0));
    surroundingBlocks[2] =
      RectDomain<3>(myBlockPos+POINT(0, -1, 0), myBlockPos+POINT(1, 2, 1),
                    POINT(0,2,0));
    surroundingBlocks[3] =
      RectDomain<3>(myBlockPos+POINT(0, 0, -1), myBlockPos+POINT(1, 1, 2),
                    POINT(0,0,2));
    Point<3> numCellsPerBlock = numCellsInGridSide / numBlocksInGridSide;

    // "localFromRectDomain" indicates (for a push) which portion of the local block needs to be copied
    RectDomain<3> localFromRectDomain = myPoints.domain().shrink(1);

    for (int dim=3; dim>0; dim--) {

#ifndef PUSH_DATA
      TIMER_START(myTimer);

      barrier();

      TIMER_STOP_READ(myTimer,
                      myTimes[T_BARRIER][level][9*(iterationNum-1) +
                                                3*(callNumber-1) + dim]);
#endif

      TIMER_START(myTimer);

      // COMMUNICATE
      foreach (neighborBlockPos, surroundingBlocks[dim]) {
        Point<3> actualBlockPos =
          {{mod(neighborBlockPos[1], numBlocksInGridSide[1]),
            mod(neighborBlockPos[2], numBlocksInGridSide[2]),
            mod(neighborBlockPos[3], numBlocksInGridSide[3])}};
        Point<3> actualToNeighborVector =
          (neighborBlockPos - actualBlockPos) * numCellsPerBlock;
		    
#ifdef PUSH_DATA
        gridA.points[actualBlockPos].translate(actualToNeighborVector).COPY(myPoints.constrict(localFromRectDomain));
#else
        // "remoteFromRectDomain" indicates (for a pull) which portion of the remote block needs to be copied
        RectDomain<3> remoteFromRectDomain =
          localFromRectDomain +
          ((neighborBlockPos - myBlockPos) * numCellsPerBlock);
        myPoints.COPY(gridA.points[actualBlockPos].translate(actualToNeighborVector).constrict(remoteFromRectDomain));
#endif		    
      }

      TIMER_STOP_READ(myTimer,
                      myTimes[T_GLOBAL_COMM][level][9*(iterationNum-1) +
                                                    3*(callNumber-1) + dim]);

      localFromRectDomain =
        localFromRectDomain.accrete(1, -dim, 1).accrete(1, dim, 1);

#ifdef NONBLOCKING_ARRAYCOPY
      TIMER_START(myTimer);

      // Handle.syncNBI();
      async_copy_fence();

      TIMER_STOP_READ(myTimer,
                      myTimes[T_SYNC][level][9*(iterationNum-1) +
                                             3*(callNumber-1) + dim]);
#endif

#ifdef PUSH_DATA
      TIMER_START(myTimer);

      barrier();

      TIMER_STOP_READ(myTimer,
                      myTimes[T_BARRIER][level][9*(iterationNum-1) +
                                                3*(callNumber-1) + dim]);
#endif
    }

#endif
    // end of the no contiguous array buffers case (6 directions)
#endif
    // end of the 6 directions update border case
  }
  // case where we are below THRESHOLD_LEVEL
  else if (MYTHREAD == 0) {
    RectDomain<3> myDomain = myPoints.domain();
	    
    // negative x direction
    myPoints.constrict(myDomain.border(1, -1, 0)).translate(POINT(numCellsInGridSide, 0, 0)).copy(myPoints);
    // positive x direction
    myPoints.constrict(myDomain.border(1, 1, 0)).translate(POINT(-numCellsInGridSide, 0, 0)).copy(myPoints);
    // negative y direction
    myPoints.constrict(myDomain.border(1, -2, 0)).translate(POINT(0, numCellsInGridSide, 0)).copy(myPoints);
    // positive y direction
    myPoints.constrict(myDomain.border(1, 2, 0)).translate(POINT(0, -numCellsInGridSide, 0)).copy(myPoints);
    // negative z direction
    myPoints.constrict(myDomain.border(1, -3, 0)).translate(POINT(0, 0, numCellsInGridSide)).copy(myPoints);
    // positive z direction
    myPoints.constrict(myDomain.border(1, 3, 0)).translate(POINT(0, 0, -numCellsInGridSide)).copy(myPoints);
  }
}

void MG::applySmoother(Grid &gridA, Grid &gridB, int level, int iterationNum) {
  Point<3> myBlockPos = Grid::myBlockPos;
  ndarray<double, 3 UNSTRIDED> myAPoints =
    (ndarray<double, 3 UNSTRIDED>) gridA.points[myBlockPos];
  ndarray<double, 3 UNSTRIDED> myBPoints =
    (ndarray<double, 3 UNSTRIDED>) gridB.points[myBlockPos];
	
  RectDomain<1> zLine = myAPoints.domain().slice(2).slice(1);
  ndarray<double, 1 UNSTRIDED> sumEdgesOfXYPlane(zLine);
  ndarray<double, 1 UNSTRIDED> sumCornersOfXYPlane(zLine);
  ndarray<double, 1 UNSTRIDED> gridBZLine(zLine);

  COUNTER_START(myCounter);
  TIMER_START(myTimer);

  // set lowest level points equal to 0
  if (level == 1) {
    // foreach (p, myAPoints.domain().shrink(1)) {
    //   myAPoints[p] = 0;
    // }
    myAPoints.constrict(myAPoints.domain().shrink(1)).set(0);
  }

#ifdef SPLIT_LOOP
  foreach2 (i, j, myAPoints.domain().slice(3).shrink(1)) {
    foreach1 (k, zLine) {
      sumEdgesOfXYPlane[k] =
        myBPoints[Pmzzs(i, j, k)] + myBPoints[Ppzzs(i, j, k)] +
        myBPoints[Pzmzs(i, j, k)] + myBPoints[Pzpzs(i, j, k)];
      sumCornersOfXYPlane[k] =
        myBPoints[Pmmzs(i, j, k)] + myBPoints[Pmpzs(i, j, k)] +
        myBPoints[Ppmzs(i, j, k)] + myBPoints[Pppzs(i, j, k)];
      gridBZLine[k] = myBPoints[i][j][k];
    }
    foreach1 (k, zLine.shrink(1)) {
      myAPoints[i][j][k] += S0 * myBPoints[i][j][k]
        + S1 * (sumEdgesOfXYPlane[k] + gridBZLine[k-1] + gridBZLine[k+1])
        + S2 * (sumCornersOfXYPlane[k] + sumEdgesOfXYPlane[k-1] +
                sumEdgesOfXYPlane[k+1]);
      // Enable the following if S3 != 0
      // + S3 * (sumCornersOfXYPlane[k-1] + sumCornersOfXYPlane[k+1]);
    }
  }
#else
  foreach (p, myAPoints.domain().slice(3).shrink(1)) {
    foreach (q, zLine) {
      Point<3> r = {{p[1], p[2], q[1]}};
      sumEdgesOfXYPlane[q] = myBPoints[Pmzz(r)] + myBPoints[Ppzz(r)] +
        myBPoints[Pzmz(r)] + myBPoints[Pzpz(r)];
      sumCornersOfXYPlane[q] = myBPoints[Pmmz(r)] + myBPoints[Pmpz(r)] +
        myBPoints[Ppmz(r)] + myBPoints[Pppz(r)];
      gridBZLine[q] = myBPoints[r];
    }
    foreach (q, zLine.shrink(1)) {
      Point<3> r = {{p[1], p[2], q[1]}};
      myAPoints[r] += S0 * myBPoints[r]
        + S1 * (sumEdgesOfXYPlane[q] + gridBZLine[q[1]-1] + gridBZLine[q[1]+1])
        + S2 * (sumCornersOfXYPlane[q] + sumEdgesOfXYPlane[q[1]-1] +
                sumEdgesOfXYPlane[q[1]+1]);
      // Enable the following if S3 != 0
      // + S3 * (sumCornersOfXYPlane[q[1]-1] + sumCornersOfXYPlane[q[1]+1]);
    }
  }
#endif // SPLIT_LOOP

  TIMER_STOP_READ(myTimer, myTimes[T_APPLY_SMOOTHER][level][iterationNum]);
  COUNTER_STOP_READ(myCounter, myCounts[T_APPLY_SMOOTHER][level][iterationNum]);
}

void MG::evaluateResidual(Grid &gridA, Grid &gridB, Grid &gridC, int level,
                          int iterationNum, int callNumber) {
  Point<3> myBlockPos = Grid::myBlockPos;
  ndarray<double, 3 UNSTRIDED> myAPoints =
    (ndarray<double, 3 UNSTRIDED>) gridA.points[myBlockPos];
  ndarray<double, 3 UNSTRIDED> myBPoints =
    (ndarray<double, 3 UNSTRIDED>) gridB.points[myBlockPos];
  ndarray<double, 3 UNSTRIDED> myCPoints =
    (ndarray<double, 3 UNSTRIDED>) gridC.points[myBlockPos];

  RectDomain<1> zLine = myCPoints.domain().slice(2).slice(1);
  ndarray<double, 1 UNSTRIDED> sumEdgesOfXYPlane(zLine);
  ndarray<double, 1 UNSTRIDED> sumCornersOfXYPlane(zLine);
	
  COUNTER_START(myCounter);
  TIMER_START(myTimer);

#ifdef SPLIT_LOOP
  foreach2 (i, j, myCPoints.domain().slice(3).shrink(1)) {
    foreach1 (k, zLine) {
      sumEdgesOfXYPlane[k] =
        myBPoints[Pmzzs(i, j, k)] + myBPoints[Ppzzs(i, j, k)] +
        myBPoints[Pzmzs(i, j, k)] + myBPoints[Pzpzs(i, j, k)];
      sumCornersOfXYPlane[k] =
        myBPoints[Pmmzs(i, j, k)] + myBPoints[Pmpzs(i, j, k)] +
        myBPoints[Ppmzs(i, j, k)] + myBPoints[Pppzs(i, j, k)];
    }
    foreach1 (k, zLine.shrink(1)) {
      myCPoints[i][j][k] = myAPoints[i][j][k]
        - (A0 * myBPoints[i][j][k]
           // Enable the following if A1 != 0
           // + A1 * (sumEdgesOfXYPlane[k] + myBPoints[Pzzms(i, j, k)] +
           //         myBPoints[Pzzps(i, j, k)])
           + A2 * (sumCornersOfXYPlane[k] + sumEdgesOfXYPlane[k-1] +
                   sumEdgesOfXYPlane[k+1])
           + A3 * (sumCornersOfXYPlane[k-1] + sumCornersOfXYPlane[k+1]));
    }
  }
#else
  foreach (p, myCPoints.domain().slice(3).shrink(1)) {
    foreach (q, zLine) {
      Point<3> r = {{p[1], p[2], q[1]}};
      sumEdgesOfXYPlane[q] = myBPoints[Pmzz(r)] + myBPoints[Ppzz(r)] +
        myBPoints[Pzmz(r)] + myBPoints[Pzpz(r)];
      sumCornersOfXYPlane[q] = myBPoints[Pmmz(r)] + myBPoints[Pmpz(r)] +
        myBPoints[Ppmz(r)] + myBPoints[Pppz(r)];
    }
    foreach (q, zLine.shrink(1)) {
      Point<3> r = {{p[1], p[2], q[1]}};
      myCPoints[r] = myAPoints[r]
        - (A0 * myBPoints[r]
           // Enable the following if A1 != 0
           // + A1 * (sumEdgesOfXYPlane[q] + myBPoints[Pzzm(r)] +
           //         myBPoints[Pzzp(r)])
           + A2 * (sumCornersOfXYPlane[q] + sumEdgesOfXYPlane[q[1]-1] +
                   sumEdgesOfXYPlane[q[1]+1])
           + A3 * (sumCornersOfXYPlane[q[1]-1] + sumCornersOfXYPlane[q[1]+1]));
    }
  }
#endif // SPLIT_LOOP

  if (level != startLevel) {
    TIMER_STOP_READ(myTimer,
                    myTimes[T_EVALUATE_RESIDUAL][level][iterationNum]);
    COUNTER_STOP_READ(myCounter,
                      myCounts[T_EVALUATE_RESIDUAL][level][iterationNum]);
  }
  else {
    TIMER_STOP_READ(myTimer,
                    myTimes[T_EVALUATE_RESIDUAL][level][2*iterationNum
                                                        + callNumber - 1]);
    COUNTER_STOP_READ(myCounter,
                      myCounts[T_EVALUATE_RESIDUAL][level][2*iterationNum
                                                           + callNumber - 1]);
  }
}

void MG::coarsen(Grid &gridA, Grid &gridB, int level, int iterationNum) {
  Point<3> myBlockPos = Grid::myBlockPos;
  ndarray<double, 3 UNSTRIDED> myBPoints;
  ndarray<double, 3 UNSTRIDED> myAPoints =
    (ndarray<double, 3 UNSTRIDED>) gridA.points[myBlockPos];

  // In this case, if the array is distributed, it stays distributed.  Similarly, if it's only on proc 0, it stays on proc 0
  if (level != THRESHOLD_LEVEL) {
    myBPoints = (ndarray<double, 3 UNSTRIDED>) gridB.points[myBlockPos];
  }
  // This case is when we go from a distributed array to the entire array on proc 0
  else {
    RectDomain<3> rd(POINT((myAPoints.domain().min()[1]+1)/2 - 1,
                           (myAPoints.domain().min()[2]+1)/2 - 1,
                           (myAPoints.domain().min()[3]+1)/2 - 1),
                     POINT((myAPoints.domain().max()[1]-1)/2 + 2,
                           (myAPoints.domain().max()[2]-1)/2 + 2,
                           (myAPoints.domain().max()[3]-1)/2 + 2));
    myBPoints = ndarray<double, 3 UNSTRIDED>(rd);
  }

  RectDomain<1> zLine = myBPoints.domain().slice(2).slice(1);
  double sumEdgesOfCenterYZPlane, sumCornersOfCenterYZPlane;
  ndarray<double, 1 UNSTRIDED> sumEdgesOfLowerYZPlane(zLine.shrink(1, -1));
  ndarray<double, 1 UNSTRIDED> sumCornersOfLowerYZPlane(zLine.shrink(1, -1));

  COUNTER_START(myCounter);
  TIMER_START(myTimer);

#ifdef SPLIT_LOOP
  foreach2 (i, j, myBPoints.domain().slice(3).shrink(1)) {
    foreach1 (k, zLine.shrink(1, -1)) {
      int a = 2*i+1, b = 2*j+1, c = 2*k+1;
      sumEdgesOfLowerYZPlane[k] =
        myAPoints[Pmzms(a, b, c)] + myAPoints[Pzmms(a, b, c)] +
        myAPoints[Ppzms(a, b, c)] + myAPoints[Pzpms(a, b, c)];
      sumCornersOfLowerYZPlane[k] =
        myAPoints[Pmmms(a, b, c)] + myAPoints[Ppmms(a, b, c)] +
        myAPoints[Pmpms(a, b, c)] + myAPoints[Pppms(a, b, c)];
    }
    foreach1 (k, zLine.shrink(1)) {
      int a = 2*i+1, b = 2*j+1, c = 2*k+1;
      sumEdgesOfCenterYZPlane =
        myAPoints[Pmzzs(a, b, c)] + myAPoints[Pzmzs(a, b, c)] +
        myAPoints[Ppzzs(a, b, c)] + myAPoints[Pzpzs(a, b, c)];
      sumCornersOfCenterYZPlane =
        myAPoints[Pmmzs(a, b, c)] + myAPoints[Ppmzs(a, b, c)] +
        myAPoints[Pmpzs(a, b, c)] + myAPoints[Pppzs(a, b, c)];
      myBPoints[i][j][k] = P0 * myAPoints[a][b][c]
        + P1 * (myAPoints[Pzzms(a, b, c)] + myAPoints[Pzzps(a, b, c)] +
                sumEdgesOfCenterYZPlane)
        + P2 * (sumEdgesOfLowerYZPlane[k] + sumEdgesOfLowerYZPlane[k+1] +
                sumCornersOfCenterYZPlane)
        + P3 * (sumCornersOfLowerYZPlane[k] + sumCornersOfLowerYZPlane[k+1]);
    }
  }
#else
  foreach (p, myBPoints.domain().slice(3).shrink(1)) {
    foreach (q, zLine.shrink(1, -1)) {
      Point<3> s = {{2*p[1]+1, 2*p[2]+1, 2*q[1]+1}};
      sumEdgesOfLowerYZPlane[q] = myAPoints[Pmzm(s)] + myAPoints[Pzmm(s)] +
        myAPoints[Ppzm(s)] + myAPoints[Pzpm(s)];
      sumCornersOfLowerYZPlane[q] = myAPoints[Pmmm(s)] + myAPoints[Ppmm(s)] +
        myAPoints[Pmpm(s)] + myAPoints[Pppm(s)];
    }
    foreach (q, zLine.shrink(1)) {
      Point<3> r = {{p[1], p[2], q[1]}};
      Point<3> s = {{2*r[1]+1, 2*r[2]+1, 2*r[3]+1}};
      sumEdgesOfCenterYZPlane = myAPoints[Pmzz(s)] + myAPoints[Pzmz(s)] +
        myAPoints[Ppzz(s)] + myAPoints[Pzpz(s)];
      sumCornersOfCenterYZPlane = myAPoints[Pmmz(s)] + myAPoints[Ppmz(s)] +
        myAPoints[Pmpz(s)] + myAPoints[Pppz(s)];
      myBPoints[r] = P0 * myAPoints[s]
        + P1 * (myAPoints[Pzzm(s)] + myAPoints[Pzzp(s)] + sumEdgesOfCenterYZPlane)
        + P2 * (sumEdgesOfLowerYZPlane[q] + sumEdgesOfLowerYZPlane[q+1] +
                sumCornersOfCenterYZPlane)
        + P3 * (sumCornersOfLowerYZPlane[q] + sumCornersOfLowerYZPlane[q+1]);
    }
  }
#endif // SPLIT_LOOP

  TIMER_STOP_READ(myTimer, myTimes[T_COARSEN][level][iterationNum]);
  COUNTER_STOP_READ(myCounter, myCounts[T_COARSEN][level][iterationNum]);

  if (level == THRESHOLD_LEVEL) {
    TIMER_START(myTimer);

    gridB.myBufferForProc0.copy(myBPoints.constrict(myBPoints.domain().shrink(1)));
    gridB.myRemoteBufferOnProc0.copy(gridB.myBufferForProc0);

    TIMER_STOP_READ(myTimer,
                    myTimes[T_THRESHOLD_COMM][THRESHOLD_LEVEL][2*(iterationNum-1)+2]);
    TIMER_START(myTimer);

    barrier();

    if (MYTHREAD == 0) {
      foreach (p, gridB.localBuffersOnProc0.domain()) {
        gridB.points[POINT(0, 0, 0)].copy(gridB.localBuffersOnProc0[p].constrict(gridB.localBuffersOnProc0[p].domain().shrink(1)));
      }
    }

    TIMER_STOP_READ(myTimer,
                    myTimes[T_BARRIER][THRESHOLD_LEVEL][-2*iterationNum+2]);
  }
}

void MG::prolongate(Grid &gridA, Grid &gridB, int level, int iterationNum) {
  Point<3> myBlockPos = Grid::myBlockPos;
  ndarray<double, 3 UNSTRIDED> myAPoints;
  ndarray<double, 3 UNSTRIDED> myBPoints =
    (ndarray<double, 3 UNSTRIDED>) gridB.points[myBlockPos];

  // In this case, if the array is distributed, it stays distributed.  Similarly, if it's only on proc 0, it stays on proc 0
  if (level != THRESHOLD_LEVEL) {
    myAPoints = (ndarray<double, 3 UNSTRIDED>) gridA.points[myBlockPos];
  }
  // This case is when we go from the entire array on proc 0 to a distributed array
  else {
    RectDomain<3> rd(POINT((myBPoints.domain().min()[1]+1)/2 - 1,
                           (myBPoints.domain().min()[2]+1)/2 - 1,
                           (myBPoints.domain().min()[3]+1)/2 - 1),
                     POINT((myBPoints.domain().max()[1]-1)/2 + 2,
                           (myBPoints.domain().max()[2]-1)/2 + 2,
                           (myBPoints.domain().max()[3]-1)/2 + 2));
    myAPoints = ndarray<double, 3 UNSTRIDED>(rd);

    TIMER_START(myTimer);

    if (MYTHREAD == 0) {
      foreach (p, gridA.localBuffersOnProc0.domain()) {
        gridA.localBuffersOnProc0[p].copy(gridA.points[POINT(0, 0, 0)]);
      }
    }

    barrier();

    TIMER_STOP_READ(myTimer,
                    myTimes[T_BARRIER][THRESHOLD_LEVEL][-2*iterationNum+1]);
    TIMER_START(myTimer);

    gridA.myBufferForProc0.copy(gridA.myRemoteBufferOnProc0);
    myAPoints.copy(gridA.myBufferForProc0);

    TIMER_STOP_READ(myTimer,
                    myTimes[T_THRESHOLD_COMM][THRESHOLD_LEVEL][2*(iterationNum-1)+1]);
  }

  RectDomain<1> zLine = myAPoints.domain().slice(2).slice(1);
  ndarray<double, 1 UNSTRIDED> xSumOfLowerXYPlane(zLine);
  ndarray<double, 1 UNSTRIDED> ySumOfLowerXYPlane(zLine);
  ndarray<double, 1 UNSTRIDED> sumOfLowerXYPlane(zLine);

  COUNTER_START(myCounter);
  TIMER_START(myTimer);

  if (level != startLevel) {
#ifdef SPLIT_LOOP
    foreach2 (i, j, myAPoints.domain().slice(3).shrink(1,1).shrink(1,2)) {
      foreach1 (k, zLine) {
        xSumOfLowerXYPlane[k] = myAPoints[i][j][k] + myAPoints[Ppzzs(i, j, k)];
        ySumOfLowerXYPlane[k] = myAPoints[i][j][k] + myAPoints[Pzpzs(i, j, k)];
        sumOfLowerXYPlane[k] = xSumOfLowerXYPlane[k] +
          myAPoints[Pzpzs(i, j, k)] + myAPoints[Pppzs(i, j, k)];
      }
      foreach1 (k, zLine.shrink(1,1)) {
        int a = 2*(i+1), b = 2*(j+1), c = 2*(k+1);
        myBPoints[Pmmms(a, b, c)] = Q0 * myAPoints[i][j][k];
        myBPoints[Pzmms(a, b, c)] = Q1 * (xSumOfLowerXYPlane[k]);
        myBPoints[Pmzms(a, b, c)] = Q1 * (ySumOfLowerXYPlane[k]);
        myBPoints[Pzzms(a, b, c)] = Q2 * (sumOfLowerXYPlane[k]);
        myBPoints[Pmmzs(a, b, c)] = Q1 * (myAPoints[i][j][k] +
                                         myAPoints[Pzzps(i, j, k)]);
        myBPoints[Pzmzs(a, b, c)] = Q2 * (xSumOfLowerXYPlane[k] +
                                          xSumOfLowerXYPlane[k+1]);
        myBPoints[Pmzzs(a, b, c)] = Q2 * (ySumOfLowerXYPlane[k] +
                                          ySumOfLowerXYPlane[k+1]);
        myBPoints[a][b][c] = Q3 * (sumOfLowerXYPlane[k] +
                                   sumOfLowerXYPlane[k+1]);
      }
    }
#else
    foreach (p, myAPoints.domain().slice(3).shrink(1,1).shrink(1,2)) {
      foreach (q, zLine) {
        Point<3> r = {{p[1], p[2], q[1]}};
        xSumOfLowerXYPlane[q] = myAPoints[r] + myAPoints[Ppzz(r)];
        ySumOfLowerXYPlane[q] = myAPoints[r] + myAPoints[Pzpz(r)];
        sumOfLowerXYPlane[q] = xSumOfLowerXYPlane[q] + myAPoints[Pzpz(r)] +
          myAPoints[Pppz(r)];
      }
      foreach (q, zLine.shrink(1,1)) {
        Point<3> r = {{p[1], p[2], q[1]}};
        Point<3> s = {{2*(r[1]+1), 2*(r[2]+1), 2*(r[3]+1)}};
        myBPoints[Pmmm(s)] = Q0 * myAPoints[r];
        myBPoints[Pzmm(s)] = Q1 * (xSumOfLowerXYPlane[q]);
        myBPoints[Pmzm(s)] = Q1 * (ySumOfLowerXYPlane[q]);
        myBPoints[Pzzm(s)] = Q2 * (sumOfLowerXYPlane[q]);
        myBPoints[Pmmz(s)] = Q1 * (myAPoints[r] + myAPoints[Pzzp(r)]);
        myBPoints[Pzmz(s)] = Q2 * (xSumOfLowerXYPlane[q] +
                                  xSumOfLowerXYPlane[q[1]+1]);
        myBPoints[Pmzz(s)] = Q2 * (ySumOfLowerXYPlane[q] +
                                  ySumOfLowerXYPlane[q[1]+1]);
        myBPoints[s] = Q3 * (sumOfLowerXYPlane[q] + sumOfLowerXYPlane[q[1]+1]);
      }
    }
#endif // SPLIT_LOOP
  }
  else {
#ifdef SPLIT_LOOP
    foreach2 (i, j, myAPoints.domain().slice(3).shrink(1,1).shrink(1,2)) {
      foreach1 (k, zLine) {
        xSumOfLowerXYPlane[k] = myAPoints[i][j][k] + myAPoints[Ppzzs(i, j, k)];
        ySumOfLowerXYPlane[k] = myAPoints[i][j][k] + myAPoints[Pzpzs(i, j, k)];
        sumOfLowerXYPlane[k] = xSumOfLowerXYPlane[k] +
          myAPoints[Pzpzs(i, j, k)] + myAPoints[Pppzs(i, j, k)];
      }
      foreach1 (k, zLine.shrink(1,1)) {
        int a = 2*(i+1), b = 2*(j+1), c = 2*(k+1);
        myBPoints[Pmmms(a, b, c)] += Q0 * myAPoints[i][j][k];
        myBPoints[Pzmms(a, b, c)] += Q1 * (xSumOfLowerXYPlane[k]);
        myBPoints[Pmzms(a, b, c)] += Q1 * (ySumOfLowerXYPlane[k]);
        myBPoints[Pzzms(a, b, c)] += Q2 * (sumOfLowerXYPlane[k]);
        myBPoints[Pmmzs(a, b, c)] += Q1 * (myAPoints[i][j][k] +
                                           myAPoints[Pzzps(i, j, k)]);
        myBPoints[Pzmzs(a, b, c)] += Q2 * (xSumOfLowerXYPlane[k] +
                                           xSumOfLowerXYPlane[k+1]);
        myBPoints[Pmzzs(a, b, c)] += Q2 * (ySumOfLowerXYPlane[k] +
                                           ySumOfLowerXYPlane[k+1]);
        myBPoints[a][b][c] += Q3 * (sumOfLowerXYPlane[k] +
                                    sumOfLowerXYPlane[k+1]);
      }
    }
  }
#else
    foreach (p, myAPoints.domain().slice(3).shrink(1,1).shrink(1,2)) {
      foreach (q, zLine) {
        Point<3> r = {{p[1], p[2], q[1]}};
        xSumOfLowerXYPlane[q] = myAPoints[r] + myAPoints[Ppzz(r)];
        ySumOfLowerXYPlane[q] = myAPoints[r] + myAPoints[Pzpz(r)];
        sumOfLowerXYPlane[q] = xSumOfLowerXYPlane[q] + myAPoints[Pzpz(r)] +
          myAPoints[Pppz(r)];
      }
      foreach (q, zLine.shrink(1,1)) {
        Point<3> r = {{p[1], p[2], q[1]}};
        Point<3> s = {{2*(r[1]+1), 2*(r[2]+1), 2*(r[3]+1)}};
        myBPoints[Pmmm(s)] += Q0 * myAPoints[r];
        myBPoints[Pzmm(s)] += Q1 * (xSumOfLowerXYPlane[q]);
        myBPoints[Pmzm(s)] += Q1 * (ySumOfLowerXYPlane[q]);
        myBPoints[Pzzm(s)] += Q2 * (sumOfLowerXYPlane[q]);
        myBPoints[Pmmz(s)] += Q1 * (myAPoints[r] + myAPoints[Pzzp(r)]);
        myBPoints[Pzmz(s)] += Q2 * (xSumOfLowerXYPlane[q] +
                                   xSumOfLowerXYPlane[q[1]+1]);
        myBPoints[Pmzz(s)] += Q2 * (ySumOfLowerXYPlane[q] +
                                   ySumOfLowerXYPlane[q[1]+1]);
        myBPoints[s] += Q3 * (sumOfLowerXYPlane[q] + sumOfLowerXYPlane[q[1]+1]);
      }
    }
  }
#endif // SPLIT_LOOP

  TIMER_STOP_READ(myTimer, myTimes[T_PROLONGATE][level][iterationNum]);
  COUNTER_STOP_READ(myCounter, myCounts[T_PROLONGATE][level][iterationNum]);
}

void MG::vCycle(ndarray<Grid *, 1 UNSTRIDED> residualGrids,
                ndarray<Grid *, 1 UNSTRIDED> correctionGrids,
                Grid &rhsGrid, int iterationNum) {
  int level;

  // descending the V-cycle
  for (level=startLevel; level > 1; level--) {
    coarsen(*(residualGrids[level]), *(residualGrids[level-1]),
            level, iterationNum);
    updateBorder(*(residualGrids[level-1]), level-1, iterationNum, 1);
  }

  // smoothing at the bottom of the V-cycle
  level = 1;
  applySmoother(*(correctionGrids[level]), *(residualGrids[level]),
                level, iterationNum);
  updateBorder(*(correctionGrids[level]), level, iterationNum, 2);

  // ascending the V-cycle
  for (level=2; level <= startLevel; level++) {
    prolongate(*(correctionGrids[level-1]), *(correctionGrids[level]),
               level, iterationNum);
    if (level != startLevel) {
      evaluateResidual(*(residualGrids[level]), *(correctionGrids[level]),
                       *(residualGrids[level]), level, iterationNum, 1);
    }
    else {
      evaluateResidual(rhsGrid, *(correctionGrids[level]),
                       *(residualGrids[level]), level, iterationNum, 1);
    }
    updateBorder(*(residualGrids[level]), level, iterationNum, 2);
    applySmoother(*(correctionGrids[level]), *(residualGrids[level]),
                  level, iterationNum);
    updateBorder(*(correctionGrids[level]), level, iterationNum, 3);
  }
}

void MG::resetProfile() {
#ifdef TIMERS_ENABLED
  myTimer.reset();

  foreach (timer, myTimes.domain()) {
    foreach (level, myTimes[timer].domain()) {
      foreach (reading, myTimes[timer][level].domain()) {
        myTimes[timer][level][reading] = 0;
      }
    }
  }
#ifdef COUNTERS_ENABLED
  myCounter.clear();

  foreach (timer, myCounts.domain()) {
    foreach (level, myCounts[timer].domain()) {
      foreach (reading, myCounts[timer][level].domain()) {
        myCounts[timer][level][reading] = 0;
      }
    }
  }
#endif
#endif
}

void MG::printSummary() {
  if (MYTHREAD == 0) {
    println("\nSUMMARY- min, mean, max for each component");
    println("All times in seconds.\n");
  }

  for (int timerIdx=1; timerIdx<=numTimers; timerIdx++) {
    if (MYTHREAD == 0) {
      switch (timerIdx) {
      case T_L2NORM:
        println("L2 NORM:");
        break;
      case T_APPLY_SMOOTHER:
        println("APPLY SMOOTHER:");
        break;
      case T_EVALUATE_RESIDUAL:
        println("EVALUATE RESIDUAL:");
        break;
      case T_COARSEN:
        println("COARSEN:");
        break;
      case T_PROLONGATE:
        println("PROLONGATE:");
        break;
      case T_GLOBAL_COMM:
        println("GLOBAL COMMUNICATION:");
        break;
      case T_THRESHOLD_COMM:
        println("THRESHOLD LEVEL COMMUNICATION:");
        break;
      case T_BARRIER:
        println("BARRIER:");
        break;
#ifdef CONTIGUOUS_ARRAY_BUFFERS
      case T_PACKING:
        println("PACKING:");
        break;
      case T_UNPACKING:
        println("UNPACKING:");
        break;
#endif
#ifdef NONBLOCKING_ARRAYCOPY
      case T_SYNC:
        println("SYNC:");
        break;
#endif
      }
    }

#ifdef TIMERS_ENABLED
    double totalComponentTime = 0.0;
    foreach (level, myTimes[timerIdx].domain()) {
      foreach (timing, myTimes[timerIdx][level].domain()) {
        totalComponentTime += myTimes[timerIdx][level][timing];
      }
    }

    double minTotalComponentTime = reduce::min(totalComponentTime);
    double sumTotalComponentTime = reduce::add(totalComponentTime);
    double maxTotalComponentTime = reduce::max(totalComponentTime);

    if (MYTHREAD == 0) {
      println("Time: " << minTotalComponentTime << ", " <<
              (sumTotalComponentTime/THREADS) << ", " <<
              maxTotalComponentTime);
    }
#endif
#ifdef COUNTERS_ENABLED
    if (timerIdx <= numCounters) {
      long totalComponentCount = 0;
      foreach (level, myCounts[timerIdx].domain()) {
        foreach (count, myCounts[timerIdx][level].domain()) {
          totalComponentCount += myCounts[timerIdx][level][count];
        }
      }
		
      long minTotalComponentCount = reduce::min(totalComponentCount);
      long sumTotalComponentCount = reduce::add(totalComponentCount);
      long maxTotalComponentCount = reduce::max(totalComponentCount);

      if (MYTHREAD == 0) {
        println("Count: " << minTotalComponentCount << ", " <<
                (sumTotalComponentCount/THREADS) << ", " <<
                maxTotalComponentCount);
      }
    }
#endif
  }
}

void MG::printProfile() {
  if (MYTHREAD == 0) {
    println("\n\nFULL PROFILE");
    println("All times in seconds.\n");
  }

  for (int timerIdx=1; timerIdx<=numTimers; timerIdx++) {
    if (MYTHREAD == 0) {
      switch (timerIdx) {
      case T_L2NORM:
        println("L2 NORM:");
        break;
      case T_APPLY_SMOOTHER:
        println("APPLY SMOOTHER:");
        break;
      case T_EVALUATE_RESIDUAL:
        println("EVALUATE RESIDUAL:");
        break;
      case T_COARSEN:
        println("COARSEN:");
        break;
      case T_PROLONGATE:
        println("PROLONGATE:");
        break;
      case T_GLOBAL_COMM:
        println("GLOBAL COMMUNICATION:");
        break;
      case T_THRESHOLD_COMM:
        println("THRESHOLD LEVEL COMMUNICATION:");
        break;
      case T_BARRIER:
        println("BARRIER:");
        break;
#ifdef CONTIGUOUS_ARRAY_BUFFERS
      case T_PACKING:
        println("PACKING:");
        break;
      case T_UNPACKING:
        println("UNPACKING:");
        break;
#endif
#ifdef NONBLOCKING_ARRAYCOPY
      case T_SYNC:
        println("SYNC:");
        break;
#endif
      }
    }

    // we limit the level to just "startLevel", but that can be changed
    int levelIdxMin = myTimes[timerIdx].domain().min()[1];
    int levelIdxMax = myTimes[timerIdx].domain().max()[1];
    for (int levelIdx=levelIdxMax; levelIdx<=levelIdxMax; levelIdx++) {
      if (MYTHREAD == 0) {
        println("LEVEL " << levelIdx << ": ");
      }

#ifdef TIMERS_ENABLED
      if ((timerIdx <= numCounters) && (levelIdx < THRESHOLD_LEVEL)) {
        // computation code on proc 0 only
        if (MYTHREAD == 0) {
          double lmin =
            myTimes[timerIdx][levelIdx][myTimes[timerIdx][levelIdx].domain().min()];
          double lmax = lmin;
          double lsum = 0;
          int numReadingsPerProc = 0;
			
          foreach (reading, myTimes[timerIdx][levelIdx].domain()) {
            double value = myTimes[timerIdx][levelIdx][reading];
            if (value < lmin) {
              lmin = value;
            }
            if (value > lmax) {
              lmax = value;
            }
            lsum += value;
            numReadingsPerProc++;
          }

          println("Num Readings On Proc 0:\t" << numReadingsPerProc);
          println("Min Time On Proc 0:\t" << lmin);
          double lmean = lsum/numReadingsPerProc;
          println("Mean Time On Proc 0:\t" << lmean);
          println("Max Time On Proc 0:\t" << lmax << "\n");
        }
      }
#if UPDATE_BORDER_DIRECTIONS == 6
      else if ((timerIdx == T_GLOBAL_COMM) || (timerIdx == T_SYNC)) {
        // global communication or syncs
        double lmin1 =
          myTimes[timerIdx][levelIdx][myTimes[timerIdx][levelIdx].domain().min()];
        double lmax1 = lmin1;
        double lsum1 = 0;
        int numReadingsPerProc1 = 0;
        double lmin2 =
          myTimes[timerIdx][levelIdx][myTimes[timerIdx][levelIdx].domain().min()[1]
                                      + 1];
        double lmax2 = lmin2;
        double lsum2 = 0;
        int numReadingsPerProc2 = 0;
        double lmin3 =
          myTimes[timerIdx][levelIdx][myTimes[timerIdx][levelIdx].domain().min()[1]
                                      + 2];
        double lmax3 = lmin3;
        double lsum3 = 0;
        int numReadingsPerProc3 = 0;
		
        foreach (reading, myTimes[timerIdx][levelIdx].domain()) {
          double value = myTimes[timerIdx][levelIdx][reading];
          if (mod(reading[1], 3) == 1) {
            if (value < lmin1) {
              lmin1 = value;
            }
            if (value > lmax1) {
              lmax1 = value;
            }
            lsum1 += value;
            numReadingsPerProc1++;
          }
          if (mod(reading[1], 3) == 2) {
            if (value < lmin2) {
              lmin2 = value;
            }
            if (value > lmax2) {
              lmax2 = value;
            }
            lsum2 += value;
            numReadingsPerProc2++;
          }
          if (mod(reading[1], 3) == 0) {
            if (value < lmin3) {
              lmin3 = value;
            }
            if (value > lmax3) {
              lmax3 = value;
            }
            lsum3 += value;
            numReadingsPerProc3++;
          }
        }
		    
        double gmin1 = reduce::min(lmin1);
        double gmax1 = reduce::max(lmax1);
        double gsum1 = reduce::add(lsum1);
        double gmin2 = reduce::min(lmin2);
        double gmax2 = reduce::max(lmax2);
        double gsum2 = reduce::add(lsum2);
        double gmin3 = reduce::min(lmin3);
        double gmax3 = reduce::max(lmax3);
        double gsum3 = reduce::add(lsum3);
		    
        if (MYTHREAD == 0) {
          println("YZ-plane:\nNum Readings Per Proc:\t" << numReadingsPerProc1);
          println("Min Time Across Procs:\t" << gmin1);
          double gmean1 = gsum1/(numReadingsPerProc1*THREADS);
          println("Mean Time Across Procs:\t" << gmean1);
          println("Max Time Across Procs:\t" << gmax1);
          println("XZ-plane:\nNum Readings Per Proc:\t" << numReadingsPerProc2);
          println("Min Time Across Procs:\t" << gmin2);
          double gmean2 = gsum2/(numReadingsPerProc2*THREADS);
          println("Mean Time Across Procs:\t" << gmean2);
          println("Max Time Across Procs:\t" << gmax2);
          println("XY-plane:\nNum Readings Per Proc:\t" << numReadingsPerProc3);
          println("Min Time Across Procs:\t" << gmin3);
          double gmean3 = gsum3/(numReadingsPerProc3*THREADS);
          println("Mean Time Across Procs:\t" << gmean3);
          println("Max Time Across Procs:\t" << gmax3 << "\n");
        }		
      }
#ifdef CONTIGUOUS_ARRAY_BUFFERS
      else if ((timerIdx == T_PACKING) || (timerIdx == T_UNPACKING)) {
        // packing and unpacking
        double lmin1 =
          myTimes[timerIdx][levelIdx][myTimes[timerIdx][levelIdx].domain().min()];
        double lmax1 = lmin1;
        double lsum1 = 0;
        int numReadingsPerProc1 = 0;
        double lmin2 =
          myTimes[timerIdx][levelIdx][myTimes[timerIdx][levelIdx].domain().min()[1]
                                      + 1];
        double lmax2 = lmin2;
        double lsum2 = 0;
        int numReadingsPerProc2 = 0;
		
        foreach (reading, myTimes[timerIdx][levelIdx].domain()) {
          double value = myTimes[timerIdx][levelIdx][reading];
          if (mod(reading[1], 2) == 1) {
            if (value < lmin1) {
              lmin1 = value;
            }
            if (value > lmax1) {
              lmax1 = value;
            }
            lsum1 += value;
            numReadingsPerProc1++;
          }
          if (mod(reading[1], 2) == 0) {
            if (value < lmin2) {
              lmin2 = value;
            }
            if (value > lmax2) {
              lmax2 = value;
            }
            lsum2 += value;
            numReadingsPerProc2++;
          }
        }
		    
        double gmin1 = reduce::min(lmin1);
        double gmax1 = reduce::max(lmax1);
        double gsum1 = reduce::add(lsum1);
        double gmin2 = reduce::min(lmin2);
        double gmax2 = reduce::max(lmax2);
        double gsum2 = reduce::add(lsum2);
		    
        if (MYTHREAD == 0) {
          println("XZ-plane:\nNum Readings Per Proc:\t" << numReadingsPerProc1);
          println("Min Time Across Procs:\t" << gmin1);
          double gmean1 = gsum1/(numReadingsPerProc1*THREADS);
          println("Mean Time Across Procs:\t" << gmean1);
          println("Max Time Across Procs:\t" << gmax1);
          println("XY-plane:\nNum Readings Per Proc:\t" << numReadingsPerProc2);
          println("Min Time Across Procs:\t" << gmin2);
          double gmean2 = gsum2/(numReadingsPerProc2*THREADS);
          println("Mean Time Across Procs:\t" << gmean2);
          println("Max Time Across Procs:\t" << gmax2 << "\n");
        }
      }
#endif
#endif
      else if (timerIdx == T_BARRIER) {
        // barriers
        if (levelIdx == THRESHOLD_LEVEL) {
          // barriers for changing from 1 to many procs or vice versa

          RectDomain<1> domainRange(POINT(-2*numIterations+1), POINT(1));
          ndarray<double, 1> minArray(domainRange);
          ndarray<double, 1> sumArray(domainRange);
          ndarray<double, 1> maxArray(domainRange);

          foreach (p, domainRange) {
            minArray[p] = reduce::min(myTimes[timerIdx][levelIdx][p]);
            sumArray[p] = reduce::add(myTimes[timerIdx][levelIdx][p]);
            maxArray[p] = reduce::max(myTimes[timerIdx][levelIdx][p]);
          }
			
          double sumMins1 = 0;
          double sumSums1 = 0;
          double sumMaxs1 = 0;
          double sumMins2 = 0;
          double sumSums2 = 0;
          double sumMaxs2 = 0;
          foreach (p, domainRange) {
            if (mod(p[1], 2) == 1) {
              sumMins1 += minArray[p];
              sumSums1 += sumArray[p];
              sumMaxs1 += maxArray[p];
            }
            else if (mod(p[1], 2) == 0) {
              sumMins2 += minArray[p];
              sumSums2 += sumArray[p];
              sumMaxs2 += maxArray[p];
            }
          }
			
          double meanMins1 = sumMins1/(domainRange.size()/2);
          double meanMeans1 = (sumSums1/((domainRange.size()/2)*THREADS));
          double meanMaxs1 = sumMaxs1/(domainRange.size()/2);
          double meanMins2 = sumMins2/(domainRange.size()/2);
          double meanMeans2 = (sumSums2/((domainRange.size()/2)*THREADS));
          double meanMaxs2 = sumMaxs2/(domainRange.size()/2);

          if (MYTHREAD == 0) {
            println("Threshold Barrier for Prolongate (one proc -> many procs):");
            println("Num Readings Per Proc:\t" << (domainRange.size()/2));
            println("Mean Min Wait Time:\t" << meanMins1);
            println("Mean Mean Wait Time:\t" << meanMeans1);
            println("Mean Max Wait Time:\t" << meanMaxs1 << "\n");
            println("Threshold Barrier for Coarsen (many procs -> one proc):");
            println("Num Readings Per Proc:\t" << (domainRange.size()/2));
            println("Mean Min Wait Time:\t" << meanMins2);
            println("Mean Mean Wait Time:\t" << meanMeans2);
            println("Mean Max Wait Time:\t" << meanMaxs2 << "\n");
          }
        }
		    
        Point<1> domainMin;
        if (levelIdx == startLevel) {
          domainMin = POINT(-2);//broadcast [-2] from 0;
        }
        else {
          domainMin = POINT(1);//broadcast [1] from 0;
        }
        Point<1> domainMax = myTimes[timerIdx][levelIdx].domain().max();
        // = broadcast myTimes[timerIdx][levelIdx].domain().max() from 0;

        RectDomain<1> domainRange(domainMin, domainMax+POINT(1));
        ndarray<double, 1> minArray(domainRange);
        ndarray<double, 1> sumArray(domainRange);
        ndarray<double, 1> maxArray(domainRange);

        foreach (p, domainRange) {
          minArray[p] = reduce::min(myTimes[timerIdx][levelIdx][p]);
          sumArray[p] = reduce::add(myTimes[timerIdx][levelIdx][p]);
          maxArray[p] = reduce::max(myTimes[timerIdx][levelIdx][p]);
        }

        double sumMins = 0;
        double sumSums = 0;
        double sumMaxs = 0;
        foreach (p, domainRange) {
          sumMins += minArray[p];
          sumSums += sumArray[p];
          sumMaxs += maxArray[p];
        }

        double meanMins = sumMins/domainRange.size();
        double meanMeans = (sumSums/(domainRange.size()*THREADS));
        double meanMaxs = sumMaxs/domainRange.size();

        if (MYTHREAD == 0) {
          println("Num Readings Per Proc:\t" << domainRange.size());
          println("Mean Min Wait Time:\t" << meanMins);
          println("Mean Mean Wait Time:\t" << meanMeans);
          println("Mean Max Wait Time:\t" << meanMaxs << "\n");
        }
      }
      else {
        double lmin =
          myTimes[timerIdx][levelIdx][myTimes[timerIdx][levelIdx].domain().min()];
        double lmax = lmin;
        double lsum = 0;
        int numReadingsPerProc = 0;
		    
        foreach (reading, myTimes[timerIdx][levelIdx].domain()) {
          double value = myTimes[timerIdx][levelIdx][reading];
          if (value < lmin) {
            lmin = value;
          }
          if (value > lmax) {
            lmax = value;
          }
          lsum += value;
          numReadingsPerProc++;
        }
		    
        double gmin = reduce::min(lmin);
        double gmax = reduce::max(lmax);
        double gsum = reduce::add(lsum);
		    
        if (MYTHREAD == 0) {
          println("Num Readings Per Proc:\t" << numReadingsPerProc);
          println("Min Time Across Procs:\t" << gmin);
          double gmean = gsum/(numReadingsPerProc*THREADS);
          println("Mean Time Across Procs:\t" << gmean);
          println("Max Time Across Procs:\t" << gmax << "\n");
        }
      }
      // end of timer code
#endif
#ifdef COUNTERS_ENABLED
      if (timerIdx <= numCounters) {
        if (levelIdx < THRESHOLD_LEVEL) {
          // computation only on proc 0
          if (MYTHREAD == 0) {
            long lmin =
              myCounts[timerIdx][levelIdx][myCounts[timerIdx][levelIdx].domain().min()];
            long lmax = lmin;
            long lsum = 0;
            int numReadingsPerProc = 0;
			    
            foreach (reading, myCounts[timerIdx][levelIdx].domain()) {
              long value = myCounts[timerIdx][levelIdx][reading];
              if (value < lmin) {
                lmin = value;
              }
              if (value > lmax) {
                lmax = value;
              }
              lsum += value;
              numReadingsPerProc++;
            }

            println("Num Readings On Proc 0:\t" << numReadingsPerProc);
            println("Min Count On Proc 0:\t" << lmin);
            long lmean = (lsum/numReadingsPerProc);
            println("Mean Count On Proc 0:\t" << lmean);
            println("Max Count On Proc 0:\t" << lmax << "\n");
          }
        }
        else {
          // computation on all procs
          long lmin =
            myCounts[timerIdx][levelIdx][myCounts[timerIdx][levelIdx].domain().min()];
          long lmax = lmin;
          long lsum = 0;
          int numReadingsPerProc = 0;
			
          foreach (reading, myCounts[timerIdx][levelIdx].domain()) {
            long value = myCounts[timerIdx][levelIdx][reading];
            if (value < lmin) {
              lmin = value;
            }
            if (value > lmax) {
              lmax = value;
            }
            lsum += value;
            numReadingsPerProc++;
          }
			
          long gmin = reduce::min(lmin);
          long gmax = reduce::max(lmax);
          long gsum = reduce::add(lsum);
		    
          if (MYTHREAD == 0) {
            println("Num Readings Per Proc:\t" << numReadingsPerProc);
            println("Min Count Across Procs:\t" << gmin);
            long gmean = gsum/(numReadingsPerProc*THREADS);
            println("Mean Count Across Proc:\t" << gmean);
            println("Max Count Across Procs:\t" << gmax << "\n");
          }
        }
      }
#endif
      // end of counter code
    }
  }
}
