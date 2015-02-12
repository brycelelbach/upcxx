#include <cmath>
#include "MGDriver.h"
#include "MG.h"
#include "Test.h"

double S0, S1, S2;

int MGDriver::startLevel;
int MGDriver::numIterations;

MGDriver::MGDriver(char paramClassType) : epsilon(1e-8) {
  int startLevelLocal, numIterationsLocal;

  // initializations of parameters based on class S
  startLevelLocal = 5;
  numIterationsLocal = 4;
  verifyValue = 5.30770700573e-5;
  switch (paramClassType) {
  case 'W':
    startLevelLocal = 6;
    numIterationsLocal = 40;
    verifyValue = 2.50391406439e-18;
    break;
  case 'A':
    startLevelLocal = 8;
    numIterationsLocal = 4;
    verifyValue =  2.433365309e-6;
    break;
  case 'B':
    startLevelLocal = 8;
    numIterationsLocal = 20;
    verifyValue = 1.80056440132e-6;
    break;
  case 'C':
    startLevelLocal = 9;
    numIterationsLocal = 20;
    verifyValue = 5.70674826298e-7;
    break;
  case 'D':
    startLevelLocal = 10;
    numIterationsLocal = 50;
    verifyValue = 1.58327506043e-10;
    break;
  }

  startLevel = startLevelLocal;//broadcast startLevelLocal from 0;
  numIterations = numIterationsLocal;//broadcast numIterationsLocal from 0;
  classType = paramClassType;//broadcast paramClassType from 0;

  if (classType == 'A' || classType == 'S' || classType == 'W') {
    S0 = (-.375);
    S1 = .03125;
    S2 = (-.015625);
  } else {
    S0 = (-3.0/17.0);
    S1 = (1.0/33.0);
    S2 = (-1.0/61.0);
  }

  Grid::init();
  RectDomain<1> level_rd(PT(1), PT(startLevel+1));
  rhsGrid = new Grid(startLevel, false, false);
  residualGrids = ndarray<Grid *, 1 UNSTRIDED>(level_rd);
  correctionGrids = ndarray<Grid *, 1 UNSTRIDED>(level_rd);
	
  foreach (level, level_rd) {
    if (level[1] != (THRESHOLD_LEVEL-1)) {
      residualGrids[level] = new Grid(level[1], true, false);
      correctionGrids[level] = new Grid(level[1], true, false);
    }
    else {
      residualGrids[level] = new Grid(level[1], true, true);
      correctionGrids[level] = new Grid(level[1], true, true);
    }
  };

  // populate rhsGrid with initial values
  Test::populateRHSGrid(*rhsGrid, startLevel);
	
  // profiling
  // myTotalTimer = new Timer();
#ifdef COUNTERS_ENABLED
  myTotalCounter = new PAPICounter(PAPI_MEASURE);
#endif
}

