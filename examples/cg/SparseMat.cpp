#include "SparseMat.h"
#include "Util.h"
#ifdef TIMERS_ENABLED
# include "CGDriver.h"
#endif

// timer index for "myTimes" array, and the first two are for the "myCounts" array:
#define T_SPMV_LOCAL_COMP 1
#define T_SPMV_REDUCTION_COMP 2
#define T_SPMV_REDUCTION_COMM 3
#define T_SPMV_REDUCTION_BARRIER_POLLS 4
#define T_SPMV_DIAGONAL_SWAP_COMM 5
#define T_SPMV_DIAGONAL_SWAP_BARRIER_POLLS 6
#define T_SPMV_REDUCTION_UPDATE_FLAGS 7
#define T_SPMV_DIAGONAL_SWAP_UPDATE_FLAGS 8

int SparseMat::multiplyCallCount = 0;

SparseMat::SparseMat(LocalSparseMat &paramMySparseMat, int paramNumProcRows,
                     int paramNumProcCols, int paramN)
  : mySparseMat(paramMySparseMat), N(paramN), numProcRows(paramNumProcRows),
    numProcCols(paramNumProcCols) {

  // // tempSparseMatGrid is indexed by proc number, and is used to create sparseMatGrid
  // LocalSparseMat [1d] tempSparseMatGrid = new LocalSparseMat [0:(THREADS-1)];
  // tempSparseMatGrid.exchange(mySparseMat);

  // // now create sparseMatGrid, which is indexed by processor grid position
  // sparseMatGrid = new LocalSparseMat [[0,0]:[numProcRows-1, numProcCols-1]];
  // foreach (p, sparseMatGrid.domain()) {
  //     sparseMatGrid[p] = tempSparseMatGrid[p[1]*numProcCols + p[2]];
  // }

  // Retrieve values from Util class.
  int procRowPos = Util::procRowPos;
  int procColPos = Util::procColPos;
  rowStart = Util::rowStart;
  rowEnd = Util::rowEnd;
  log2numProcCols = Util::log2numProcCols;
  transposeProc = Util::transposeProc(MYTHREAD);

  // form the allResults array
#ifdef TEAMS
  myResults = ndarray<double, 1 UNSTRIDED>(RD(rowStart, rowEnd+1));
  allResults =
    ndarray<ndarray<double, 1, global GUNSTRIDED>, 1 UNSTRIDED>(RD(0, (int)THREADS));
  allResults.exchange(myResults);
  mtmp = ndarray<double, 1 UNSTRIDED>(RD(rowStart, rowEnd+1));
#else
  // for myResult's first index, 0 to log2numProcCols are actual sums,
  // while -log2numProcCols to -1 are data gathered from other procs
  ndarray<double, 2, global> myResults(RD(PT(-log2numProcCols, rowStart),
                                          PT(log2numProcCols+1, rowEnd+1)));
  allResults =
    ndarray<ndarray<double, 2, global>, 1 UNSTRIDED>(RD(0, (int)THREADS));
  allResults.exchange(myResults);

  // set up the reduce phase schedule for SpMV
  int divFactor = numProcCols;
  reduceExchangeProc = ndarray<int, 1 UNSTRIDED>(RD(0, log2numProcCols));
  for (int i = 0; i < log2numProcCols; i++) {
    int j = ((procColPos + divFactor/2) % divFactor) + procColPos / divFactor * divFactor;
    reduceExchangeProc[i] = Util::posToProcId(procRowPos, j);//procRowPos * numProcCols + j;
    divFactor /= 2;
  }
#endif

  // profiling
  multiplyCallCount = 0;

#ifdef TIMERS_ENABLED
  spmvCommTime = 0;

  // myTimer = new Timer();
  numTimers = 6;
  myTimes = ndarray<ndarray<double, 1 UNSTRIDED>, 1 UNSTRIDED>(RD(1, numTimers+1));

  // set up arrays for storing times
  myTimes[T_SPMV_LOCAL_COMP] =
    ndarray<double, 1 UNSTRIDED>(RD(1, 26 * CGDriver::niter + 1));
  myTimes[T_SPMV_REDUCTION_COMP] =
    ndarray<double, 1 UNSTRIDED>(RD(1, 26 * CGDriver::niter * log2numProcCols + 1));
#ifdef TEAMS
  myTimes[T_SPMV_REDUCTION_COMM] =
    ndarray<double, 1 UNSTRIDED>(RD(1, 26 * CGDriver::niter + 1));
#else
  myTimes[T_SPMV_REDUCTION_COMM] =
    ndarray<double, 1 UNSTRIDED>(RD(1, 26 * CGDriver::niter * log2numProcCols + 1));
#endif
  myTimes[T_SPMV_REDUCTION_BARRIER_POLLS] =
    ndarray<double, 1 UNSTRIDED>(RD(1, 26 * CGDriver::niter * log2numProcCols + 1));
  myTimes[T_SPMV_DIAGONAL_SWAP_COMM] =
    ndarray<double, 1 UNSTRIDED>(RD(1, 26 * CGDriver::niter + 1));
  myTimes[T_SPMV_DIAGONAL_SWAP_BARRIER_POLLS] =
    ndarray<double, 1 UNSTRIDED>(RD(1, 26 * CGDriver::niter + 1));
#endif

#ifdef COUNTERS_ENABLED
  myCounter = new PAPICounter(PAPI_MEASURE);
  numCounters = 2;
  myCounts =  ndarray<ndarray<long, 1 UNSTRIDED>, 1 UNSTRIDED>(RD(1, numTimers+1));

  // set up arrays for storing counts
  myCounts[T_SPMV_LOCAL_COMP] =
    ndarray<long, 1 UNSTRIDED>(RD(1, 26 * CGDriver::niter + 1));
  myCounts[T_SPMV_REDUCTION_COMP] =
    ndarray<long, 1 UNSTRIDED>(RD(1, 26 * CGDriver::niter * log2numProcCols + 1));
#endif

#ifdef TEAMS
  rowTeam = Util::rowTeam;
# ifdef CTEAMS
  columnTeam = Util::colTeam;
  rpivot = procRowPos;
  cpivot = procColPos;
  reduceCopy = (procColPos == procRowPos);
  reduceSource = MYTHREAD;
  if (numProcRows != numProcCols) {
    rpivot = procRowPos * 2;
    cpivot = procColPos / 2;
    reduceCopy = true;
    if ((procColPos % 2) != 0 && cpivot == procRowPos) {
      reduceSource = MYTHREAD - 1;
    } else if (cpivot != procRowPos)
      reduceCopy = false;
  }
  // setup team for synchronizing before reduce copies
  copyTeam = new team();
  copyTeam->split(reduceSource, reduceSource == MYTHREAD ? 0 : 1);//MYTHREAD);
  // copyTeam->initializeAll();
  copySync = copyTeam->mychild()->size() > 1;
# else
  transposeTeam = new team();
  transposeTeam->split(transposeProc < MYTHREAD ? transposeProc : MYTHREAD,
                       transposeProc < MYTHREAD ? 1 : 0);
  // transposeTeam->initializeAll();
# endif
#endif
}

