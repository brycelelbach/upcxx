#include <cassert>
#include <cmath>
#include "Vector.h"
#include "Util.h"

int Vector::N;                              // length of global vector
bool Vector::initialized = false;
int Vector::iStart;
int Vector::iEnd;
int Vector::numProcRows;
int Vector::numProcCols;

// Timer Vector::reduceTimer;

#ifdef TEAMS
Team Vector::rowTeam;
#endif

ndarray<int, 1> Vector::reduceExchangeProc;        // used to do communication in dot products
int Vector::log2numProcCols;

Vector::Vector() {
  assert(initialized); // ensure Vector class is initialized

  // form "localVectors" distributed array
  allArrays = ndarray<global_ndarray<double, 1>, 1>(RECTDOMAIN((0), (THREADS)));
  ndarray<double, 1> myArray(RECTDOMAIN((iStart), (iEnd+1)));
  allArrays.exchange(myArray);

#if !defined(TEAMS)
  // form the allResults array
  allResults = ndarray<global_ndarray<double, 1>, 1>(RECTDOMAIN((0), (THREADS)));
  // for myResult's first index, [0:log2numProcCols] are actual sums,
  // while [-log2numProcCols:-1] are data gathered from other procs
  ndarray<double, 1> myResults(RECTDOMAIN((-log2numProcCols), (log2numProcCols+1)));
  allResults.exchange(myResults);
#endif

  tmp = ndarray<double, 1>(RECTDOMAIN((iStart), (iEnd+1)));
}

void Vector::initialize(int paramN) {
  N = paramN;

  // Retrieve values from Util class.
  numProcRows = Util::numProcRows;
  numProcCols = Util::numProcCols;
  iStart = Util::colStart;
  iEnd = Util::colEnd;

  // reduceTimer = new Timer();

#ifdef TEAMS
  rowTeam = new Team();
  rowTeam.splitTeamAll(Util::procRowPos, Util::procColPos);
  rowTeam.initializeAll();
#else
  // Retrieve value from Util class.
  log2numProcCols = Util::log2numProcCols;

  // set up the reduce phase schedule (for dot products)
  int divFactor = numProcCols;
  reduceExchangeProc = ndarray<int, 1>(RECTDOMAIN((0), (log2numProcCols)));
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
  ndarray<double, 1> myArray = getMyArray();
  ndarray<double, 1> myAArray = a.getMyArray();
  double myResult = 0;

  foreach (p, myArray.domain()) {
    myResult += myArray[p] * myAArray[p];
  }

#ifdef TEAMS
  // reduceTimer.start();
  teamsplit(rowTeam) {
    myResult = REDUCE.add(myResult);
  }
  // reduceTimer.stop();
  return myResult;
#else // TEAMS
  // reduceTimer.start();
  ndarray<double, 1> myResults = (ndarray<double, 1>) allResults[MYTHREAD];
  myResults[0] = myResult;

  for (int i=0; i < log2numProcCols; i++) {
    // info sent to other procs is stored in index -(i+1) of their "allResults" array
    allResults[reduceExchangeProc[i]][-(i+1)] = myResults[i];
    barrier();
    myResults[i+1] = myResults[i] + myResults[-(i+1)];
  }

  // reduceTimer.stop();
  return myResults[log2numProcCols];
#endif // TEAMS
}

// compute the L2 Norm of vector
double Vector::L2Norm() {
  ndarray<double, 1> myArray = getMyArray();
  double myResult = 0;

  foreach (p, myArray.domain()) {
    myResult += myArray[p]*myArray[p];
  }

#ifdef TEAMS
  // reduceTimer.start();
  teamsplit(rowTeam) {
    myResult = REDUCE.add(myResult);
  }
  // reduceTimer.stop();
  return sqrt(myResult);
#else // TEAMS
  // reduceTimer.start();
  ndarray<double, 1> myResults = (ndarray<double, 1>) allResults[MYTHREAD];
  myResults[0] = myResult;

  for (int i=0; i < log2numProcCols; i++) {
    // info sent to other procs is stored in index -(i+1) of their "allResults" array
    allResults[reduceExchangeProc[i]][-(i+1)] = myResults[i];
    barrier();
    myResults[i+1] = myResults[i] + myResults[-(i+1)];
  }

  // reduceTimer.stop();
  return sqrt(myResults[log2numProcCols]);
#endif // TEAMS
}
