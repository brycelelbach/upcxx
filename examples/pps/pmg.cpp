#include <string>
#include <sstream>
#include "pmg.h"
#include "pmg_test.h"
#include "nodal_infdmn.h"
#include "plot.h"

/** The real constructor - takes all the input I can ever imagine
    wanting, and sets up the data accordingly. */
pmg::pmg(RectDomain<2> interior_domain, int npatches, int C,
         int Nref, int id_border, double h, int S, 
         ndarray<charge, 1> charge_fields, int reporting) {

  pmg_test::log.begin("1 of constructor");
  fine_domain = interior_domain;
  c = C;
  nref = Nref;
  report_level = reporting;
  communication_time.reset();

  /* border needs to be at least (c+1)*nref and id_border% of the
     side of the fine_domain.  I'm still assuming that fine_domain
     is square. */
  int border = ((c+1)*nref > id_border*(fine_domain.max()[1]
                                        - fine_domain.min()[1]
                                        )/(npatches*100)
                ? (c+1)*nref : id_border*(fine_domain.max()[1]
                                          - fine_domain.min()[1]
                                          )/(npatches*100));

  fine_h = h;
  coarse_h = h*Nref;
  s = broadcast(S, 0);
  charges = charge_fields;

  /* The following is a rather unclear method for setting up the
     patches.  Basically, npatches is the number of patches in each
     direction.  This code sets up npatches^2 total patches, and
     sizes them to cover the given area, interior_domain.  This will
     be poorly balanced if interior_domain and npatches are not
     numbers which play together well. */
  ndarray<RectDomain<2>, 1> Patches(RECTDOMAIN((0),
                                               (npatches*npatches)));
  int i_length = (interior_domain.max()[1] - interior_domain.min()[1])
    /npatches;
  int j_length = (interior_domain.max()[2] - interior_domain.min()[2])
    /npatches;
  Point<2> min, max;
  int max_ii, max_jj;
  for (int ii = 0; ii < npatches; ii++) {
    for (int jj = 0; jj < npatches; jj++) {
      min = POINT(interior_domain.min()[1] + ii*i_length,
                  interior_domain.min()[2] + jj*j_length);
      max_ii = ( ((ii + 1) < npatches) ? interior_domain.min()[1] + 
                 (ii + 1)*i_length
                 : interior_domain.max()[1] );
      max_jj = ( ((jj + 1) < npatches) ? interior_domain.min()[2] + 
                 (jj + 1)*j_length
                 : interior_domain.max()[2] );
      max = POINT(max_ii+1, max_jj+1);
      Patches[ii + jj*npatches] = RectDomain<2>(min, max);
    }
  }

  int my_NPatches = my_patch_count(npatches*npatches, THREADS);

  fine_interior = Patches;

  ndarray<RectDomain<2>, 1> my_patches = distribute(Patches);

  local_fine_rho =
    ndarray<ndarray<double, 2>, 1>(my_patches.domain());
  local_fine_phi =
    ndarray<ndarray<double, 2>, 1>(my_patches.domain());
  local_coarse_rho =
    ndarray<ndarray<double, 2>, 1>(my_patches.domain());
  local_coarse_phi =
    ndarray<ndarray<double, 2>, 1>(my_patches.domain());
  coarse_domain = ndarray<RectDomain<2>, 1>(my_patches.domain());

  Point<2> mid_low, mid_high;
  RectDomain<2> phi_region;

  pmg_test::log.end("1 of constructor");    
  pmg_test::log.begin("2 of constructor");

  /* We need to make sure that phi_region can be evenly coarsened 
     by 4 and by nref: lcm_ref should be the least common multiple of 
     nref and 4.  Instead, here it is the lowest multiple of four
     that is greater than or equal to nref. */
  int lcm_ref = ((nref - 1)/4 + 1)*4;
  foreach (i, my_patches.domain()) {
    local_fine_rho[i] = ndarray<double, 2>(my_patches[i]);
    phi_region = my_patches[i].accrete(border);
      
    phi_region =
      my_box::nrefine(my_box::ncoarsen(phi_region, lcm_ref), lcm_ref);
    /* Find corners of an intermediate RectDomain between
       phi_region and my_patches[i]. */
    mid_low = (phi_region.min() + my_patches[i].min())/POINT(2, 2);
    mid_high = (phi_region.max() + my_patches[i].max())/POINT(2, 2);

    /* Make sure intermediate RectDomain can be evenly coarsened by
       2. */
    mid_low = (mid_low/2)*2;
    mid_high = ((mid_high - POINT(1, 1))/2 + POINT(1, 1))*2;
    if (!(phi_region.shrink(1).contains(mid_low) 
          && phi_region.shrink(1).contains(mid_high))) {
      phi_region = phi_region.accrete(lcm_ref);
    }
    local_fine_phi[i] = ndarray<double, 2>(phi_region);
    coarse_domain[i] = my_box::ncoarsen(my_patches[i], nref);
    local_coarse_rho[i] =
      ndarray<double, 2>(my_box::ncoarsen(my_patches[i], nref)
                         .accrete(c));
    local_coarse_phi[i] =
      ndarray<double, 2>(my_box::ncoarsen(my_patches[i], nref)
                         .accrete(c+1));
  }
    
  RectDomain<1> Processors(POINT(0), POINT((int) THREADS));

  RectDomain<2> CoarseRhsRegion = 
    my_box::ncoarsen(fine_domain, nref).accrete(c);
  border = id_border*(CoarseRhsRegion.max()[1] 
                      - CoarseRhsRegion.min()[1])/100;
  RectDomain<2> CoarseSolnRegion = CoarseRhsRegion.accrete(border);
  CoarseSolnRegion =
    my_box::nrefine(my_box::ncoarsen(CoarseSolnRegion, 4), 4);
    
  mid_low =
    (CoarseSolnRegion.min() + CoarseRhsRegion.min())/POINT(2, 2);
  mid_high =
    (CoarseSolnRegion.max() + CoarseRhsRegion.max())/POINT(2, 2);
  mid_low = (mid_low/2)*2;
  mid_high = ((mid_high - POINT(1, 1))/2 + POINT(1, 1))*2;
  if (!(CoarseSolnRegion.shrink(1).contains(mid_low) &&
        CoarseSolnRegion.shrink(1).contains(mid_high))) {
    CoarseSolnRegion = CoarseSolnRegion.accrete(4);
  }

  coarse_domain_rhs =
    ndarray<ndarray<double, 2, global>, 1>(RECTDOMAIN((0),
                                                      ((int) THREADS)));
  coarse_domain_soln = ndarray<double, 2>(CoarseSolnRegion);
  coarse_domain_rhs.exchange(ndarray<double, 2,
                             global>(CoarseRhsRegion));

  int qq = 0;
  global_fine_phi =
    ndarray<ndarray<double, 2, global>,
            1>(RECTDOMAIN((0), (npatches*npatches)));
  for (int ii = 0; ii < THREADS; ii++) {
    for (int jj = 0;
         jj < broadcast(local_fine_phi.domain().size(), ii); jj++) {
      global_fine_phi[qq] = broadcast(local_fine_phi[jj], ii);
      qq++;
    }
  }

  pmg_test::log.end("2 of constructor");
  pmg_test::log.begin("3 of constructor");

  qq = 0;
  global_coarse_phi =
    ndarray<ndarray<double, 2, global>,
            1>(RECTDOMAIN((0), (npatches*npatches)));
  for (int ii = 0; ii < THREADS; ii++) {
    for (int jj = 0;
         jj < broadcast(local_coarse_phi.domain().size(), ii);
         jj++) {
      global_coarse_phi[qq] = broadcast(local_coarse_phi[jj], ii);
      qq++;
    }
  }
    
  final_solution = ndarray<nodal_multigrid, 1>(my_patches.domain());
  foreach (i, final_solution.domain()) {
    final_solution[i] = nodal_multigrid(my_patches[i], fine_h); // FIX!!!
  }

  qq = 0;
  // all_solutions = new nodal_multigrid [[0: npatches*npatches - 1]];
  // for (int ii = 0; ii < THREADS; ii++) {
  //   for (int jj = 0; jj < (broadcast final_solution.domain().size()
  //     			    from ii); jj++) {
  //     all_solutions[qq] = broadcast final_solution[jj] from ii;
  //     qq++;
  //   }
  // }
  all_solutions =
    ndarray<ndarray<double, 2, global>,
            1>(RECTDOMAIN((0), (npatches*npatches)));
  for (int ii = 0; ii < THREADS; ii++) {
    for (int jj = 0;
         jj < broadcast(final_solution.domain().size(), ii); jj++) {
      all_solutions[qq] = broadcast(final_solution[jj].phi, ii);
      qq++;
    }
  }

  ndarray<double, 1> center;
  double r0;
  int m;
  int star_shaped;
  foreach (p, charges.domain()) {
    center = charges[p].center();
    r0 = charges[p].r0();
    m = charges[p].m();
    star_shaped = charges[p].s();

    foreach (i, local_fine_rho.domain()) {
      if (star_shaped == 0) {
        my_math::makeRhs(local_fine_rho[i], fine_h, center, r0, m);
      }
      else {
        my_math::make_star_rhs(local_fine_rho[i], fine_h, center, r0, m);
      }
    }
  }
  int dd, ii;
  Point<2> mm;
  ndarray<double, 1> side;
  foreach (i, local_fine_rho.domain()) {
    /* We need to correct the edges so that the rhs doesn't get counted
       twice along boundaries. */
    for (dd = 1; dd <=2; dd++) {
      for (mm = local_fine_rho[i].domain().min(), ii = 0; ii < 2;
           mm = local_fine_rho[i].domain().max(), ii++) {
        side = local_fine_rho[i].slice(dd, mm[dd]);
        foreach (p, side.domain()) {
          side[p] /= 2.0;
        }
      }
    }
  }
  construct_neighboring_data_arrays();
  pmg_test::log.end("3 of constructor");
}

