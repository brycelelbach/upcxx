#pragma once

#include "globals.h"
#include "LocalSparseMat.h"
#include "Vector.h"
#ifdef TIMERS_ENABLED
# include <timer.h>
#endif

/* This class represents a distributed sparse square matrix (in CSR format).  Each
   proc should have an equal (or off by one) number of rows of the sparse matrix.
   Each local sparse matrix is represented as a LocalSparseMat object. */

class SparseMat {
 public:
  // sparseMatGrid is a distributed array of LocalSparseMat objects.
  // It is 2D-blocked, indexed first by proc row and then by proc col
  // LocalSparseMat [2d] sparseMatGrid;
  LocalSparseMat &mySparseMat;             // this thread's share of the sparse matrix
  int N;                                   // dimension of the square matrix
  int numProcRows, numProcCols;            // # of processor rows and columns (multiplied = # procs)
  int rowStart, rowEnd;                    // start and end row for this processor
#ifdef TEAMS
  ndarray<global_ndarray<double, 1 UNSTRIDED>, 1 UNSTRIDED> allResults;      // used for storing SpMV results
  ndarray<double, 1 UNSTRIDED> myResults;
  ndarray<double, 1 UNSTRIDED> mtmp;
#else
  ndarray<global_ndarray<double, 2>, 1 UNSTRIDED> allResults;      // used for storing SpMV results
 private:
  ndarray<int, 1 UNSTRIDED> reduceExchangeProc;            // used to do communication in dot products
#endif
  int transposeProc;                      // used to do communication in dot products
  int log2numProcCols;
  static int multiplyCallCount;           // used for profiling

#ifdef TEAMS
  team *rowTeam;
# ifdef CTEAMS
  team *columnTeam, *copyTeam;
  bool reduceCopy, copySync;
  int reduceSource;
  int rpivot, cpivot;
# else
  team *transposeTeam;
# endif
#endif

 public:
#ifdef TIMERS_ENABLED
  /* profiling */
  int numTimers, numCounters;
  timer myTimer;
  // "myTimes" is indexed by (CG Component #) and then timing number
  ndarray<ndarray<double, 1 UNSTRIDED>, 1 UNSTRIDED> myTimes;
  // total SPMV communication time
  double spmvCommTime;
#endif

#ifdef COUNTERS_ENABLED
  PAPICounter myCounter;
  // "myCounts" is indexed by (CG Component #) and then counting number
  ndarray<ndarray<long, 1 UNSTRIDED>, 1 UNSTRIDED> myCounts;
#endif

  // This constructor makes the array of local sparse matrices "localSparseMats" from the
  // global arrays for matrix A.
  // the "l" at the start of each parameter name means "local"
  static SparseMat *makeSparseMat(ndarray<double, 1 UNSTRIDED> la,
                                  ndarray<int, 1 UNSTRIDED> lcidx,
                                  ndarray<int, 1 UNSTRIDED> lrow,
                                  int paramNumProcRows,
                                  int paramNumProcCols, int paramN) {
    LocalSparseMat *lsmat = new LocalSparseMat(la, lcidx, lrow, paramN,
                                               paramNumProcRows, paramNumProcCols);
    return new SparseMat(*lsmat, paramNumProcRows, paramNumProcCols, paramN);
  }

  SparseMat(LocalSparseMat &paramMySparseMat, int paramNumProcRows,
            int paramNumProcCols, int paramN);

  // This method does a sparse matrix-vector multiply, with each proc's portion of
  // the "output" Vector being computed.  This is done by copying the full "input"
  // vector onto each processor and then multiplying by the respective processor's
  // LocalSparseMat object to produce it's portion of the "output" vector.
  void multiply(Vector &output, Vector &input);

  void resetProfile();

  void printSummary();

  void printProfile();
};
