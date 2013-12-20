#include <cmath>
#include "Random.h"

const double Random::d2m46 = pow(0.5, 46);
const long Random::i246m1 = (long) pow(2, 46) - 1;

double randlc(ndarray<double, 1> x, double a) {
  double r23,r46,t23,t46,t1,t2,t3,t4,a1,a2,x1,x2,z;
  r23 = pow(0.5,23); 
  r46 = pow(r23, 2); 
  t23 = pow(2.0,23);
  t46 = pow(t23, 2);
  //---------------------------------------------------------------------
  //   Break A into two parts such that A = 2^23 * A1 + A2.
  //---------------------------------------------------------------------
  t1 = r23 * a;
  a1 = (int) t1;
  a2 = a - t23 * a1;
  //---------------------------------------------------------------------
  //   Break X into two parts such that X = 2^23 * X1 + X2, compute
  //   Z = A1 * X2 + A2 * X1  (mod 2^23), and then
  //   X = 2^23 * Z + A2 * X2  (mod 2^46).
  //---------------------------------------------------------------------
  t1 = r23 * x[0];
  x1 = (int) t1;
  x2 = x[0] - t23 * x1;
  t1 = a1 * x2 + a2 * x1;
  t2 = (int) (r23 * t1);
  z = t1 - t23 * t2;
  t3 = t23 * z + a2 * x2;
  t4 = (int) (r46 * t3);
  x[0] = t3 - t46 * t4;
  return (r46 * x[0]);
}

double ipow46(double a, int exp1, int exp2) {
  int n2;
  double dummy;
  ndarray<double, 1> temp(RECTDOMAIN((0), (1)));

  //---------------------------------------------------------------------
  // Use
  //   a^n = a^(n/2)*a^(n/2) if n even else
  //   a^n = a*a^(n-1)       if n odd
  //---------------------------------------------------------------------
  if (exp1 == 0 || exp2 == 0) return 1;
  double q = a;
  double r = 1;
  int n = exp1;
  bool two_pow = true;

  while (two_pow) {
    n2 = n/2;
    if (2*n2 == n) {
      temp[0] = q;
      dummy = randlc(temp, q);
      q = temp[0];
      n = n2;
    }
    else {
      n = n * exp2;
      two_pow = false;
    }
  }

  while (n > 1) {
    n2 = n/2;
    if (n2*2 == n) {
      temp[0] = q;
      dummy = randlc(temp, q);
      q = temp[0];
      n = n2;
    }
    else {
      temp[0] = r;
      dummy = randlc(temp, q);
      r = temp[0];
      n = n-1;
    }
  }

  temp[0] = r;
  dummy = randlc(temp, q);
  r = temp[0];
  return r;
}
