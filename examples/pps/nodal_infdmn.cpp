#include <string>
#include "nodal_infdmn.h"
#include "nodal_multigrid.h"
#include "pmg_test.h"
#include "my_math.h"

void nodal_infdmn::solve(ndarray<double, 2> rhs, 
                         ndarray<double, 2> soln, 
                         double h, int report_level) {
  Point<2> lowH, highH;
  Point<2> e1 = POINT(1, 1);
  Point<2> e2 = POINT(2, 2);

  lowH = soln.domain().min();
  lowH = lowH + rhs.domain().min();
  lowH = lowH/e2;
  highH = soln.domain().max();
  highH = highH + rhs.domain().max();
  highH = highH/e2;

  lowH = (lowH/2)*2;
  highH = ((highH - e1)/2 + e1)*2;
  RectDomain<2> dH(lowH,  highH + POINT(1, 1));
  nodal_multigrid aH(dH, h);

  my_math::nidbound(aH.phi, rhs, h);
  my_math::nlaplacian(aH.phi, aH.rhs, h);

  aH.phi.set(0.);
  foreach (p, aH.rhs.domain()) {
    aH.rhs[p] = - aH.rhs[p];
  }
  foreach (p, aH.rhs.domain()*rhs.domain()) {
    aH.rhs[p] = aH.rhs[p] + rhs[p];
  }

  double error0, error;

  error0 = aH.errorest();
  error = error0;

  int ii;
  string phase = "first pass in IDsolve";
  pmg_test::log.begin(phase);
  for (ii = 0; (ii < 50) & (error > 1.e-10*error0); ii++) {
    aH.wrelax();
    error = aH.errorest();
  }
  pmg_test::log.end(phase);

  if (report_level > 0) {
    println("Inner: iteration = " << ii << ", e = " << error
            << ", e0 = " << error0);
  }

  my_math::zerobc(aH.getSoln());
  my_math::nidbound(aH.getSoln(), rhs, h);

  RectDomain<2> dF = soln.domain();

  if (report_level > 0) {
    println(MYTHREAD << ": medium box: " << dH);
    println(MYTHREAD << ": big box: " << dF);
  }

  nodal_multigrid a(dF, h);

  a.rhs.set(0.);
  a.phi.set(0.);

  ndarray<double, 2> extra(soln.domain());
  extra.set(0.);
  phase = "potential calculation";
  pmg_test::log.begin(phase);

  my_math::nodal_potential_calc(aH.phi, soln, extra, rhs, h);

  pmg_test::log.end(phase);


  my_math::nidbound(soln, rhs, h);
  my_math::nlaplacian(soln, a.rhs, h);

  a.phi.set(0.);
  foreach (p, a.rhs.domain()) {
    a.rhs[p] = - a.rhs[p];
  }

  foreach (p, a.rhs.domain()*rhs.domain()) {
    a.rhs[p] = a.rhs[p] + rhs[p];
  }

  error0 = a.errorest();
  error = error0;

  phase = "second pass in IDsolve";
  pmg_test::log.begin(phase);

  for (ii = 0; (ii < 50) & (error > 1.e-12*error0); ii++) {
    a.wrelax();
    error = a.errorest();
  }
  pmg_test::log.end(phase);
    
  if (report_level > 0) {
    println("Outer: iteration = " << ii << ", e = " << error
            << ", e0 = " << error0);
  }

  foreach(p, soln.domain().shrink(1)) {
    soln[p] = a.phi[p];
  }
}

