#pragma once

#include "globals.h"
#ifdef TIMERS_ENABLED
# include "Timer.h"
#endif

/*  This class represents a distributed vector.  The vector is
    processor column-partitioned across all the processors. */

class Vector {
 public:
  ndarray<global_ndarray<double, 1>, 1> allArrays;   // distributedly stores actual vector data (in global coordinates)

#ifdef TIMERS_ENABLED
  static Timer reduceTimer;
#endif

 private:
  static int N;                              // length of global vector
  static bool initialized;
  static int iStart;
  static int iEnd;
  static int numProcRows;
  static int numProcCols;

  ndarray<double, 1> tmp;

#ifdef TEAMS
  static Team *rowTeam;
#else
  static ndarray<int, 1> reduceExchangeProc;        // used to do communication in dot products
  static int log2numProcCols;

 public:
  ndarray<global_ndarray<double, 1>, 1> allResults;  // used for storing dot product and L2-Norm results
#endif
 public:

  // constructor
  Vector();

  // Initialize the Vector class. The given parameter is constant over all instances.
  static void initialize(int paramN);

  // return current "vector" field
  inline ndarray<double, 1> getMyArray() const {
    return (ndarray<double, 1>) allArrays[MYTHREAD];
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
    ndarray<double, 1> myArray = getMyArray();
    ndarray<double, 1> myAArray = a.getMyArray();

    foreach (p, myArray.domain()) {
      myArray[p] -= myAArray[p];
    }
    return *this;
  }

  // operator overload, scalar vector divide
  Vector &operator/=(double x) {
    ndarray<double, 1> myArray = getMyArray();

    foreach (q, myArray.domain()) {
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
    ndarray<double, 1> myXArray = x.getMyArray();
    ndarray<double, 1> myYArray = y.getMyArray();
    ndarray<double, 1> myDestArray = getMyArray();

    foreach (p, myDestArray.domain()) {
      myDestArray[p] = alpha * myXArray[p] + myYArray[p];
    }
  }
};