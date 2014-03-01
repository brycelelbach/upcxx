#include <cstdlib>
#include <cstring>
#include "globals.h"
#include "my_box.h"
#include "my_math.h"
#include "nodal_infdmn.h"
#include "plot.h"
#include "pmg_test.h"

execution_log pmg_test::log;

/** This is a program to test just the infinite domain solver.  Since
    I believe the infinite domain solver is performing as specified,
    this isn't so necessary anymore. */
int main(int argc, char **argv) {
  init(&argc, &argv);
  pmg_test::log.init(2);

  int C = 0;
  int LongDim = 32;
  int IDBorder = 10;
  bool mehrstellen = true;
  int M = 0;

  for (int i = 1; (i < argc && !strncmp(argv[i], "-", 1)); 
       i++) {
    if (!strncmp(argv[i], "-C", 2)) {
      C = atoi(argv[++i]);
    }
    else if (!strncmp(argv[i], "-I", 2)) {
      IDBorder = atoi(argv[++i]);
    }
    else if (!strncmp(argv[i], "-L", 2)) {
      LongDim = atoi(argv[++i]);
    }
    else if (!strncmp(argv[i], "-no", 3)) {
      mehrstellen = false;
    }
    else if (!strncmp(argv[i], "-W", 2)) {
      M = atoi(argv[++i]);
    }
  }

  RectDomain<2> interior(POINT(0, 0), POINT(LongDim+1, LongDim+1));
  println("C: " << C << ", IDBorder: " << IDBorder);
  double H = 1.0/LongDim;

  int Border = 0;
  if (C > 0) {
    Border = C;
  }
  else if (IDBorder > 0) {
    Border = IDBorder*(LongDim)/100;
  }

  int SBorder = broadcast(Border, 0);
  println(SBorder);
  if (SBorder > 0) {
    RectDomain<2> large = interior.accrete(Border);
    large = my_box::nrefine(my_box::ncoarsen(large, 4), 4);
      
    Point<2> lowH, highH;

    lowH = (large.min() + interior.min())/POINT(2, 2);
    highH  = (large.max() + interior.max())/POINT(2, 2);

    lowH = (lowH/2)*2;
    highH = ((highH - POINT(1, 1))/2 + POINT(1, 1))*2;

    if (!(large.shrink(1).contains(lowH) 
          && large.shrink(1).contains(highH))) {
      large = large.accrete(4);
    }
    ndarray<double, 2> soln(large);
    ndarray<double, 2> mehr(interior);
    ndarray<double, 2> exact(interior);
    ndarray<double, 2> rhs(interior);

    double r0 = 0.425;
    int m = M;
    ndarray<double, 1> center(RECTDOMAIN((1), (3)));
    RectDomain<1> locations(POINT(0), POINT(1));
    foreach (p, locations) {
      foreach (i, center.domain()) {
        center[i] = 0.475 + p[1]*0.25;
      }
      my_math::makeRhs(rhs, H, center, r0, m);
      my_math::exactSoln(exact, H, center, r0, m);
    }

    //my_math::nlaplacian(rhs, mehr, H);
    foreach (p, mehr.domain().shrink(1)) {
      mehr[p] = 2.0*rhs[p]/3.0 + (rhs[p + POINT(1, 0)] +
                                  rhs[p + POINT(-1, 0)] +
                                  rhs[p + POINT(0, 1)] +
                                  rhs[p + POINT(0, -1)])/12.0;
    }
    if (mehrstellen) {
      nodal_infdmn::solve(mehr, soln, H, 0);
    }
    else {
      nodal_infdmn::solve(rhs, soln, H, 0);
    }

    foreach (p, exact.domain()) {
      exact[p] -= soln[p];
    }

    plot::dump(exact, "iderror");
    println("L2 error: " << H*my_math::norm2(exact));
    println("L1 error: " << H*H*my_math::norm1(exact));
    println("Max error: " << my_math::normMax(exact));
  }

  finalize();
  return 0;
}
