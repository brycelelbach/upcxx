#pragma once

#include "globals.h"
#include "charge.h"
#include "nodal_multigrid.h"

class pmg {
private:
  ndarray<charge, 1> charges;	// charge fields
  int c;		// correction radius
  int nref;		// refinement ratio
  int s;		// stencil for fine grids: 9 or 5
  double fine_h;	// fine grid spacing
  double coarse_h;	// coarse grid spacing

  RectDomain<2> fine_domain;    // domain of entire problem
  ndarray<RectDomain<2>, 1> fine_interior;     // local rhs domains
  ndarray<RectDomain<2>, 1> coarse_domain;     // local coarse domains

  /* The following arrays contain data which local to a processor. */
  ndarray<ndarray<double, 2>, 1> local_fine_rho; 
  ndarray<ndarray<double, 2>, 1> local_fine_phi; 
  ndarray<ndarray<double, 2>, 1> local_coarse_rho; 
  ndarray<ndarray<double, 2>, 1> local_coarse_phi;

  /* The following two arrays exist on all processors to represent the
     coarse problem. */
  ndarray<ndarray<double, 2, global>, 1> coarse_domain_rhs;
  ndarray<double, 2> coarse_domain_soln;

  /* The following two arrays point to data which can live on any
     processor - these arrays contain the solutions for the entire
     problem domain. */
  ndarray<ndarray<double, 2, global>, 1> global_fine_phi;
  ndarray<ndarray<double, 2, global>, 1> global_coarse_phi;

  /* The following two arrays are constructed to represent the set of
     patches which neighbor the given patch. */
  ndarray<ndarray<ndarray<ndarray<ndarray<double, 2>, 1>, 1>, 1>,
          1> all_neighboring_fine_data;
  ndarray<ndarray<ndarray<ndarray<ndarray<double, 2>, 1>, 1>, 1>,
          1> all_neighboring_coarse_data;

  /* I don't actually know what all_solutions does: it may not do what
     it's supposed to do.  final_solution holds the solution to the
     completed problem on each grid. */
  ndarray<nodal_multigrid, 1> final_solution;
  // ndarray<nodal_multigrid, 1> all_solutions;
  ndarray<ndarray<double, 2, global>, 1> all_solutions;

  timer communication_time;

public:
  /* These arrays are for i/o and error estimation. */
  ndarray<double, 2> full_solution;
  ndarray<ndarray<double, 2>, 1> exact_solution;
  int report_level;

  pmg() {}

  /** The real constructor - takes all the input I can ever imagine
      wanting, and sets up the data accordingly. */
  pmg(RectDomain<2> interior_domain, int npatches, int C,
      int Nref, int id_border, double h, int S, 
      ndarray<charge, 1> charge_fields, int reporting);

  /** This is called by the constructor to set up the arrays to hold
      data from neighboring patches.  Apparently since the data is
      local, the the method that sets it has to be local. */
  void construct_neighboring_data_arrays();

  /** solve() does all the high-level work to calculate the
      solution. */
  void solve();

  /** Distribute an array of patches cyclically to the processors. */
  ndarray<RectDomain<2>, 1> distribute(ndarray<RectDomain<2>,
                                       1> patches);

  /** Count how many patches this processor should own, so that the
      right size array can be built to store them. */
  int my_patch_count(int npatches, int nprocessors) {
    int mpc = 0;
    for (int ii = 0; ii < npatches; ii++) {
      if (MYTHREAD == ii%nprocessors) {
	mpc++;
      }
    }
    return mpc;
  }

  /** Copy all data from neighbors necessary to set the final fine
      grid boundary condition. */
  void copy_neighbor_data();
      
  /** Set the fine boundary for a patch i, where i is the index of
      _all_ patches, not just patches local to a processor. */
  void set_local_fine_boundary(Point<1> i);

  /** Build the solution from the data on the several processors,
      calculate the error, and write some data to files. */
  void wrap_up();

  /** Construct the entire solution out of the data on the several
      processors. */
  void compose_solution() {
    if (MYTHREAD == 0) {
      full_solution = ndarray<double, 2>(fine_domain);
      foreach (i, all_solutions.domain()) {
        full_solution.copy(all_solutions[i]);
      }
    }
  }

  ndarray<double, 2, global> compose_error(ndarray<ndarray<double, 2,
                                           global>, 1> distributed_error) {
    ndarray<double, 2, global> full_error;
    if (MYTHREAD == 0) {
      full_error = ndarray<double, 2, global>(fine_domain);
      foreach (i, distributed_error.domain()) {
	full_error.copy(distributed_error[i]);
      }
    }
    return broadcast(full_error, 0);
  }

  /** Determine if an exact solution can be found for our problem, and
      if so, calculate it.*/
  bool calculate_exact();

  /** Calculate the error in our solution.  This requires that the
      exact solution has already been calculated. */
  ndarray<ndarray<double, 2, global>, 1> get_error() {
    ndarray<ndarray<double, 2, global>, 1> error(exact_solution.domain());
    foreach (i, error.domain()) {
      error[i] = ndarray<double, 2>(exact_solution[i].domain());
      foreach (p, error[i].domain()) {
	error[i][p] = exact_solution[i][p] - final_solution[i].phi[p];
      }
    }
    return error;
  }
};