void SparseMat::multiply(Vector &output, Vector &input) {
  ndarray<double, 1 UNSTRIDED> myOut = output.getMyArray();
  ndarray<double, 1 UNSTRIDED> myIn = input.getMyArray();

  // LocalSparseMat local mySparseMat = (LocalSparseMat local) mySparseMat;//sparseMatGrid[MYTHREAD/numProcCols, MYTHREAD%numProcCols];
  LocalSparseMat &mySparseMat = this->mySparseMat;
#ifdef VREDUCE
  ndarray<double, 1 UNSTRIDED> myResults0 = mtmp;
#elif defined(TEAMS)
  ndarray<double, 1 UNSTRIDED> myResults0 = myResults;
#else
  ndarray<double, 2 UNSTRIDED> myResults =
    (ndarray<double, 2 UNSTRIDED>) allResults[MYTHREAD];
  ndarray<double, 1 UNSTRIDED> myResults0 =
    (ndarray<double, 1 UNSTRIDED>) myResults.slice(1, 0);
#endif
  multiplyCallCount++;

  COUNTER_START(myCounter);
  TIMER_START(myTimer);

  // local SpMV
  mySparseMat.multiply(myResults0, myIn);

  TIMER_STOP_READ(myTimer, myTimes[T_SPMV_LOCAL_COMP][multiplyCallCount]);
  COUNTER_STOP_READ(myCounter, myCounts[T_SPMV_LOCAL_COMP][multiplyCallCount]);

#ifdef CTEAMS
  TIMER_START(myTimer);
  teamsplit(rowTeam) {
    myResults0 = (ndarray<double, 1 UNSTRIDED>) myResults;
    reduce::add(mtmp, myResults0, rpivot);
  }
  TIMER_STOP_READ(myTimer, myTimes[T_SPMV_REDUCTION_COMM][multiplyCallCount]);
  TIMER_START(myTimer);
  teamsplit(copyTeam) {
    if (copySync)
      barrier(); // ensure reductions above complete before copy
  }
  if (reduceCopy)
    myOut.copy(allResults[reduceSource]);
  teamsplit(columnTeam) {
    myOut.vbroadcast(cpivot);
  }
  TIMER_STOP_READ(myTimer, myTimes[T_SPMV_DIAGONAL_SWAP_COMM][multiplyCallCount]);
  TIMER_START(myTimer);
#elif defined(VREDUCE)
  TIMER_START(myTimer);
  teamsplit(rowTeam) {
    myResults0 = myResults;
    reduce::add(mtmp, myResults0);
  }
  barrier(); // ensure reductions above complete
  TIMER_STOP_READ(myTimer, myTimes[T_SPMV_REDUCTION_COMM][multiplyCallCount]);
  TIMER_START(myTimer);
  teamsplit(transposeTeam) {
    // actual diagonal swap
    if (team::current_team()->size() == 1) {
      myOut.copy(myResults0);
    } else {
# ifdef PUSH_DATA
      output.allArrays[transposeProc].copy(myResults0);
# else
      myOut.copy(allResults[transposeProc]);
# endif
    }
    barrier(); // wait for copy to be done
  }
  TIMER_STOP_READ(myTimer, myTimes[T_SPMV_DIAGONAL_SWAP_COMM][multiplyCallCount]);
  TIMER_START(myTimer);
#elif defined(TEAMS)
  TIMER_START(myTimer);
  teamsplit(rowTeam) {
    int start = rowStart;
    int end = rowEnd;
    for (int r = start; r <= end; r++) {
      myResults0[r] = reduce::add(myResults0[r]);
    }
  }
  TIMER_STOP_READ(myTimer, myTimes[T_SPMV_REDUCTION_COMM][multiplyCallCount]);
  TIMER_START(myTimer);
  teamsplit(transposeTeam) {
    // actual diagonal swap
    if (team::current_team()->size() == 1) {
      myOut.copy(myResults0);
    } else {
# ifdef PUSH_DATA
      output.allArrays[transposeProc].copy(myResults0);
# else
      myOut.copy(allResults[transposeProc]);
# endif
    }
    barrier(); // wait for copy to be done
  }
  TIMER_STOP_READ(myTimer, myTimes[T_SPMV_DIAGONAL_SWAP_COMM][multiplyCallCount]);
  TIMER_START(myTimer);
#else // teams
  // adding across processor column rows
  for (int i=0; i < log2numProcCols; i++) {
    TIMER_START(myTimer);

    // info sent to other procs is stored in index -(i+1) of their "allResults" array
    allResults[reduceExchangeProc[i]].slice(1, -(i+1)).copy(myResults.slice(1, i));

    TIMER_STOP_READ(myTimer, myTimes[T_SPMV_REDUCTION_COMM][(multiplyCallCount-1) * log2numProcCols + (i+1)]);

    TIMER_START(myTimer);
    barrier();
    TIMER_STOP_READ(myTimer, myTimes[T_SPMV_REDUCTION_BARRIER_POLLS][(multiplyCallCount-1) * log2numProcCols + (i+1)]);

    COUNTER_START(myCounter);
    TIMER_START(myTimer);

    ndarray<double, 1> myNewResults = myResults.slice(1, i+1);
    ndarray<double, 1> myOldResults = myResults.slice(1, i);
    ndarray<double, 1> myNewData = myResults.slice(1, -(i+1));
    FOREACH (p, myNewResults.domain()) {
      myNewResults[p] = myNewData[p] + myOldResults[p];
    };

    TIMER_STOP_READ(myTimer, myTimes[T_SPMV_REDUCTION_COMP][(multiplyCallCount-1) * log2numProcCols + (i+1)]);
    COUNTER_STOP_READ(myCounter, myCounts[T_SPMV_REDUCTION_COMP][(multiplyCallCount-1) * log2numProcCols + (i+1)]);
  }

  // diagonal swap - change from proc row partitioning to proc
  // col partitioning for "output" vector
# ifdef PUSH_DATA
  // begin diagonal swap puts
  TIMER_START(myTimer);

  // actual diagonal swap
  if (MYTHREAD == transposeProc) {
    myOut.copy(myResults.slice(1, log2numProcCols));
  }
  else {
    output.allArrays[transposeProc].copy(myResults.slice(1, log2numProcCols));
  }

  TIMER_STOP_READ(myTimer, myTimes[T_SPMV_DIAGONAL_SWAP_COMM][multiplyCallCount]);

  TIMER_START(myTimer);
  barrier();
  TIMER_STOP_READ(myTimer, myTimes[T_SPMV_DIAGONAL_SWAP_BARRIER_POLLS][multiplyCallCount]);
  // end diagonal swap puts
# else
  // begin diagonal swap gets
  TIMER_START(myTimer);
  barrier();
  TIMER_STOP_READ(myTimer, myTimes[T_SPMV_DIAGONAL_SWAP_BARRIER_POLLS][multiplyCallCount]);

  TIMER_START(myTimer);

  // actual diagonal swap
  if (MYTHREAD == transposeProc) {
    myOut.copy(myResults.slice(1, log2numProcCols));
  }
  else {
    myOut.copy(allResults[transposeProc].slice(1, log2numProcCols));
  }

  TIMER_STOP_READ(myTimer, myTimes[T_SPMV_DIAGONAL_SWAP_COMM][multiplyCallCount]);
  // end diagonal swap gets
# endif
#endif // teams
}

