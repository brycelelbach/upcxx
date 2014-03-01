#pragma once

#include <iostream>
#include "globals.h"
#include "my_math.h"

/*
  Things that make this code go faster: (all tests on a 16x16 grid with only
  gsrb rather than full multigrid)
  (improvements listed in the reverse order I discovered them)
  
  1 - replacing rbDomain array with four individual RectDomains
      performance improved about 8%

  2 - replacing (simple) offset arrays with more (complicated) math
      performance improved about 42%

  3 - replacing slicing on the fly with pre-sliced boundaries to set to zero
      performance improved about 22%
*/

/** This is a node-centered multigrid, fairly simple, but including a 
    w-cycle and the ability to do growing multigrid. */
class nodal_multigrid {
public:
  ndarray<double, 2> phi;
  ndarray<double, 2> rhs;
private:
  RectDomain<2> interior;
  ndarray<double, 2> res;
  ndarray<double, 2> restmp;
  nodal_multigrid *next;
  RectDomain<2> bcoarse;
  double h;
  //ndarray<RectDomain<2>, 2> rbDomain;

  /*faster 1*/
  RectDomain<2> rbdSW;
  RectDomain<2> rbdNW;
  RectDomain<2> rbdNE;
  RectDomain<2> rbdSE;

  RectDomain<1> ibn, ibs, ibe, ibw;
  ndarray<double, 1> nside;
  ndarray<double, 1> sside;
  ndarray<double, 1> eside;
  ndarray<double, 1> wside;

public:
  nodal_multigrid() {}

  nodal_multigrid(RectDomain<2> box, double H) {
    setDomain(box, H);
  }

  // Print a string representation to the given stream.
  friend std::ostream& operator<<(std::ostream& os,
                                  const nodal_multigrid& nmg) {
    os << "mg:" << " " << nmg.interior;
    return os;
  }

  void setDomain(RectDomain<2> box, double H);

  /** A v-cycle relaxation. */
  void relax();

  /** A w-cycle relaxation. */
  void wrelax();

  void wrelax5();

  /** Gauss-Seidel with Red-Black ordering. */
  void gsrb() {
    /* setBC() is not really necessary as long as nothing writes to the
       boundary of phi.  I'll remove the calls to setBC after I check that
       and when I want the code to run ever so slightly faster. */
    setBC();
    redRelax();
    setBC();
    blackRelax();
  }
  
  /** Gauss-Seidel with four-color ordering */
  void gsfc() {
    /* Pink hearts */
    point_relax_fc(POINT(0, 0), rbdSW, h*h);
    /* Yellow moons */
    point_relax_fc(POINT(1, 1), rbdNE, h*h);
    /* Blue stars */
    point_relax_fc(POINT(0, 1), rbdNW, h*h);
    /* Green clovers */
    point_relax_fc(POINT(1, 0), rbdSE, h*h);
  }

  void gsrb5() {
    /* setBC() is not really necessary as long as nothing writes to the
       boundary of phi.  I'll remove the calls to setBC after I check that
       and when I want the code to run ever so slightly faster. */
    setBC();
    redRelax5();
    setBC();
    blackRelax5();
  }

  void redRelax() {
    pointRelax(POINT(0, 0) /*faster 1*/ , rbdSW, h*h);
    pointRelax(POINT(1, 1) /*faster 1*/ , rbdNE, h*h);
    pointCopy(POINT(0, 0) /*faster 1*/ , rbdSW);
    pointCopy(POINT(1, 1) /*faster 1*/ , rbdNE);
  }

  void blackRelax() {
    pointRelax(POINT(0, 1) /*faster 1*/ , rbdNW, h*h);
    pointRelax(POINT(1, 0) /*faster 1*/ , rbdSE, h*h);
    pointCopy(POINT(0, 1) /*faster 1*/ , rbdNW);
    pointCopy(POINT(1, 0) /*faster 1*/ , rbdSE);
  }

  void redRelax5() {
    pointRelax5(POINT(0, 0), rbdSW, h*h);
    pointRelax5(POINT(1, 1), rbdNE, h*h);
  }

  void blackRelax5() {
    pointRelax5(POINT(0, 1), rbdNW, h*h);
    pointRelax5(POINT(1, 0), rbdSE, h*h);
  }

  void pointRelax(Point<2> offset, /*faster 1*/ RectDomain<2> d,
                  double hsquared);

  void point_relax_fc(Point<2> offset, RectDomain<2> d,
                      double hsquared);

  void pointRelax5(Point<2> offset, /*faster 1*/ RectDomain<2> d,
                   double hsquared);

  void pointCopy(Point<2> offset /*faster 1*/, RectDomain<2> d);

  void setBC();

  void average(ndarray<double, 2> fromArray,
               ndarray<double, 2> toArray);

  void interp(ndarray<double, 2> fromArray,
              ndarray<double, 2> toArray);

  void resid();

  void resid5();

  double errorest() {
    resid();
    return h*my_math::norm2(res);
  }

  double errorest5() {
    resid5();
    return h*my_math::norm2(res);
  }

  ndarray<double, 2> getRhs() {
    return rhs;
  }
  
  ndarray<double, 2> getSoln() {
    return phi;
  }
};
