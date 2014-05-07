#pragma once

#include "globals.h"
#include "Vector.h"

/*  This class implements the conjugate gradient method. */

class CG {
 public:
  Vector *p, *q, *z; // reused Vectors; z shared with CGDriver instance

  CG() {
    p = new Vector();
    q = new Vector();
    z = new Vector();
  }

  // find z such that A * z = x
  Vector *cg(SparseMat *A, Vector *x, Vector *r) {
    double d, rho;
    double alpha, rho0, beta;

    r->copy(*x);
    p->copy(*x);
    z->zeroOut(); // reset z
	
    rho = r->dot(*r);

    for (int iterNum=1; iterNum <= 25; iterNum++) {
      A->multiply(*q, *p);      // q = A * p
      d = p->dot(*q);
      alpha = rho/d;
      z->axpy(alpha, *p, *z);
      r->axpy(-alpha, *q, *r);
      rho0 = rho;
      rho = r->dot(*r);
      beta = rho/rho0;
      p->axpy(beta, *p, *r);
    }
    return z;
  }
};
