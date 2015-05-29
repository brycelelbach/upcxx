/* point constants (z = zero, m = minus 1, p = plus 1) */
#define Pmmm(p) (p + PT(-1,-1,-1))
#define Pmmz(p) (p + PT(-1,-1,0))
#define Pmmp(p) (p + PT(-1,-1,1))
#define Pmzm(p) (p + PT(-1,0,-1))
#define Pmzz(p) (p + PT(-1,0,0))
#define Pmzp(p) (p + PT(-1,0,1))
#define Pmpm(p) (p + PT(-1,1,-1))
#define Pmpz(p) (p + PT(-1,1,0))
#define Pmpp(p) (p + PT(-1,1,1))
#define Pzmm(p) (p + PT(0,-1,-1))
#define Pzmz(p) (p + PT(0,-1,0))
#define Pzmp(p) (p + PT(0,-1,1))
#define Pzzm(p) (p + PT(0,0,-1))
#define Pzzz(p) (p + PT(0,0,0))
#define Pzzp(p) (p + PT(0,0,1))
#define Pzpm(p) (p + PT(0,1,-1))
#define Pzpz(p) (p + PT(0,1,0))
#define Pzpp(p) (p + PT(0,1,1))
#define Ppmm(p) (p + PT(1,-1,-1))
#define Ppmz(p) (p + PT(1,-1,0))
#define Ppmp(p) (p + PT(1,-1,1))
#define Ppzm(p) (p + PT(1,0,-1))
#define Ppzz(p) (p + PT(1,0,0))
#define Ppzp(p) (p + PT(1,0,1))
#define Pppm(p) (p + PT(1,1,-1))
#define Pppz(p) (p + PT(1,1,0))
#define Pppp(p) (p + PT(1,1,1))
/* split indexing constants */
#define Pmmms(i, j, k) i-1, j-1, k-1
#define Pmmzs(i, j, k) i-1, j-1, k
#define Pmmps(i, j, k) i-1, j-1, k+1
#define Pmzms(i, j, k) i-1, j, k-1
#define Pmzzs(i, j, k) i-1, j, k
#define Pmzps(i, j, k) i-1, j, k+1
#define Pmpms(i, j, k) i-1, j+1, k-1
#define Pmpzs(i, j, k) i-1, j+1, k
#define Pmpps(i, j, k) i-1, j+1, k+1
#define Pzmms(i, j, k) i, j-1, k-1
#define Pzmzs(i, j, k) i, j-1, k
#define Pzmps(i, j, k) i, j-1, k+1
#define Pzzms(i, j, k) i, j, k-1
#define Pzzzs(i, j, k) i, j, k
#define Pzzps(i, j, k) i, j, k+1
#define Pzpms(i, j, k) i, j+1, k-1
#define Pzpzs(i, j, k) i, j+1, k
#define Pzpps(i, j, k) i, j+1, k+1
#define Ppmms(i, j, k) i+1, j-1, k-1
#define Ppmzs(i, j, k) i+1, j-1, k
#define Ppmps(i, j, k) i+1, j-1, k+1
#define Ppzms(i, j, k) i+1, j, k-1
#define Ppzzs(i, j, k) i+1, j, k
#define Ppzps(i, j, k) i+1, j, k+1
#define Pppms(i, j, k) i+1, j+1, k-1
#define Pppzs(i, j, k) i+1, j+1, k
#define Pppps(i, j, k) i+1, j+1, k+1
    
/* A = laplacian operator */
#define A0 (-8.0/3.0)
#define A1 0.0
#define A2 (1.0/6.0)
#define A3 (1.0/12.0)
/* P = restrict (coarsen) operator */
#define P0 0.5
#define P1 0.25
#define P2 0.125
#define P3 0.0625
/* Q = prolongate (interpolate) operator */
#define Q0 1.0
#define Q1 0.5
#define Q2 0.25
#define Q3 0.125
/* S = Smoother */
extern double S0, S1, S2;
/* #ifdef CLASS_A_S_OR_W */
/* #define S0 (-.375) */
/* #define S1 .03125 */
/* #define S2 (-.015625) */
/* #else */
/* #define S0 (-3.0/17.0) */
/* #define S1 (1.0/33.0) */
/* #define S2 (-1.0/61.0) */
/* #endif */
#define S3 0.0