void MGDriver::main(int argc, char **argv) {
  // validate input
  char classArg;
  bool invalidInput = false;
  if (argc < 1) {
    invalidInput = true;
  }
  else {
    classArg = argv[0][0];//Character.toUpperCase(argv[0].charAt(0));
    if (classArg != 'A' && classArg != 'B' && classArg != 'C' &&
        classArg != 'D' && classArg != 'S' && classArg != 'W') {
      invalidInput = true;
    }
  }

  if (invalidInput) {
    if (MYTHREAD == 0) {
      println("Allowed problem classes are A, B, C, D, S, and W.");
    }
    exit(1);
  }

  // check that there are a power of two number of processors
  bool powerOfTwoProcs = true;
  int tempNumOfProcs = THREADS;
  while (tempNumOfProcs > 1) {
    if (tempNumOfProcs % 2 == 1) powerOfTwoProcs = false;
    tempNumOfProcs /= 2;
  }

  if (!powerOfTwoProcs) {
    if (MYTHREAD == 0) {
      println("The number of processors must be a power of two.");
    }
    exit(1);
  }

  MGDriver Driver = MGDriver(classArg);
  MG myMG = MG();

  // one untimed iteration for warming up the cache
  myMG.evaluateResidual(*Driver.rhsGrid,
                        *(Driver.correctionGrids[Driver.startLevel]),
                        *(Driver.residualGrids[Driver.startLevel]),
                        Driver.startLevel, 1, 1);
  myMG.updateBorder(*(Driver.residualGrids[Driver.startLevel]),
                    Driver.startLevel, 1, 1);
  double L2ResInitial =
    myMG.getL2Norm(*(Driver.residualGrids[Driver.startLevel]), 1);
  myMG.vCycle(Driver.residualGrids, Driver.correctionGrids, *Driver.rhsGrid, 1);
  myMG.evaluateResidual(*Driver.rhsGrid,
                        *(Driver.correctionGrids[Driver.startLevel]),
                        *(Driver.residualGrids[Driver.startLevel]),
                        Driver.startLevel, 1, 1);
  myMG.updateBorder(*(Driver.residualGrids[Driver.startLevel]),
                    Driver.startLevel, 1, 2);

  // reset initial values
  myMG = MG();
  Driver.correctionGrids[Driver.startLevel]->zeroOut();
  Driver.rhsGrid->zeroOut();
  Test::populateRHSGrid(*Driver.rhsGrid, Driver.startLevel);
  myMG.resetProfile();

  // start timed iterations
  COUNTER_START(Driver.myTotalCounter);
  barrier();
  TIMER_START(Driver.myTotalTimer);

  myMG.evaluateResidual(*Driver.rhsGrid,
                        *(Driver.correctionGrids[Driver.startLevel]),
                        *(Driver.residualGrids[Driver.startLevel]),
                        Driver.startLevel, 0, 2);
  myMG.updateBorder(*(Driver.residualGrids[Driver.startLevel]),
                    Driver.startLevel, 0, 3);
  L2ResInitial = myMG.getL2Norm(*(Driver.residualGrids[Driver.startLevel]), 1);

  for (int iter=1; iter <= Driver.numIterations; iter++) {
    myMG.vCycle(Driver.residualGrids, Driver.correctionGrids,
                *Driver.rhsGrid, iter);
    myMG.evaluateResidual(*Driver.rhsGrid,
                          *(Driver.correctionGrids[Driver.startLevel]),
                          *(Driver.residualGrids[Driver.startLevel]),
                          Driver.startLevel, iter, 2);
    myMG.updateBorder(*(Driver.residualGrids[Driver.startLevel]),
                      Driver.startLevel, iter, 1);
  }
  double L2ResFinal =
    myMG.getL2Norm(*(Driver.residualGrids[Driver.startLevel]), 2);

  TIMER_STOP(Driver.myTotalTimer);
  COUNTER_STOP(Driver.myTotalCounter);
  // end timed iterations

  if (MYTHREAD == 0) {
    println("Titanium NAS MG Benchmark- Parallel\n");
    println("Problem class = " << Driver.classType);
    println("Number of processors = " << THREADS);
    println("Start level (log base 2 of the cubic grid side length) = " <<
            Driver.startLevel);
    println("Threshold level (below this level, all the work is done " <<
            "on processor 0) = " << THRESHOLD_LEVEL);
    println("Number of blocks in each grid dimension [x,y,z] = " <<
            Grid::numBlocksInGridSide);
    println("Number of iterations = " << Driver.numIterations);

    // verify final L2 residual value
    double error = abs(L2ResFinal - Driver.verifyValue);
    if (error <= Driver.epsilon) {
      println("\nVERIFICATION SUCCESSFUL");
      println("Initial L2 Residual = " << L2ResInitial);
      println("L2 Residual after iteration " << Driver.numIterations <<
              " = " << L2ResFinal);
    }
    else {
      println("\nVERIFICATION FAILED");
      println("Initial L2 Residual = " << L2ResInitial);
      println("L2 Residual after iteration " << Driver.numIterations <<
              " = " << L2ResFinal);
      println("Correct final L2 Residual = " << Driver.verifyValue);
    }
    println("Error in final value = " << error);
  }

  // profiling
  myMG.printSummary();
#ifdef TIMERS_ENABLED
  double myTotalTime = Driver.myTotalTimer.secs();
  double maxTotalTime = reduce::max(myTotalTime);
#endif
#ifdef COUNTERS_ENABLED
  long myTotalCount = Driver.myTotalCounter.getCounterValue();
  long allTotalCount = reduce::add(myTotalCount);
#endif
  if (MYTHREAD == 0) {
    println("\nTOTAL:");
#ifdef TIMERS_ENABLED
    println("Max Time Across Procs:\t\t" << maxTotalTime);
#endif
#ifdef COUNTERS_ENABLED
    println("Total Count Across Procs:\t" << allTotalCount);
#endif
  }
  myMG.printProfile();
}

int main(int argc, char **argv) {
  init(&argc, &argv);
  MGDriver::main(argc-1, argv+1);
  finalize();
  return 0;
}
