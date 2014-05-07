#pragma once

#include "globals.h"

/**
 * This class is used by the Test class.
 */
class Random {
 public:
  //default seed
  double tran;   //First 9 digits of PI
  double seed;

  //Random Number Multiplier
  double amult; //=Math.pow(5.0,13);
  int KS;
  double R23, R46, T23, T46;

  //constants
  static const double d2m46;
  static const long i246m1;

  Random() : tran(314159265.0), seed(0), amult(1220703125.0), KS(0) {}

  Random(double sd) : tran(314159265.0), seed(sd), amult(1220703125.0), KS(0) {}

  /**
   * Random number generator with an external seed
   */
  double randlc(ndarray<double, 1> x, double a);

  inline double vranlc(int ny, int nz, double x, double a,
                       ndarray<Complex, 2> arraySlice) {
    long Lx = (long)x;
    long La = (long)a;

    for (int j=0; j<ny; j++) {
      for(int k=0; k<nz; k++){
        Lx = (Lx*La) & (i246m1);
        double re = (double)(d2m46*Lx);
        Lx = (Lx*La) & (i246m1);
        double im = (double)(d2m46*Lx);
        arraySlice[POINT(j, k)] = Complex(re, im);
      }
    }

    return (double) Lx;
  }

  double ipow46(double a, int exp1, int exp2);
};
