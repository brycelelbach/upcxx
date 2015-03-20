#include <cassert>
#include <cmath>
#include "Vector.h"
#include "Util.h"

#ifdef TIMERS_ENABLED
timer Vector::reduceTimer;
#endif

int Vector::N;                              // length of global vector
bool Vector::initialized = false;
int Vector::iStart;
int Vector::iEnd;
int Vector::numProcRows;
int Vector::numProcCols;

#ifdef VTEAMS
team *Vector::rowTeam;
#else
ndarray<int, 1 UNSTRIDED> Vector::reduceExchangeProc;        // used to do communication in dot products
int Vector::log2numProcCols;
#endif

Vector::Vector() {
  assert(initialized); // ensure Vector class is initialized

  // form "localVectors" distributed array
  allArrays = ndarray<ndarray<double, 1, global GUNSTRIDED>, 1 UNSTRIDED>(RD(0, (int)THREADS));
  ndarray<double, 1 UNSTRIDED> myArray(RD(iStart, iEnd+1));
  allArrays.exchange(myArray);

#if !defined(VTEAMS)
  // form the allResults array
  allResults = ndarray<ndarray<double, 1, global GUNSTRIDED>, 1 UNSTRIDED>(RD(0, (int)THREADS));
  // for myResult's first index, [0:log2numProcCols] are actual sums,
  // while [-log2numProcCols:-1] are data gathered from other procs
  ndarray<double, 1 UNSTRIDED> myResults(RD(-log2numProcCols, log2numProcCols+1));
  allResults.exchange(myResults);
#endif

#ifdef FORCE_VREDUCE
  src = ndarray<double, 1 UNSTRIDED>(RD(0, 1));
  dst = ndarray<double, 1 UNSTRIDED>(RD(0, 1));
#endif
}

void Vector::initialize(int paramN) {
  N = paramN;

  // reduceTimer = new Timer();

  // Retrieve values from Util class.
  numProcRows = Util::numProcRows;
  numProcCols = Util::numProcCols;
  iStart = Util::colStart;
  iEnd = Util::colEnd;
#ifdef VTEAMS
  rowTeam = Util::rowTeam;
#else
  log2numProcCols = Util::log2numProcCols;

  // set up the reduce phase schedule (for dot products)
  int divFactor = numProcCols;
  reduceExchangeProc = ndarray<int, 1 UNSTRIDED>(RD(0, log2numProcCols));
  for (int i = 0; i < log2numProcCols; i++) {
    int procColPos = MYTHREAD % numProcCols;
    int procRowPos = MYTHREAD / numProcCols;
    int j = ((procColPos + divFactor/2) % divFactor) + procColPos / divFactor * divFactor;
    reduceExchangeProc[i] = Util::posToProcId(procRowPos, j);//procRowPos * numProcCols + j;
    divFactor /= 2;
  }
#endif

  initialized = true;
}

double Vector::dot(const Vector &a) {
  ndarray<double, 1 UNSTRIDED> myArray = getMyArray();
  ndarray<double, 1 UNSTRIDED> myAArray = a.getMyArray();
  double myResult = 0;

  FOREACH (p, myArray.domain()) {
    myResult += myArray[p] * myAArray[p];
  }

#ifdef VTEAMS
  TIMER_START(reduceTimer);
  // a bug? in clang causes failure without the following line, likely
  // due to improper inlining of the reduction
  double myResult0 = myResult;
# ifdef FORCE_VREDUCE
  src[0] = myResult;
# endif
  upcxx_teamsplit(rowTeam) {
    barrier(); // added
# if !defined(FORCE_VREDUCE)
    myResult = reduce::add(myResult0); // myResult0 just to be safe
# else
    reduce::add(src, dst);
    myResult = dst[0];
# endif
  }
  TIMER_STOP(reduceTimer);
  return myResult;
#else // VTEAMS
  TIMER_START(reduceTimer);
  ndarray<double, 1 UNSTRIDED> myResults =
    (ndarray<double, 1 UNSTRIDED>) allResults[(int) MYTHREAD];
  myResults[0] = myResult;

  for (int i=0; i < log2numProcCols; i++) {
    // info sent to other procs is stored in index -(i+1) of their "allResults" array
    allResults[reduceExchangeProc[i]][-(i+1)] = myResults[i];
    barrier();
    myResults[i+1] = myResults[i] + myResults[-(i+1)];
  }

  TIMER_STOP(reduceTimer);
  return myResults[log2numProcCols];
#endif // VTEAMS
}

// compute the L2 Norm of vector
double Vector::L2Norm() {
  ndarray<double, 1 UNSTRIDED> myArray = getMyArray();
  double myResult = 0;

  FOREACH (p, myArray.domain()) {
    myResult += myArray[p]*myArray[p];
  }

#ifdef VTEAMS
  TIMER_START(reduceTimer);
  // a bug? in clang causes failure without the following line, likely
  // due to improper inlining of the reduction
  double myResult0 = myResult;
# ifdef FORCE_VREDUCE
  src[0] = myResult;
# endif
  upcxx_teamsplit(rowTeam) {
    barrier(); // added
# if !defined(FORCE_VREDUCE)
    myResult = reduce::add(myResult0); // myResult0 just to be safe
# else
    reduce::add(src, dst);
    myResult = dst[0];
# endif
  }
  TIMER_STOP(reduceTimer);
  return sqrt(myResult);
#else // VTEAMS
  TIMER_START(reduceTimer);
  ndarray<double, 1 UNSTRIDED> myResults =
    (ndarray<double, 1 UNSTRIDED>) allResults[(int) MYTHREAD];
  myResults[0] = myResult;

  for (int i=0; i < log2numProcCols; i++) {
    // info sent to other procs is stored in index -(i+1) of their "allResults" array
    allResults[reduceExchangeProc[i]][-(i+1)] = myResults[i];
    barrier();
    myResults[i+1] = myResults[i] + myResults[-(i+1)];
  }

  TIMER_STOP(reduceTimer);
  return sqrt(myResults[log2numProcCols]);
#endif // VTEAMS
}