/** This is called by the constructor to set up the arrays to hold
    data from neighboring patches.  Apparently since the data is
    local, the the method that sets it has to be local. */
void pmg::construct_neighboring_data_arrays() {
  RectDomain<1> directions(POINT(1), POINT(3));
  RectDomain<1> min_max(POINT(0), POINT(2));

  all_neighboring_fine_data =
    ndarray<ndarray<ndarray<ndarray<ndarray<double, 2>, 1>, 1>, 1>,
            1>(local_fine_phi.domain());
  all_neighboring_coarse_data =
    ndarray<ndarray<ndarray<ndarray<ndarray<double, 2>, 1>, 1>, 1>,
            1>(local_coarse_phi.domain());

  //  all_neighboring_fine_data = new double [local_fine_phi.domain()]
  //    /*l*/ [directions] /*l*/ [min_max] /*l*/
  //    [global_fine_phi.domain()] /*l*/ [2d] /*l*/;
  //  all_neighboring_coarse_data = new double
  //    [local_coarse_phi.domain()] /*l*/ [directions] /*l*/ [min_max]
  //    /*l*/ [global_coarse_phi.domain()] /*l*/ [2d] /*l*/;

  foreach (i, all_neighboring_fine_data.domain()) {
    all_neighboring_fine_data[i] =
      ndarray<ndarray<ndarray<ndarray<double, 2>, 1>, 1>,
              1>(directions);
    all_neighboring_coarse_data[i] =
      ndarray<ndarray<ndarray<ndarray<double, 2>, 1>, 1>,
              1>(directions);
    foreach (dir, directions) {
      all_neighboring_fine_data[i][dir] =
        ndarray<ndarray<ndarray<double, 2>, 1>, 1>(min_max);
      all_neighboring_coarse_data[i][dir] =
        ndarray<ndarray<ndarray<double, 2>, 1>, 1>(min_max);
      foreach (mm, min_max) {
        all_neighboring_fine_data[i][dir][mm] =
          ndarray<ndarray<double, 2>, 1>(global_fine_phi.domain());
        all_neighboring_coarse_data[i][dir][mm] =
          ndarray<ndarray<double, 2>, 1>(global_coarse_phi.domain());
        foreach (ii, global_fine_phi.domain()) {
          all_neighboring_fine_data[i][dir][mm][ii] =
            ndarray<double, 2>(final_solution[i].phi.domain()
                               .border(1, dir[1]*(mm[1]*2 - 1), 0)
                               *global_fine_phi[ii].domain());
          //	    println("Fine border: " << final_solution[i].phi.domain()
          //	      .border(1, dir[1]*(mm[1]*2 - 1), 0)
          //	      *global_fine_phi[ii].domain());
          all_neighboring_coarse_data[i][dir][mm][ii] =
            ndarray<double, 2>(coarse_domain[i].border(1, dir[1]*(mm[1]*2 - 1), 0)
                               .accrete(1)*global_coarse_phi[ii].domain());
          //	    println("Coarse border: " << coarse_domain[i].border(1, dir[1]*(mm[1]*2 - 1), 0)
          //	      .accrete(1)*global_coarse_phi[ii].domain());
        }
      }
    }
  }
}

