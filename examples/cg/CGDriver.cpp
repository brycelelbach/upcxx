#include <cmath>
#include "CGDriver.h"
#include "MatGen.h"
#include "Util.h"
#include "CG.h"

int CGDriver::niter;
#ifdef COUNTERS_ENABLED
PAPICounter CGDriver::myTotalCounter;
#endif

CGDriver::CGDriver(char paramClassType) {
  int niterLocal;

  // initializations of parameters based on class S
  // nonzer, shift, and rcond are all used to generate the sparse matrix
  na         = 1400;
  nonzer     = 7;
  shift      = 10;
  niterLocal = 15;
  zeta_verify_value = 8.5971775078648;
  switch (paramClassType) {
  case 's':
    na         = 1401;
    nonzer     = 7;
    shift      = 10;
    niterLocal = 15;
    zeta_verify_value = 8.57650477891241;
    break;
  case 'W':
    na         = 7000; 
    nonzer     = 8;
    shift      = 12;
    niterLocal = 15;
    zeta_verify_value = 10.362595087124;
    break;   
  case 'A':
    na         = 14000;
    nonzer     = 11;
    shift      = 20;
    niterLocal = 15;
    zeta_verify_value = 17.130235054029;
    break;
  case 'B':
    na         = 75000;
    nonzer     = 13;
    shift      = 60;
    niterLocal = 75;
    zeta_verify_value = 22.712745482631;
    break;
  case 'b':
    na         = 75000;
    nonzer     = 13;
    shift      = 60;
    niterLocal = 225;
    zeta_verify_value = 22.712745482631;
    break;
  case 'C':
    na         = 150000;
    nonzer     = 15;
    shift      = 110;
    niterLocal = 75;
    zeta_verify_value = 28.973605592845;
    break;
  case 'D':
    na         = 1500000;
    nonzer     = 21;
    shift      = 500;
    niterLocal = 100;
    zeta_verify_value = 52.5145321058;
    break;
  }

  niter = niterLocal; //broadcast niterLocal from 0;
  rcond = 0.1;
  epsilon = 1e-10;

  Util::initialize(na);
  A = MatGen::Initialize(na, nonzer, shift, rcond);
  Vector::initialize(A->N);
  x = new Vector();
  r = new Vector();

  // profiling
  // myTotalTimer = new Timer();
#ifdef COUNTERS_ENABLED
  myTotalCounter = new PAPICounter(PAPI_MEASURE);
#endif
}

