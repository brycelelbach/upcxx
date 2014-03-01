#pragma once

#include <string>
#include "globals.h"
#include "my_math.h"
#include "boxedlist.h"

class amr_level : my_math {
private:
  ndarray<ndarray<double, 2, global>, 1> solution;
  ndarray<ndarray<double, 2, global>, 1> phi;
  ndarray<ndarray<double, 2, global>, 1> rho;
  ndarray<ndarray<double, 2, global>, 1> res;
  ndarray<int, 1> owner;
  ndarray<RectDomain<2>, 1> interior;
  ndarray<ndarray<Point<1>, 1>, 1> neighbor;
  double h;
//    static timer math;
//    static timer communication;
//    static timer barrier_wait;
//    static timer ghost;
//    static timer relaxation;
  static string bb;
  static string eb;
  static string bfg;
  static string efg;
  static string bm;
  static string em;

public:
  amr_level() {}
  
  amr_level(ndarray<RectDomain<2>, 1> grids, double dx);

  amr_level(ndarray<ndarray<double, 2, global>, 1> input,
            ndarray<ndarray<double, 2, global>, 1> soln, 
            ndarray<int, 1> assignments, double dx);

  void form_neighbor_list();

  void verify() {
    foreach (i, phi.domain()) {
      if (owner[i] == MYTHREAD) {
	println(i[1] << " owned by processor " << MYTHREAD);
      }
    }
  }

  void fill_ghost();

  void smooth_level();

  void point_relax(Point<2> offset, ndarray<double, 2> phi,
                   ndarray<double, 2> rhs, double hsquared);

  double errorest_level();

  double level_norm2(ndarray<ndarray<double, 2, global>, 1> a);
};
