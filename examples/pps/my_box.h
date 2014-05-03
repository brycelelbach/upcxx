#pragma once

#include "globals.h"

/** This class performs all the simple domain calculus functions that
    come up frequently in codes of this sort. */
class my_box {
private:
  my_box() {}

public:
  static RectDomain<2> ngsrbbox(RectDomain<2> a, Point<2> offset) {
    return RectDomain<2>((a.min() + POINT(1, 1) + offset)/2,
                         (a.max() + offset)/2 + POINT(1, 1));
  }

  static RectDomain<2> ncoarsen(RectDomain<2> a, int nref) {
    return RectDomain<2>(a.min()/nref,
                         ((a.max() + POINT(-1, -1))/nref) +
                         POINT(2, 2));
  }

  static RectDomain<2> nrefine(RectDomain<2> a, int nref) {
    return RectDomain<2>(a.min()*nref, a.max()*nref + POINT(1, 1));
  }

  static RectDomain<2> gsrbbox(RectDomain<2> a, Point<2> offset) {
    return RectDomain<2>((a.min() + POINT(1, 1) + offset)/2,
                         (a.max() + offset)/2 + POINT(1, 1));
  }

  static RectDomain<2> coarsen(RectDomain<2> a, int nref) {
    return RectDomain<2>(a.min()/nref, a.max()/nref + POINT(1, 1));
  }

  static RectDomain<2> refine(RectDomain<2> a, int nref) {
    return RectDomain<2>(a.min()*nref,
                         (a.max() + POINT(1, 1))*nref);
  }
};