void nodal_infdmn::solve95(ndarray<double, 2> rhs, ndarray<double, 2>
			   soln, double h) {
  Point<2> lowH, highH;
  Point<2> e1 = POINT(1, 1);
  Point<2> e2 = POINT(2, 2);

  lowH = soln.domain().min();
  lowH = lowH + rhs.domain().min();
  lowH = lowH/e2;
  highH = soln.domain().max();
  highH = highH + rhs.domain().max();
  highH = highH/e2;

  lowH = (lowH/4)*4;
  highH = ((highH - e1)/4 + e1)*4;
  RectDomain<2> dH(lowH, highH + POINT(1, 1));

  nodal_multigrid aH(dH, h);
    
  my_math::nidbound(aH.phi, rhs, h);
  my_math::nlaplacian5(aH.phi, aH.rhs, h);

  aH.phi.set(0.);
  foreach (p, aH.rhs.domain()) {
    aH.rhs[p] = - aH.rhs[p];
  }
  foreach (p, aH.rhs.domain()*rhs.domain()) {
    aH.rhs[p] = aH.rhs[p] + rhs[p];
  }

  double error0, error;

  error0 = aH.errorest5();
  error = error0;

  int ii;
  for (ii = 0; (ii < 50) & (error > 1.e-10*error0); ii++) {
    aH.wrelax5();
    error = aH.errorest5();
  }
  /*
    println("Inner: iteration = " << ii << ", e = " << error
            << ", e0 = " << error0);
  */
  /* println("time " + firstPassTime.total()/1000.0); */

  my_math::zerobc(aH.getSoln());
  my_math::nidbound(aH.getSoln(), rhs, h);

  RectDomain<2> dF = soln.domain();
  /*
    println(MYTHREAD << ": small box: " << rhs.domain());
    println(MYTHREAD << ": medium box: " << dH);
    println(MYTHREAD << ": big box: " << dF);
  */
  nodal_multigrid a(dF, h);

  a.rhs.set(0.);
  a.phi.set(0.);

  ndarray<double, 2> extra(soln.domain());
  extra.set(0.);

  my_math::nodal_potential_calc(aH.phi, soln, extra, rhs, h);

  my_math::nidbound(soln, rhs, h);
  my_math::nlaplacian5(soln, a.rhs, h);

  a.phi.set(0.);
  foreach (p, a.rhs.domain()) {
    a.rhs[p] = - a.rhs[p];
  }

  foreach (p, a.rhs.domain()*rhs.domain()) {
    a.rhs[p] = a.rhs[p] + rhs[p];
  }

  error0 = a.errorest5();
  error = error0;

  for (ii = 0; (ii < 50) & (error > 1.e-12*error0); ii++) {
    a.wrelax5();
    error = a.errorest5();
  }
  /*
    println("Outer: iteration = " << ii << ", e = " << error
            << ", e0 = " << error0);
  */
  /* println("time " + secondPassTime.total()/1000.0); */

  foreach(p, soln.domain().shrink(1)) {
    soln[p] = a.phi[p];
  }
}

/* This is the same, but it calls a five-point multigrid method and has
   matching reduced accuracy elsewhere. */
void nodal_infdmn::solve5(ndarray<double, 2> rhs, ndarray<double, 2>
                          soln, double h) {
  Point<2> lowH, highH;
  Point<2> e1 = POINT(1, 1);
  Point<2> e2 = POINT(2, 2);

  lowH = soln.domain().min();
  lowH = lowH + rhs.domain().min();
  lowH = lowH/e2;
  highH = soln.domain().max();
  highH = highH + rhs.domain().max();
  highH = highH/e2;

  lowH = (lowH/2)*2;
  highH = ((highH - e1)/2 + e1)*2;
  RectDomain<2> dH(lowH, highH + POINT(1, 1));

  nodal_multigrid aH(dH, h);
    
  my_math::nidbound(aH.phi, rhs, h);
  my_math::nlaplacian5(aH.phi, aH.rhs, h);

  aH.phi.set(0.);
  foreach (p, aH.rhs.domain()) {
    aH.rhs[p] = - aH.rhs[p];
  }
  foreach (p, aH.rhs.domain()*rhs.domain()) {
    aH.rhs[p] = aH.rhs[p] + rhs[p];
  }

  double error0, error;

  error0 = aH.errorest5();
  error = error0;

  int ii;
  for (ii = 0; (ii < 50) & (error > 1.e-10*error0); ii++) {
    aH.wrelax5();
    error = aH.errorest5();
  }
  /*
    println("Inner: iteration = " << ii << ", e = " << error
            << ", e0 = " << error0);
  */
  /* println("time " << (firstPassTime.total()/1000.0)); */

  my_math::zerobc(aH.getSoln());
  my_math::nidbound(aH.getSoln(), rhs, h);

  RectDomain<2> dF = soln.domain();
  /*
    println(MYTHREAD + ": small box: " 
    + new rhs.domain()); 
    println(MYTHREAD + ": medium box: " + dH);
    println(MYTHREAD + ": big box: " + dF);
  */
  nodal_multigrid a(dF, h);

  a.rhs.set(0.);
  a.phi.set(0.);

  ndarray<double, 2> extra(soln.domain());
  extra.set(0.);

  my_math::nodal_potential_calc5(aH.phi, soln, extra, rhs, h);

  /* plot.dump(soln, "setBound"); */

  my_math::nidbound(soln, rhs, h);
  my_math::nlaplacian5(soln, a.rhs, h);

  a.phi.set(0.);
  foreach (p, a.rhs.domain()) {
    a.rhs[p] = - a.rhs[p];
  }

  foreach (p, a.rhs.domain()*rhs.domain()) {
    a.rhs[p] = a.rhs[p] + rhs[p];
  }

  error0 = a.errorest5();
  error = error0;

  for (ii = 0; (ii < 50) & (error > 1.e-12*error0); ii++) {
    a.wrelax5();
    error = a.errorest5();
  }
  /* println("Outer: iteration = " << ii << ", e = " << error
             << ", e0 = " << error0); */
  /* println("time " << (secondPassTime.total()/1000.0)); */

  foreach(p, soln.domain().shrink(1)) {
    soln[p] = a.phi[p];
  }
}