void SparseMat::resetProfile() {
  multiplyCallCount = 0;

#ifdef TIMERS_ENABLED
  FOREACH (component, myTimes.domain()) {
    FOREACH (timing, myTimes[component].domain()) {
      myTimes[component][timing] = 0;
    };
  };
#endif
#ifdef COUNTERS_ENABLED
  FOREACH (component, myCounts.domain()) {
    FOREACH (count, myCounts[component].domain()) {
      myCounts[component][count] = 0;
    };
  };
#endif
}

void SparseMat::printSummary() {
  if (MYTHREAD == 0) {
    println("\nSUMMARY- min, mean, max for each component");
    println("All times in seconds.\n");
  }

#if defined(TIMERS_ENABLED) || defined(COUNTERS_ENABLED)
  double totalTime = 0.0;
  double totalCommTime = 0.0;

  for (int timerIdx=1; timerIdx<=numTimers; timerIdx++) {
    if (MYTHREAD == 0) {
      switch (timerIdx) {
      case T_SPMV_LOCAL_COMP:
        println("SPMV LOCAL COMPUTATION: ");
        break;
      case T_SPMV_REDUCTION_COMP:
        println("SPMV REDUCTION COMPUTATION: ");
        break;
      case T_SPMV_REDUCTION_COMM:
        println("SPMV REDUCTION COMMUNICATION: ");
        break;
      case T_SPMV_REDUCTION_BARRIER_POLLS:
        println("SPMV REDUCTION BARRIERS: ");
        break;
      case T_SPMV_DIAGONAL_SWAP_COMM:
        println("SPMV DIAGONAL SWAP COMMUNICATION: ");
        break;
      case T_SPMV_DIAGONAL_SWAP_BARRIER_POLLS:
        println("SPMV DIAGONAL SWAP BARRIERS: ");
        break;
      }
    }

# ifdef TIMERS_ENABLED
    double totalComponentTime = 0.0;
    FOREACH (timing, myTimes[timerIdx].domain()) {
      totalComponentTime += myTimes[timerIdx][timing];
    };
    totalTime += totalComponentTime;
    if (timerIdx != T_SPMV_LOCAL_COMP)
      totalCommTime += totalComponentTime;

    double minTotalComponentTime = reduce::min(totalComponentTime);
    double sumTotalComponentTime = reduce::add(totalComponentTime);
    double maxTotalComponentTime = reduce::max(totalComponentTime);
	    
    if (MYTHREAD == 0) {
      println("Time: " << minTotalComponentTime << ", " << (sumTotalComponentTime/THREADS) << ", " << maxTotalComponentTime);
    }
# endif
# ifdef COUNTERS_ENABLED
    if (timerIdx <= numCounters) {
      long totalComponentCount = 0;
      FOREACH (count, myCounts[timerIdx].domain()) {
        totalComponentCount += myCounts[timerIdx][count];
      };
		
      long minTotalComponentCount = reduce::min(totalComponentCount);
      long sumTotalComponentCount = reduce::add(totalComponentCount);
      long maxTotalComponentCount = reduce::max(totalComponentCount);
		
      if (MYTHREAD == 0) {
        println("Count: " << minTotalComponentCount << ", " << (sumTotalComponentCount/THREADS) << ", " << maxTotalComponentCount);
      }
    }
# endif
  }
#endif // defined(TIMERS_ENABLED) || defined(COUNTERS_ENABLED)

#ifdef TIMERS_ENABLED
  double minTotalTime = reduce::min(totalTime);
  double sumTotalTime = reduce::add(totalTime);
  double maxTotalTime = reduce::max(totalTime);
  double minTotalCommTime = reduce::min(totalCommTime);
  double sumTotalCommTime = reduce::add(totalCommTime);
  double maxTotalCommTime = reduce::max(totalCommTime);
  spmvCommTime = totalCommTime;

  if (MYTHREAD == 0) {
    println("");
    println("SPMV TOTAL COMMUNICATION");
    println("Time: " << minTotalCommTime << ", " << (sumTotalCommTime/THREADS) << ", " << maxTotalCommTime);
    println("SPMV TOTAL");
    println("Time: " << minTotalTime << ", " << (sumTotalTime/THREADS) << ", " << maxTotalTime);
  }
#endif
}

