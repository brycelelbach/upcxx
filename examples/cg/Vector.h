#pragma once

#include "globals.h"

/*  This class represents a distributed vector.  The vector is
    processor column-partitioned across all the processors. */

class Vector {
 public:
  ndarray<ndarray<double, 1, global GUNSTRIDED>, 1 UNSTRIDED> allArrays;   // distributedly stores actual vector data (in global coordinates)

#ifdef TIMERS_ENABLED
  static timer reduceTimer;
#endif

 private:
  static int N;                              // length of global vector
  static bool initialized;
  static int iStart;
  static int iEnd;
  static int numProcRows;
  static int numProcCols;

#ifdef FORCE_VREDUCE
  ndarray<double, 1> src, dst;
#endif

#ifdef TEAMS
  static team *rowTeam;
#else
  static ndarray<int, 1 UNSTRIDED> reduceExchangeProc;        // used to do communication in dot products
  static int log2numProcCols;

 public:
  ndarray<ndarray<double, 1, global GUNSTRIDED>, 1 UNSTRIDED> allResults;  // used for storing dot product and L2-Norm results
#endif
 public:

  // constructor
  Vector();

  // Initialize the Vector class. The given parameter is constant over all instances.
  static void initialize(int paramN);

  // return current "vector" field
  inline ndarray<double, 1 UNSTRIDED> getMyArray() const {
    return (ndarray<double, 1 UNSTRIDED>) allArrays[MYTHREAD];
  }

  // deep-copy the elements of "a"'s vector field to "this"'s vector field
  void copy(const Vector &a) {
    getMyArray().copy(a.getMyArray());
  }
 
  // put 0's in every entry of "this"'s vector field
  void zeroOut() {
    getMyArray().set(0.0);
  }

  // put 1's in every entry of "this"'s vector field
  void oneOut() {
    getMyArray().set(1.0);
  }

  // operator overload, vector-vector subtraction
  Vector &operator-=(const Vector &a) {
    ndarray<double, 1 UNSTRIDED> myArray = getMyArray();
    ndarray<double, 1 UNSTRIDED> myAArray = a.getMyArray();

    FOREACH (p, myArray.domain()) {
      myArray[p] -= myAArray[p];
    }
    return *this;
  }

  // operator overload, scalar vector divide
  Vector &operator/=(double x) {
    ndarray<double, 1 UNSTRIDED> myArray = getMyArray();

    FOREACH (q, myArray.domain()) {
      myArray[q] /= x;
    }
    return *this;
  }

  // dot product of two vectors
  double dot(const Vector &a);

  // compute the L2 Norm of vector
  double L2Norm();
    
  // "this" vector = alpha * x + y
  void axpy(double alpha, const Vector &x, const Vector &y) {
    ndarray<double, 1 UNSTRIDED> myXArray = x.getMyArray();
    ndarray<double, 1 UNSTRIDED> myYArray = y.getMyArray();
    ndarray<double, 1 UNSTRIDED> myDestArray = getMyArray();

    FOREACH (p, myDestArray.domain()) {
      myDestArray[p] = alpha * myXArray[p] + myYArray[p];
    }
  }
};
