#pragma once

#include "globals.h"
#include "SparseMat.h"
#include "Vector.h"
#ifdef TIMERS_ENABLED
# include <timer.h>
#endif

class CGDriver {
 private:
  SparseMat *A;
  Vector *x, *r, *z;
  int na;       // the dimension of square matrix A
  int nonzer;          // used in generating matrix A
  double shift, rcond; // used in generating matrix A
  double zeta_verify_value, epsilon;  // used to verify correctness

 public:
  static int niter;    // the number of outer iterations performed

  // profiling information
#ifdef TIMERS_ENABLED
  timer myTotalTimer;
#endif
#ifdef COUNTERS_ENABLED
  static PAPICounter myTotalCounter;
#endif

  // constructor
  CGDriver(char paramClassType);

  static void main(int argc, char **argv);
};
