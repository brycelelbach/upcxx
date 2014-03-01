#include <cmath>
#include "amr_level.h"
#include "amr_test.h"

string amr_level::bb = "begin barrier";
string amr_level::eb = "end barrier";
string amr_level::bfg = "begin fill_ghost";
string amr_level::efg = "end fill_ghost";
string amr_level::bm = "begin math";
string amr_level::em = "end math";

amr_level::amr_level(ndarray<RectDomain<2>, 1> grids, double dx) {
  interior = grids;
  h = dx;
  phi = ndarray<ndarray<double, 2, global>, 1>(grids.domain());
  rho = ndarray<ndarray<double, 2, global>, 1>(grids.domain());
  res = ndarray<ndarray<double, 2, global>, 1>(grids.domain());
  neighbor = ndarray<ndarray<Point<1>, 1>, 1>(grids.domain());

  ndarray<double, 2, global> pa, rha, rea;
  foreach (i, phi.domain()) {
    if (MYTHREAD == i[1]%THREADS) {
      pa = ndarray<double, 2, global>(grids[i].accrete(1));
      rha = ndarray<double, 2, global>(grids[i]);
      rea = ndarray<double, 2, global>(grids[i]);
    }
    phi[i] = broadcast(pa, i[1]%THREADS);
    rho[i] = broadcast(rha, i[1]%THREADS);
    res[i] = broadcast(rea, i[1]%THREADS);
  }
}

amr_level::amr_level(ndarray<ndarray<double, 2, global>, 1> input,
                     ndarray<ndarray<double, 2, global>, 1> soln, 
                     ndarray<int, 1> assignments, double dx) {

  /* Set up interior regions. */
  interior = ndarray<RectDomain<2>, 1>(input.domain());
  owner = assignments;
  h = dx;

  //      math = new Timer();
  //      communication = new Timer();
  //      barrier_wait = new Timer();
  //      ghost = new Timer();
  //      relaxation = new Timer();

  /* Set up references to the input data and desired solution grids. */
  rho = input;
  solution = soln;

  /* Allocate and size the necessary temporary storage. */
  phi = ndarray<ndarray<double, 2, global>, 1>(interior.domain());
  res = ndarray<ndarray<double, 2, global>, 1>(interior.domain());
  neighbor = ndarray<ndarray<Point<1>, 1>, 1>(interior.domain());

  ndarray<double, 2, global> pa, rea;
  foreach (i, interior.domain()) {
    interior[i] = input[i].domain();
    if (MYTHREAD == owner[i]) {
      pa = ndarray<double, 2, global>(interior[i].accrete(1));
      rea = ndarray<double, 2, global>(interior[i]);
    }
    phi[i] = broadcast(pa, owner[i]);
    res[i] = broadcast(rea, owner[i]);
  }

  form_neighbor_list();
}

void amr_level::form_neighbor_list() {
  /* Loop over all grids. */
  foreach (i, interior.domain()) {

    /* Only do checks if grid is local. */
    if (owner[i] == MYTHREAD) {

      /* Loop over all grids to check overlaps and list grids. */
      int neighbor_list_count = 0;
      boxedlist< Point<1> > neighbor_list;
      foreach (j, interior.domain()) {
        if (!(phi[i].domain()*interior[j]).is_empty()) {
          if (i != j) {
            neighbor_list_count += 1;
            neighbor_list.push(j);
          }
        }
      }
      neighbor[i] = neighbor_list.toArray();
      foreach (j, neighbor[i].domain()) {
        //	  println("It seems that " << i << " thinks that " << neighbor[i][j] << " is his neighbor.");
      }
    }
  }
}

