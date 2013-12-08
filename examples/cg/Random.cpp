#include <cmath>
#include "Random.h"

double Random::d2m46=pow(0.5,46);
int64_t Random::i246m1 = (int64_t)pow(2,46)-1;

double Random::randlc(double x, double a){
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
  t1 = r23 * x;
  x1 = (int) t1;
  x2 = x - t23 * x1;
  t1 = a1 * x2 + a2 * x1;
  t2 = (int) (r23 * t1);
  z = t1 - t23 * t2;
  t3 = t23 * z + a2 * x2;
  t4 = (int) (r46 * t3);
  x = t3 - t46 * t4;
  return x;
}

//Random number generator with an internal seed
double Random::randlc(double a){
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
  t1 = r23 * tran;
  x1 = (int) t1;
  x2 = tran - t23 * x1;
  t1 = a1 * x2 + a2 * x1;
  t2 = (int) (r23 * t1);
  z = t1 - t23 * t2;
  t3 = t23 * z + a2 * x2;
  t4 = (int) (r46 * t3);
  tran = t3 - t46 * t4;
  return(r46 * tran);
}

double Random::ipow46(double a, int exponent) {
  int n, n2;
  double q, r;
  //---------------------------------------------------------------------
  // Use
  //   a^n = a^(n/2)*a^(n/2) if n even else
  //   a^n = a*a^(n-1)       if n odd
  //---------------------------------------------------------------------
  if (exponent == 0) return seed;
  q = a;
  r = 1;
  n = exponent;
	
  while (n>1) {
    n2 = n/2;
    if (n2*2==n){
      seed = randlc(q,q);
      q=seed;
      n = n2;
    }
    else{
      seed = randlc(r,q);
      r=seed;
      n = n-1;
    }
  }
  seed = randlc(r,q);
  return seed;
}

double Random::power(double a, int n) {
  //c---------------------------------------------------------------------
  //c     power  raises an integer, disguised as a double
  //c     precision real, to an integer power
  //c---------------------------------------------------------------------
  double aj,ajj,pow;
  int nj;
	
  pow = 1.0;
  nj = n;
  aj = a;
  while (nj != 0){
    if (nj%2==1) {
      seed=randlc(pow, aj);
      pow=seed;
    }
    ajj=aj;
    seed=randlc(aj, ajj);
    aj=seed;
    nj = nj/2;
  }
  return pow;
}