/** solve() does all the high-level work to calculate the
    solution. */
void pmg::solve() {
  string phase;
  ostringstream oss;
  /* Do the infinite domain solve on each patch. */
  foreach (i, local_fine_rho.domain()) {
    oss.str("");
    oss << "Fine grid solve " << local_fine_rho[i].domain();
    phase = oss.str();
    pmg_test::log.begin(phase);
    switch (s) {
    case 95:
      nodal_infdmn::solve95(local_fine_rho[i], local_fine_phi[i],
			    fine_h);
      break;
    case 59:
      nodal_infdmn::solve59(local_fine_rho[i], local_fine_phi[i],
			    fine_h);
      break;
    case 5:
      nodal_infdmn::solve5(local_fine_rho[i], local_fine_phi[i],
			   fine_h);
      break;
    case 9:
    default:
      nodal_infdmn::solve(local_fine_rho[i], local_fine_phi[i],
			  fine_h, report_level);
      break;
    }
    pmg_test::log.end(phase);
    phase = "Other fine grid stuff";
    pmg_test::log.begin(phase);

    /* Sample and take the laplacian to get a coarse Rhs. */
    my_math::sample(local_fine_phi[i], local_coarse_phi[i], nref);
    my_math::nlaplacian(local_coarse_phi[i], local_coarse_rho[i],
                        coarse_h);

    ndarray<double, 2> my_coarse_domain_rhs =
      (ndarray<double, 2>) coarse_domain_rhs[MYTHREAD];
    /* Add data from each patch to a global array. */
    foreach (p, local_coarse_rho[i].domain()
             *coarse_domain_rhs[MYTHREAD].domain()) {
      my_coarse_domain_rhs[p] += local_coarse_rho[i][p];
    }

    pmg_test::log.end(phase);
  }

  pmg_test::barrier();

  communication_time.start();
  /* My ugly and slightly less inefficient reduction */
  phase = "Reduction";
  pmg_test::log.begin(phase);

  if (MYTHREAD == 0) {
    ndarray<double, 2> tmp_array;
    foreach (i, coarse_domain_rhs.domain()) {
      if (i != POINT(0)) {
        /* Improvement from previous version: we make a local copy
           of the array, then add.  It would be better to do some
           sort of binary tree summation/reduction, but I haven't
           bothered to implement that yet.  This is fast enough now
           that further optimization probably wouldn't be noticed. */
        tmp_array =
          ndarray<double, 2>(coarse_domain_rhs[i].domain());
        tmp_array.copy(coarse_domain_rhs[i]);
        foreach (p, coarse_domain_rhs[0].domain()) {
          coarse_domain_rhs[0][p] += tmp_array[p];
        }
      }
    }
  }

  pmg_test::barrier();

  coarse_domain_rhs[MYTHREAD].copy(coarse_domain_rhs[0]);
    
  pmg_test::log.end(phase);
  /* That's the end of my very ugly and inefficient reduction. */

  pmg_test::barrier();
  communication_time.stop();

  /* The coarse grid solve. */
  phase = "Coarse solve";
  pmg_test::log.begin(phase);

  nodal_infdmn::solve((ndarray<double, 2>) coarse_domain_rhs[MYTHREAD],
                      coarse_domain_soln, coarse_h, report_level);

  pmg_test::log.end(phase);

  pmg_test::barrier();
  communication_time.start();

  /* Now we just need to set the boundary... */
  phase = "Set boundary";
  pmg_test::log.begin(phase);

  /* I want to do all the communication first: I don't really think
     it will make a big difference, but it's worth a try. */
  //      copy_neighbor_data();

  //      pmg_test::barrier();

  foreach (i, final_solution.domain()) {
    set_local_fine_boundary(i);
  }

  pmg_test::log.end(phase);

  pmg_test::barrier();
  communication_time.stop();

  /* And do the final multigrid solves... */
  foreach (i, final_solution.domain()) {
    oss.str("");
    oss << "Last fine grid solve " << final_solution[i].rhs.domain();
    phase = oss.str();
    pmg_test::log.begin(phase);
    my_math::nlaplacian5(final_solution[i].phi,
                         final_solution[i].rhs, fine_h);
    int ii;
    ndarray<double, 2> phiBC(final_solution[i].phi.domain());
    ndarray<double, 1, global> fside;
    ndarray<double, 1, global> pside;
    int d;
    Point<2> mm;
    for (d = 1; d <= 2; d++) {
      for (mm = phiBC.domain().min(), ii = 0; ii < 2;
           mm = phiBC.domain().max(), ii++) {
        fside = final_solution[i].phi.slice(d, mm[d]);
        pside = phiBC.slice(d, mm[d]);
        // foreach (p in fside.domain()) {
        //   pside[p] = fside[p];
        // }
        pside.copy(fside);
      }
    }
    final_solution[i].phi.set(0.);
    foreach (p, final_solution[i].rhs.domain()) {
      final_solution[i].rhs[p] = - final_solution[i].rhs[p];
    }
    foreach (p, final_solution[i].rhs.domain()
             *local_fine_rho[i].domain()) {
      final_solution[i].rhs[p] = final_solution[i].rhs[p] 
        + local_fine_rho[i][p];
    }

    double error0, error;

    error0 = final_solution[i].errorest5();
    error = error0;

    for (ii = 0; (ii < 50) & (error > 1.e-10*error0); ii++) {
      final_solution[i].wrelax5();
      error = final_solution[i].errorest5();
    }

    if (report_level > 0) {
      println("iterations for final solve: " << ii);
    }

    /* Reset boundary, which was zeroed out */
    for (d = 1; d <= 2; d++) {
      for (mm = phiBC.domain().min(), ii = 0; ii < 2;
           mm = phiBC.domain().max(), ii++) {
        fside = final_solution[i].phi.slice(d, mm[d]);
        pside = phiBC.slice(d, mm[d]);
        // foreach (p, fside.domain()) {
        //   fside[p] = pside[p];
        // }
        fside.copy(pside);
      }
    }

    pmg_test::log.end(phase);
  }

  pmg_test::barrier();
}