void CGDriver::main(int argc, char **argv) {
  // read and check input
  char classArg;
  bool invalidInput = false;
  if (argc < 1) {
    invalidInput = true;
  }
  else {
    classArg = argv[0][0];
    if (classArg != 'A' && classArg != 'B' && classArg != 'C' && classArg != 'D' && classArg != 'S' && classArg != 'W' && classArg != 'b' && classArg != 's') {
      invalidInput = true;
    }
  }

  if (invalidInput) {
    if (MYTHREAD == 0) {
      println("Allowed problem classes are A, B, b, C, D, S, and W.");
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

  CGDriver Driver(classArg);
  CG cg;
  ndarray<double, 1> rNorm(RECTDOMAIN((1), (Driver.niter+1)));
  ndarray<double, 1> zeta(RECTDOMAIN((1), (Driver.niter+1)));

  if (MYTHREAD == 0) {
    println("UPC++ NAS CG Benchmark- Parallel\n");
    println("Problem class = " << classArg);
    println("Number of processors = " << THREADS);
    println("Matrix dimension = " << Driver.na);
    println("Number of iterations = " << Driver.niter);
    println("\nIter \t\t ||r|| \t\t zeta");
  }

  barrier();

  // initially make x a vector of all 1's
  Driver.x->oneOut();

  // do one untimed inverse power method iteration to warm up cache
  for (int iterNum = 1; iterNum <= 1; iterNum++) {
    Driver.z = cg.cg(Driver.A, Driver.x, Driver.r);              // perform CG to solve Az=x
    zeta[iterNum] = Driver.shift + 1/(Driver.x->dot(*Driver.z)); // solve for zeta
    Driver.A->multiply(*Driver.r, *Driver.z);                    // r = A * z
    rNorm[iterNum] = ((*Driver.x) -= (*Driver.r)).L2Norm();      // rNorm = || x - r ||
    (*Driver.z) /= Driver.z->L2Norm();                           // normalize z's length to 1
    Driver.x->copy(*Driver.z);                                   // copy z into x
  }

  // make x a vector of all 1's again
  Driver.x->oneOut();

  // reset SpMV counters and timers for actual run
#ifdef TIMERS_ENABLED
  Driver.myTotalTimer.reset();
#endif
  Driver.A->resetProfile();
#ifdef COUNTERS_ENABLED
  Driver.myTotalCounter.clear();
#endif

  // now we can start profiling
  COUNTER_START(Driver.myTotalCounter);
  barrier();
  TIMER_START(Driver.myTotalTimer);

  // inverse power method iterations
  for (int iterNum = 1; iterNum <= Driver.niter; iterNum++) {
    Driver.z = cg.cg(Driver.A, Driver.x, Driver.r);              // perform CG to solve Az=x
    zeta[iterNum] = Driver.shift + 1/(Driver.x->dot(*Driver.z)); // solve for zeta
    Driver.A->multiply(*Driver.r, *Driver.z);                    // r = A * z
    rNorm[iterNum] = ((*Driver.x) -= (*Driver.r)).L2Norm();      // rNorm = || x - r ||
    (*Driver.z) /= Driver.z->L2Norm();                           // normalize z's length to 1
    Driver.x->copy(*Driver.z);                                   // copy z into x
  }

  TIMER_STOP(Driver.myTotalTimer);
  COUNTER_STOP(Driver.myTotalCounter);

  // end of profiling

  // print values of rNorm and zeta at each outer iteration
  if (MYTHREAD == 0) {
    for (int i=1; i <= Driver.niter; i++) {
      println(i << "\t" << rNorm[i] << "\t" << zeta[i]);
    }
    println("");
  }

  // verify final zeta value
  if (MYTHREAD == 0) {
    double final_zeta = zeta[Driver.niter];
    double error = abs(final_zeta - Driver.zeta_verify_value);
    if (error <= Driver.epsilon) {
      println("VERIFICATION SUCCESSFUL");
      println("Zeta = " << final_zeta);
    }
    else {
      println("VERIFICATION FAILED");
      println("Zeta = " << final_zeta);
      println("Correct zeta = " << Driver.zeta_verify_value);
    }
    println("Error = " << error);
  }

  // gather and print profiling information
  Driver.A->printSummary();

#ifdef TIMERS_ENABLED
  double myTotalTime = Driver.myTotalTimer.secs();
  double maxTotalTime = Reduce::max(myTotalTime);

  double redTime = Vector::reduceTimer.secs();
  double minRedTime = Reduce::min(redTime);
  double meanRedTime = Reduce::add(redTime) / THREADS;
  double maxRedTime = Reduce::max(redTime);
  double commTime = redTime + Driver.A->spmvCommTime;
  double minCommTime = Reduce::min(commTime);
  double meanCommTime = Reduce::add(commTime) / THREADS;
  double maxCommTime = Reduce::max(commTime);
#endif
#ifdef COUNTERS_ENABLED
  long myTotalCount = Driver.myTotalCounter.getCounterValue();
  long allTotalCount = Reduce::add(myTotalCount);
#endif
	
  if (MYTHREAD == 0) {
#ifdef TIMERS_ENABLED
    println("\nTotal:");
    println("Max time over all processors:\t\t " << maxTotalTime);
#endif
#ifdef COUNTERS_ENABLED
    println("Total count over all processors:\t " << allTotalCount);
#endif
#ifdef TIMERS_ENABLED
    println("");
    println("Vector reduction time" << ": " <<
            minRedTime << ", " << meanRedTime <<
            ", " << maxRedTime);
    println("");
    println("Total communication time" << ": " <<
            minCommTime << ", " << meanCommTime <<
            ", " << maxCommTime);
    println("");
#endif
  }

  Driver.A->printProfile();
}

int main(int argc, char **argv) {
  init(&argc, &argv);
#ifdef TEAMS
  Team::initialize();
#endif
  CGDriver::main(argc-1, argv+1);
  finalize();
  return 0;
}