void amr_level::fill_ghost() {
  ndarray<double, 1, global> inner_slice;
  ndarray<double, 1, global> outer_slice;
  int d, sign, mm;
  /* First we will fill the ghost cells with a value appropriate for 
     physical boundary conditions. */
  /* Loop over all grids. */
  foreach (i, phi.domain()) {
    if (phi[i].creator() == MYTHREAD) {
      /* Loop over each of x, y (later z). */
      for (d = 1; d <= phi[i].arity(); d++) {
        /* Loop over low and then high end. */
        for (sign = -1, mm = phi[i].domain().min()[d]; 
             sign <= 1; sign += 2, mm = phi[i].domain().max()[d]) {
          /* Take a slice on the boundary and fill it with the
             reflection of the interior charge. */
          inner_slice = phi[i].slice(d, mm - sign);
          outer_slice = phi[i].slice(d, mm);
          foreach (p, outer_slice.domain()) {
            outer_slice[p] = -inner_slice[p];
          }
        }
      }
    }
  }

  /* Barrier just to be sure everyone is ready to communicate. */
  amr_test::barrier();

  /* Now fill data from neighboring grids. */
  foreach (i, phi.domain()) {
    if (phi[i].creator() == MYTHREAD) {
      foreach (j, neighbor[i].domain()) {
        phi[i].copy(phi[neighbor[i][j]].constrict(interior[neighbor[i][j]]));
      }
    }
  }
}

void amr_level::smooth_level() {
  //amr_test.log.append(2, bfg);
  fill_ghost();
  //amr_test.log.append(2, efg);

  amr_test::barrier();

  //amr_test.log.append(2, bm);
  foreach (i, phi.domain()) {
    if (phi[i].creator() == MYTHREAD) {
      point_relax(POINT(0, 0), (ndarray<double, 2>) phi[i], 
                  (ndarray<double, 2>) rho[i], h*h);
      point_relax(POINT(1, 1), (ndarray<double, 2>) phi[i], 
                  (ndarray<double, 2>) rho[i], h*h);
    } 
  }
  //amr_test.log.append(2, em);

  amr_test::barrier();

  //amr_test.log.append(2, bfg);
  fill_ghost();
  //amr_test.log.append(2, efg);

  amr_test::barrier();

  //amr_test.log.append(2, bm);
  foreach (i, phi.domain()) {
    if (phi[i].creator() == MYTHREAD) {
      point_relax(POINT(1, 0), (ndarray<double, 2>) phi[i], 
                  (ndarray<double, 2>) rho[i], h*h);
      point_relax(POINT(0, 1), (ndarray<double, 2>) phi[i], 
                  (ndarray<double, 2>) rho[i], h*h);
    }
  }
  //amr_test.log.append(2, em);

  amr_test::barrier();
}

void amr_level::point_relax(Point<2> offset, ndarray<double, 2> phi,
                            ndarray<double, 2> rhs, double hsquared) {
  foreach(p, (rhs.domain().shrink(1, -1).shrink(1, -2)
              - offset)/POINT(2, 2)) {
    phi[p*2 + offset] = (phi[p*2 + offset + POINT(1, 0)]
                         + phi[p*2 + offset + POINT(-1, 0)]
                         + phi[p*2 + offset + POINT(0, 1)]
                         + phi[p*2 + offset + POINT(0, -1)]
                         - rhs[p*2 + offset]*hsquared)*0.25;
  }
}

double amr_level::errorest_level() {
  fill_ghost();

  barrier();

  foreach (i, res.domain()) {
    if (owner[i] == MYTHREAD) {
      resid((ndarray<double, 2>) res[i], (ndarray<double, 2>) phi[i], 
            (ndarray<double, 2>) rho[i], h*h);
    }
  }
  barrier();
  return h*level_norm2(res);
}

double amr_level::level_norm2(ndarray<ndarray<double, 2, global>, 1> a) {
  double sum = 0;
  foreach (i, a.domain()) {
    if (a[i].creator() == MYTHREAD) {
      sum += sumsquared((ndarray<double, 2>) a[i]);
    }
  }

  barrier();

  return sqrt(reduce::add(sum));
}