/** Distribute an array of patches cyclically to the processors. */
ndarray<RectDomain<2>, 1> pmg::distribute(ndarray<RectDomain<2>,
                                          1> patches) {
  int mpc = my_patch_count(patches.domain().max()[1] 
                           - patches.domain().min()[1] + 1, 
                           THREADS);

  ndarray<RectDomain<2>, 1> dist(RECTDOMAIN((0), (mpc)));
  for (int ii = 0; ii < mpc; ii++) {
    dist[ii] = patches[ii*THREADS + MYTHREAD];
  }
  return dist;
}

/** Copy all data from neighbors necessary to set the final fine
    grid boundary condition. */
void pmg::copy_neighbor_data() {
  RectDomain<1> directions(POINT(1), POINT(3));
  RectDomain<1> min_max(POINT(0), POINT(2));
  foreach (i, all_neighboring_fine_data.domain()) {
    foreach (dir, directions) {
      foreach (mm, min_max) {
        foreach (ii, global_fine_phi.domain()) {
          all_neighboring_fine_data[i][dir][mm][ii]
            .copy(global_fine_phi[ii]);
          all_neighboring_coarse_data[i][dir][mm][ii]
            .copy(global_coarse_phi[ii]);
        }
      }
    }
  }
}
      
/** Set the fine boundary for a patch i, where i is the index of
    _all_ patches, not just patches local to a processor. */