void SparseMat::printProfile() {
#if defined(TIMERS_ENABLED) || defined(COUNTERS_ENABLED)
  for (int timerIdx=1; timerIdx<=numTimers; timerIdx++) {
    if (MYTHREAD == 0) {
      switch (timerIdx) {
      case T_SPMV_LOCAL_COMP:
        println("SPMV LOCAL COMPUTATION:\n");
        break;
      case T_SPMV_REDUCTION_COMP:
        println("SPMV REDUCTION COMPUTATION:\n");
        break;
      case T_SPMV_REDUCTION_COMM:
        println("SPMV REDUCTION COMMUNICATION:\n");
        break;
      case T_SPMV_REDUCTION_BARRIER_POLLS:
        println("SPMV REDUCTION BARRIERS:\n");
        break;
      case T_SPMV_DIAGONAL_SWAP_COMM:
        println("SPMV DIAGONAL SWAP COMMUNICATION:\n");
        break;
      case T_SPMV_DIAGONAL_SWAP_BARRIER_POLLS:
        println("SPMV DIAGONAL SWAP BARRIERS:\n");
        break;
      }
    }

# ifdef TIMERS_ENABLED
    // print timer info
    if (myTimes[timerIdx].domain().is_empty()) {
      if (MYTHREAD == 0) {
        println("Num Readings Per Proc:\t0\nMin Time Over Procs:\t0.0\nMean Time Over Procs:\t0.0\nMax Time Over Procs:\t0.0\n");
      }
    }
    else {
      double lminTime = myTimes[timerIdx][myTimes[timerIdx].domain().min()];
      double lmaxTime = lminTime;
      double lsumTime = 0;
      int numReadingsPerProc = 0;
		
      FOREACH (timing, myTimes[timerIdx].domain()) {
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
      double gmax = reduce::max(lmaxTime);
      double gsumTime = reduce::add(lsumTime);
		
      if (MYTHREAD == 0) {
        println("Num Readings Per Proc:\t" << numReadingsPerProc);
        println("Min Time Over Procs:\t" << gminTime);
        double gmeanTime = gsumTime/(numReadingsPerProc*THREADS);
        println("Mean Time Over Procs:\t" << gmeanTime);
        println("Max Time Over Procs:\t" << gmax << "\n");
      }
    }
# endif

    //print counter info
# ifdef COUNTERS_ENABLED
    if (timerIdx <= numCounters) {
      if (myCounts[timerIdx].domain().is_empty()) {
        if (MYTHREAD == 0) {
          println("Num Readings Per Proc:\t0");
          println("Min Count Over Procs:\t0.0");
          println("Mean Count Over Procs:\t0.0");
          println("Max Count Over Procs:\t0.0\n");
        }
      }
      else {
        long lminCount = myCounts[timerIdx][myCounts[timerIdx].domain().min()];
        long lmaxCount = lminCount;
        long lsumCount = 0;
        int numReadingsPerProc = 0;
		    
        FOREACH (count, myCounts[timerIdx].domain()) {
          long value = myCounts[timerIdx][count];
          if (value < lminCount) {
            lminCount = value;
          }
          if (value > lmaxCount) {
            lmaxCount = value;
          }
          lsumCount += value;
          numReadingsPerProc++;
        };
		    
        long gminCount = reduce::min(lminCount);
        long gmaxCount = reduce::max(lmaxCount);
        long gsumCount = reduce::add(lsumCount);
		    
        if (MYTHREAD == 0) {
          println("Num Readings Per Proc:\t" << numReadingsPerProc);
          println("Min Count Over Procs:\t" << gminCount);
          long gmeanCount = gsumCount/(numReadingsPerProc*THREADS);
          println("Mean Count Over Procs:\t" << gmeanCount);
          println("Max Count Over Procs:\t" << gmaxCount << "\n");
        }
      }
    }
# endif
  }
#endif // defined(TIMERS_ENABLED) || defined(COUNTERS_ENABLED)
}
