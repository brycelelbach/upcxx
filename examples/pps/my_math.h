#pragma once

#include <cmath>
#include "globals.h"
#include "my_box.h"

static const double PI = 3.141592653589793238463;

/** This class has a lot of the math routines that I am likely to do.
    It would probably be better if at least some of this were
    somewhere else, perhaps, but I haven't figured out a better way of
    oraganizing it all. */
class my_math {
private:
  my_math() {} // disable construction

public:
  /** Cell-centered averaging.  This is no longer used in my code, but
      it is useful to have for other implementations.  Specifically,
      this code calculates the average of the four fine cells at the
      center of each projected coarse cell, and assigns that value to
      the coarse cell. */
  static void ccaverage(ndarray<double, 2, global> fine,
                        ndarray<double, 2, global> coarse,
                        int nref);

  /** Simple sampling.  This is the "averaging" operator used in my
      code.  This only works for node-centered data, though, where
      fine and coarse grid points coincide. */
  static void sample(ndarray<double, 2, global> fine,
                     ndarray<double, 2, global> coarse,
                     int nref) {
    foreach (p, coarse.domain()
	     *(my_box::ncoarsen(fine.domain().accrete(1), nref).shrink(1))) {
      coarse[p] = fine[p*nref];
    }
  }

  /** Set the boundary values of an array to zero. */
  static void zerobc(ndarray<double, 2, global> a);

  /** Calculate the sum of all the charge in an array. */
  static double sumcharge(ndarray<double, 2, global> a, double h) {
    double q = 0.0;
    foreach (p, a.domain()) {
      q += a[p];
    }
    q *= h*h;

    return q;
  }

  /** Calculate the center of the charge, or the first moment of the
      charge. */
  static ndarray<double, 1> centerofcharge(ndarray<double, 2, global> a,
                                           double q, double h);

  /** Set the boundary of a solution array with an estimate of an
      infinite domain boundary condition.  This estimate is calculated
      by approximating the charge field as a single point charge
      located at the first moment of the charge field. */
  static void nidbound(ndarray<double, 2, global> soln,
                       ndarray<double, 2, global> rhs, double h);
  
  /** Apparently not used? */
  //    static void edgebc(ndarray<double, 2, global> a) {
  //      ndarray<double, 1, global> side;
  //      ndarray<double, 1, global> inside;

  //      /** Top edge */
  //      side = a.slice(2, a.domain().max()[2]);
  //      inside = a.slice(2, a.domain().max()[2] - 1);
  //      foreach (p, side.domain()) {
  //        side[p] = 2.0*side[p] - inside[p];
  //      }

  //      /** Bottom edge */
  //      side = a.slice(2, a.domain().min()[2]);
  //      inside = a.slice(2, a.domain().min()[2] + 1);
  //      foreach (p, side.domain()) {
  //        side[p] = 2.0*side[p] - inside[p];
  //      }

  //      /** Right edge */
  //      side = a.slice(1, a.domain().max()[1]);
  //      inside = a.slice(1, a.domain().max()[1] - 1);
  //      foreach (p, side.domain()) {
  //        side[p] = 2.0*side[p] - inside[p];
  //      }

  //      /** Left edge */
  //      side = a.slice(1, a.domain().min()[1]);
  //      inside = a.slice(1, a.domain().min()[1] + 1);
  //      foreach (p, side.domain()) {
  //        side[p] = 2.0*side[p] - inside[p];
  //      }
  //    }


  /** This method calculates the nine-point laplacian of an input
      grid. */
  static void nlaplacian(ndarray<double, 2, global> input,
                         ndarray<double, 2, global> lapl, double h);

  /** This method calculates the five-point laplacian of an input
      grid. */
  static void nlaplacian5(ndarray<double, 2, global> input,
                          ndarray<double, 2, global> lapl, double h);

  /** This is a big method.  It calculates the boundary condition on a
      large grid due to the potential field caused by a discontinuity
      in potential at the boundary of a smaller, inner grid.  In
      pseudo-math the equation we are using is
      phi(y : l) = sum_{x : l_inner} (-q(x)*G(x - y)),
      where l is the boundary of the large grid and l_inner is the
      boundary of the smaller, inner grid.  G(z) is the Green's
      function for our 2d problem: G(z) = (1/(2pi))*log(|z|).  The
      charge, q, is the difference between the normal derivative of
      phi_inner at the boundary l_inner and the value predicted for a
      simple point charge (as calculated by nidbound above). */
  static void nodal_potential_calc(ndarray<double, 2, global> phiH,
                                   ndarray<double, 2, global> phi,
                                   ndarray<double, 2, global> extra,
                                   ndarray<double, 2, global> rhs,
                                   double h);

  /** This is a lower-accuracy version of the nodal_potential_calc
      method, designed to provide sufficient accuracy to be compatible
      with a five-point laplacian operator. */
  static void nodal_potential_calc5(ndarray<double, 2, global> phiH,
                                    ndarray<double, 2, global> phi,
                                    ndarray<double, 2, global> extra,
                                    ndarray<double, 2, global> rhs,
                                    double h);

  /** Code to make a star-shaped right hand side. */
  static void make_star_rhs(ndarray<double, 2, global> rhs, double h,
                            ndarray<double, 1, global> center,
                            double r0, int m);  

  /** Code to make a nice, round right hand side. */
  static void makeRhs(ndarray<double, 2, global> rhs, double h, 
                      ndarray<double, 1, global> center,
                      double r0, int m);

  /** This method calculates the exact solution expected for several
      of the possible right hand sides defined above.  No analytical
      solution is available for the star-shaped right hand side.  For
      the high wavenumber solution, a closed-form analytical solution
      does not exist, so there is a lot of hoop jumping to get an
      accurate representation of the solution in the high wavenumber
      case. */
  static void exactSoln(ndarray<double, 2, global> soln, double h,
                        ndarray<double, 1, global> center,
                        double r0, int m);

  /** This is one of the hoops that I have to jump through since a
      closed-form solution does not exist for the high wavenumber
      right hand side.  I need to calculate a good estimate of
      something that behaves like a log but disappears at the
      origin. */
  static double log_type_term(double m, double r, double r0);

  /** This method calculates the L_2 norm of a grid. */
  static double norm2(ndarray<double, 2, global> grid) {
    double sum = 0.;
    foreach(p, grid.domain()) {
      sum += grid[p]*grid[p];
    }
    return sqrt(sum);
  }


  /** This method calculate the L_1 norm of a grid. */
  static double norm1(ndarray<double, 2, global> grid) {
    double sum = 0.;
    foreach(p, grid.domain()) {
      sum += abs(grid[p]);
    }
    return sum;
  }


  /** This method calculates the max norm of a grid. */
  static double normMax(ndarray<double, 2, global> grid) {
    double max = 0.;
    foreach(p, grid.domain()) {
      max = (abs(grid[p]) > max) ? abs(grid[p]) : max;
    }
    return max;
  }


  /** This method calculates sum_{i, j} | x_{i, j} |. */
  static double sumabs(ndarray<double, 2, global> grid) {
    double sum = 0;
    foreach (p, grid.domain()) {
      sum += abs(grid[p]);
    }
    return sum;
  }


  /** This method calculates sum_{i, j} (x_{i, j})^2. */
  static double sumsquared(ndarray<double, 2, global> grid) {
    double sum = 0;
    foreach (p, grid.domain()) {
      sum += grid[p]*grid[p];
    }
    return sum;
  }
};