void pmg::set_local_fine_boundary(Point<1> i) {
  int d;
  int mm, jj;
  double phixxxx, phiyyyy, phixxx, phiyyy, phixx, phiyy, phix, phiy;
  double h = coarse_h;
  double aich;
  ndarray<double, 2> stencil;
  ndarray<double, 2> fine_stencil;
  RectDomain<2> sidecounter;
  Point<2> dMin = coarse_domain[i].min();
  Point<2> dMax = coarse_domain[i].max();
  ndarray<ndarray<double, 2>, 1> neighbor_coarse_phi;
  ndarray<ndarray<double, 2>, 1> neighbor_fine_phi;
  for ( d = 1; d <= 2; d++) {
    for (mm = -1; mm <= 1; mm += 2) {
      sidecounter = coarse_domain[i].border(1, d*mm, 0);

      //  neighbor_coarse_phi = (double [1d] /*l*/ [2d] /*l*/)
      //    all_neighboring_coarse_data[i][d][(mm+1)/2]; 
      //  neighbor_fine_phi = (double [1d] /*l*/ [2d] /*l*/)
      //    all_neighboring_fine_data[i][d][(mm+1)/2];
      /* set up neighbor_coarse_phi to be an array (of the same size
         as the global_coarse_phi domain) of arrays (of size
         sidecounter, grown by 1, intersected with the appropriate
         domain of global_coarse_phi) to hold coarse data from
         neighboring patches locally */
      neighbor_coarse_phi =
        ndarray<ndarray<double, 2>, 1>(global_coarse_phi.domain());
      foreach (ii, neighbor_coarse_phi.domain()) {
        /* instantiate the correct size array */
        neighbor_coarse_phi[ii] = 
          ndarray<double, 2>(sidecounter.accrete(1)*
                             global_coarse_phi[ii].domain());
        //  println(" " << neighbor_coarse_phi[ii].domain());
        /* fill with copied data */
        neighbor_coarse_phi[ii].copy(global_coarse_phi[ii]);
      }

      /* set up neighbor_fine_phi to be an array (of the same size
         as the global_fine_phi domain) of arrays (large enough to hold
         an edge of this fine patch intersected with the appropriate
         domain of global_fine_phi) to hold fine data from neighboring
         patches locally */
      neighbor_fine_phi =
        ndarray<ndarray<double, 2>, 1>(global_fine_phi.domain());
      foreach (ii, neighbor_fine_phi.domain()) {
        /* instantiate the correct size array */
        neighbor_fine_phi[ii] = 
          ndarray<double, 2>(final_solution[i].phi.domain().border(1, mm*d, 0) 
                             *global_fine_phi[ii].domain());
        //  println(" " << final_solution[i].phi.domain().border(1, mm*d, 0));
        //  println(" " << global_fine_phi[ii].domain());
        /* fill with copied data */
        //  plot::dump(global_fine_phi[ii], "global");
        neighbor_fine_phi[ii].copy(global_fine_phi[ii]);
        //  plot::dump(neighbor_fine_phi[ii], "neighbor");
      }

      foreach (p, sidecounter) {
        RectDomain<2> fsd(p*nref - POINT((d-1)*(nref/2), (2-d)*(nref/2)),
                          p*nref + POINT((d-1)*(nref/2)+1, (2-d)*(nref/2)+1));
        fine_stencil = ndarray<double, 2>(fsd);
        stencil = ndarray<double, 2>(RectDomain<2>(p - POINT(1, 1),
                                                   p + POINT(2, 2)));
        stencil.copy(coarse_domain_soln);

        /* Subtract off local parts from coarse data... */
        foreach (ii, neighbor_coarse_phi.domain()) {
          if (neighbor_coarse_phi[ii].domain().shrink(1).contains(p)) {
            foreach (pp, stencil.domain()) {
              stencil[pp] -= neighbor_coarse_phi[ii][pp];
            }
          }
        }
	  
        /* Interpolate */
        /* We need a bunch of derivatives... 
           We don't need all of them, but let's not worry about
           that right now. */
        if (d == 1) {
          phiyyyy = - ( stencil[p + POINT(1, 1)] + stencil[p + POINT(1, -1)]
                        + stencil[p + POINT(-1, 1)] + stencil[p + POINT(-1, -1)]
                        - 2.0*(stencil[p + POINT(1, 0)] + stencil[p + POINT(-1, 0)]
                               + stencil[p + POINT(0, 1)] + stencil[p + POINT(0, -1)])
                        + 4.0*stencil[p] )/(h*h*h*h);
          phiyyy = ( 2.0*(stencil[p + POINT(0, 1)] - stencil[p + POINT(0, -1)])
                     + stencil[p + POINT(1, -1)] + stencil[p + POINT(-1, -1)]
                     - stencil[p + POINT(1, 1)] - stencil[p + POINT(-1, 1)] )
            /(2.0*h*h*h);
          phiyy = ( stencil[p + POINT(0, 1)] - 2.0*stencil[p]
                    + stencil[p + POINT(0, -1)] )/(h*h) - h*h*phiyyyy/12.0;
          phiy = ( 4.0*(stencil[p + POINT(0, 1)] - stencil[p + POINT(0, -1)])
                   + stencil[p + POINT(1, 1)] + stencil[p + POINT(-1, 1)]
                   - stencil[p + POINT(1, -1)] - stencil[p + POINT(-1, -1)] )/(12.0*h);
          for (jj = -(nref/2); jj <= nref/2; jj++) {
            aich = jj*fine_h;
            fine_stencil[p*nref + POINT(0, jj)] = stencil[p] 
              + (aich*phiy + aich*aich*phiyy/2.0 
                 + aich*aich*aich*phiyyy/6.0
                 + aich*aich*aich*aich*phiyyyy/24.0);
          }
        }
        else {
          phixxxx = - ( stencil[p + POINT(1, 1)] + stencil[p + POINT(1, -1)]
                        + stencil[p + POINT(-1, 1)] + stencil[p + POINT(-1, -1)]
                        - 2.0*(stencil[p + POINT(1, 0)] + stencil[p + POINT(-1, 0)]
                               + stencil[p + POINT(0, 1)] + stencil[p + POINT(0, -1)])
                        + 4.0*stencil[p] )/(h*h*h*h);
          phixxx = ( 2.0*(stencil[p + POINT(1, 0)] - stencil[p + POINT(-1, 0)])
                     + stencil[p + POINT(-1, 1)] + stencil[p + POINT(-1, -1)]
                     - stencil[p + POINT(1, 1)] - stencil[p + POINT(1, -1)] )
            /(2.0*h*h*h);
          phixx = ( stencil[p + POINT(1, 0)] - 2.0*stencil[p]
                    + stencil[p + POINT(-1, 0)] )/(h*h) - h*h*phixxxx/12.0;
          phix = ( 4.0*(stencil[p + POINT(1, 0)] - stencil[p + POINT(-1, 0)])
                   + stencil[p + POINT(1, 1)] + stencil[p + POINT(1, -1)]
                   - stencil[p + POINT(-1, 1)] - stencil[p + POINT(-1, -1)] )/(12.0*h);

          for (jj = -(nref/2); jj <= nref/2; jj++) {
            aich = jj*fine_h;
            fine_stencil[p*nref + POINT(jj, 0)] = stencil[p] 
              + (aich*phix + aich*aich*phixx/2.0 
                 + aich*aich*aich*phixxx/6.0
                 + aich*aich*aich*aich*phixxxx/24.0);
          }
        }

        /* Add back local fine data... */

        //  foreach (ii in neighbor_coarse_phi.domain()) {
        //    print(" " << neighbor_fine_phi[ii].domain() << ", "
        //          << fine_stencil.domain() << "\n");
        //    foreach (pp in fine_stencil.domain()*neighbor_fine_phi[ii].domain()) {
        //      print(neighbor_fine_phi[ii][pp] << ", ");
        //      print(fine_stencil[pp] << "; ");
        //    }
        //    println("");
        //  }

        foreach (ii, neighbor_coarse_phi.domain()) {
          if (neighbor_coarse_phi[ii].domain().shrink(1).contains(p)) {
            foreach (pp, fine_stencil.domain()*neighbor_fine_phi[ii].domain()) {
              fine_stencil[pp] += neighbor_fine_phi[ii][pp];
            }
          }
        }

        /* Correct (average) endpoints of fine_stencil */
        for (jj = -(nref/2); jj <= nref/2; jj += nref) {
          if ((jj*2 == nref) || (jj*2 == -nref)) {
            fine_stencil[p*nref + POINT((d-1)*jj, (2-d)*jj)] /= 2.0;
          }
        }

        /* Correct for double counting in boundary corners */
        if ((p == dMin) || (p == POINT(dMin[1], dMax[2]))
            || (p == POINT(dMax[1], dMin[2])) || (p == dMax)) {
          fine_stencil[p*nref] /= 2.0;
        }

        /* Add result to fine boundary... */
        foreach (pp, fine_stencil.domain()*final_solution[i].phi.domain()) {
          final_solution[i].phi[pp] += fine_stencil[pp];
        }
      }
    }
  }
}

