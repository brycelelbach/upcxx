/* point constants (z = zero, m = minus 1, p = plus 1) */
#define Pmmm POINT(-1,-1,-1)
#define Pmmz POINT(-1,-1,0)
#define Pmmp POINT(-1,-1,1)
#define Pmzm POINT(-1,0,-1)
#define Pmzz POINT(-1,0,0)
#define Pmzp POINT(-1,0,1)
#define Pmpm POINT(-1,1,-1)
#define Pmpz POINT(-1,1,0)
#define Pmpp POINT(-1,1,1)
#define Pzmm POINT(0,-1,-1)
#define Pzmz POINT(0,-1,0)
#define Pzmp POINT(0,-1,1)
#define Pzzm POINT(0,0,-1)
#define Pzzz POINT(0,0,0)
#define Pzzp POINT(0,0,1)
#define Pzpm POINT(0,1,-1)
#define Pzpz POINT(0,1,0)
#define Pzpp POINT(0,1,1)
#define Ppmm POINT(1,-1,-1)
#define Ppmz POINT(1,-1,0)
#define Ppmp POINT(1,-1,1)
#define Ppzm POINT(1,0,-1)
#define Ppzz POINT(1,0,0)
#define Ppzp POINT(1,0,1)
#define Pppm POINT(1,1,-1)
#define Pppz POINT(1,1,0)
#define Pppp POINT(1,1,1)
    
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
#ifdef CLASS_A_S_OR_W
#define S0 (-.375)
#define S1 .03125
#define S2 (-.015625)
#else
#define S0 (-3.0/17.0)
#define S1 (1.0/33.0)
#define S2 (-1.0/61.0)
#endif
#define S3 0.0