void nodal_infdmn::solve59(ndarray<double, 2> rhs, ndarray<double, 2>
			   soln, double h) {
  Point<2> lowH, highH;
  Point<2> e1 = POINT(1, 1);
  Point<2> e2 = POINT(2, 2);

  lowH = soln.domain().min();
  lowH = lowH + rhs.domain().min();
  lowH = lowH/e2;
  highH = soln.domain().max();
  highH = highH + rhs.domain().max();
  highH = highH/e2;

  lowH = (lowH/2)*2;
  highH = ((highH - e1)/2 + e1)*2;
  RectDomain<2> dH(lowH, highH, POINT(1, 1));

  nodal_multigrid aH(dH, h);
    
  my_math::nidbound(aH.phi, rhs, h);
  my_math::nlaplacian(aH.phi, aH.rhs, h);

  aH.phi.set(0.);
  foreach (p, aH.rhs.domain()) {
    aH.rhs[p] = - aH.rhs[p];
  }
  foreach (p, aH.rhs.domain()*rhs.domain()) {
    aH.rhs[p] = aH.rhs[p] + rhs[p];
  }

  double error0, error;

  error0 = aH.errorest();
  error = error0;

  int ii;
  for (ii = 0; (ii < 50) & (error > 1.e-10*error0); ii++) {
    aH.wrelax();
    error = aH.errorest();
  }
  /*
    println("Inner: iteration = " << ii << ", e = " << error
            << ", e0 = " << error0);
  */
  /* println("time " << (firstPassTime.total()/1000.0)); */

  my_math::zerobc(aH.getSoln());
  my_math::nidbound(aH.getSoln(), rhs, h);

  RectDomain<2> dF = soln.domain();
  /*
    println(MYTHREAD + ": small box: " 
    + rhs.domain()); 
    println(MYTHREAD + ": medium box: " + dH);
    println(MYTHREAD + ": big box: " + dF);
  */
  nodal_multigrid a(dF, h);

  a.rhs.set(0.);
  a.phi.set(0.);

  ndarray<double, 2> extra(soln.domain());
  extra.set(0.);

  my_math::nodal_potential_calc5(aH.phi, soln, extra, rhs, h);

  /* plot.dump(soln, "setBound"); */

  my_math::nidbound(soln, rhs, h);
  my_math::nlaplacian(soln, a.rhs, h);

  a.phi.set(0.);
  foreach (p, a.rhs.domain()) {
    a.rhs[p] = - a.rhs[p];
  }

  foreach (p, a.rhs.domain()*rhs.domain()) {
    a.rhs[p] = a.rhs[p] + rhs[p];
  }

  error0 = a.errorest();
  error = error0;

  for (ii = 0; (ii < 50) & (error > 1.e-12*error0); ii++) {
    a.wrelax();
    error = a.errorest();
  }
  /* println("Outer: iteration = " << ii << ", e = " << error
             << ", e0 = " << error0); */
  /* println("time " << (secondPassTime.total()/1000.0)); */

  foreach(p, soln.domain().shrink(1)) {
    soln[p] = a.phi[p];
  }
}