/** Build the solution from the data on the several processors,
    calculate the error, and write some data to files. */
void pmg::wrap_up() {
  double max_comm_time = reduce::max(communication_time.secs());
  if (MYTHREAD == 0) {
    println("Time for communication: " << max_comm_time);
  }
  if (report_level > 0) {
    compose_solution();
  }
  pmg_test::barrier();
  bool have_exact = calculate_exact();
  double sum1, sum2, max;
  if (have_exact && MYTHREAD == 0) {
    println("made exact");
  }
  pmg_test::barrier();
  //  plot::dump(full_solution, "soln");
  if (have_exact) {
    //  plot::dump(exact_solution, "exact");
    if (MYTHREAD == 0) {
      println("calculating error");
    }
    ndarray<ndarray<double, 2, global>, 1> errTemp = get_error();
    foreach (i, errTemp.domain()) {
      sum1 += my_math::sumabs(errTemp[i]);
      sum2 += my_math::sumsquared(errTemp[i]);
      max = std::max(max, my_math::normMax(errTemp[i]));
    }
    pmg_test::barrier();
    // println("Processor " << MYTHREAD 
    //		 << " reporting an L2 error of " << sum2);
    // pmg_test::barrier();
    sum1 = reduce::add(sum1);
    sum2 = reduce::add(sum2);
    max = reduce::max(max);
    pmg_test::barrier();
    if (MYTHREAD == 0) {
      println("L2 error: " << fine_h*sqrt(sum2));
      println("L1 error: " << fine_h*fine_h*sum1);
      println("Max error: " << max);
    }

    if (report_level > 0) {
      plot::dump(compose_error(errTemp), "error");
    }
    /* The laplacian stuff is dependent on the report_level out
       because I haven't figured out (bothered) to parallelize it
       yet.  (It's also commented out because it's broken.)  The
       difficulty is that to calculate this laplacian error
       globally, we need to extend the local error patches by 1, or
       somehow communicate that information so that we can calculate
       the laplacian along the boundaries. */
    //  if (report_level > 1) {
    //    ndarray<ndarray<double, 2>, 1> lTemp(errTemp.domain().shrink(1));
    //    my_math::nlaplacian5(errTemp, lTemp, fine_h);
    //    println("Max laplacian error: " << my_math::normMax(lTemp));
    //    plot::dump(lTemp, "lapl");
    //  }
  }
  else {
    /* I still need to parellize this. */
    if (report_level > 0) {
      ndarray<double, 2> coarse_sample(my_box::ncoarsen(fine_domain, 2));
      my_math::sample(full_solution, coarse_sample, 2);
      plot::dump(coarse_sample, "coarse_sample");
    }
  }
}

/** Determine if an exact solution can be found for our problem, and
    if so, calculate it.*/
bool pmg::calculate_exact() {
  bool can_calculate_exact = false;
  bool i_think_i_can = true;
  if (MYTHREAD == 0) {
    foreach (p, charges.domain()) {
      if (charges[p].s() != 0) {
        i_think_i_can = false;
      }
    }
  }
  can_calculate_exact = broadcast(i_think_i_can, 0);
  if (can_calculate_exact) {
    ndarray<double, 1> center;
    double r0;
    int m;

    exact_solution =
      ndarray<ndarray<double, 2>, 1>(local_fine_rho.domain());
    foreach (i, exact_solution.domain()) {
      exact_solution[i] =
        ndarray<double, 2>(local_fine_rho[i].domain());
      exact_solution[i].set(0.);

      foreach (p, charges.domain()) {
        center = charges[p].center();
        r0 = charges[p].r0();
        m = charges[p].m();

        my_math::exactSoln(exact_solution[i], fine_h, center,
                           r0, m);
      }

    }
  }
  return can_calculate_exact;
}
