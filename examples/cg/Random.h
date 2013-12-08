#pragma once

/* #if USE_UPCXX */
/* # include <portable_inttypes.h> */
/* #elif __cplusplus >= 201103L */
/* # include <cstdint> */
/* #else */
/* typedef long long int64_t; */
/* #endif */

class Random {
  public:
    double seed;
    //default seed
    double tran;   //First 9 digits of PI
    //Random Number Multiplier
    double amult; //=Math.pow(5.0,13);
    //constants
    /* static double d2m46; */
    /* static int64_t i246m1; */
    
  Random() : tran(314159265.0), amult(1220703125.0) {}
  Random(double sd) : seed(sd), tran(314159265.0), amult(1220703125.0) {}

  //Random number generator with an external seed
  double randlc(double x, double a);

  //Random number generator with an internal seed
  double randlc(double a);

  // double vranlc(double n, double x, double a, double y[],int offset){ 
  //   int64_t Lx = (int64_t)x;
  //   int64_t La = (int64_t)a;

  //   for(int i=0;i<n;i++){
  //     Lx   = (Lx*La) & (i246m1);
  //     y[offset+i] = (double)(d2m46* Lx);
  //   }
  //   return (double) Lx;
  // }

  double ipow46(double a, int exponent);

  double power(double a, int n);
};
