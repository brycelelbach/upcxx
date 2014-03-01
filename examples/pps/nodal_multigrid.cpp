#include "nodal_multigrid.h"
#include "my_box.h"

void nodal_multigrid::setDomain(RectDomain<2> box, double H) {
  h = H;
  interior = box;
  //rbDomain = ndarray<RectDomain<2>, 2>(RECTDOMAIN((0, 0), (2, 2)));
  phi = ndarray<double, 2>(box);
  res = ndarray<double, 2>(box);
  rhs = ndarray<double, 2>(box);
  bcoarse = my_box::ncoarsen(box, 2);
  ibn = box.slice(1);
  ibs = box.slice(1);
  ibe = box.slice(2);
  ibw = box.slice(2);
  nside = phi.slice(1, box.max()[1]);
  sside = phi.slice(1, box.min()[1]);
  eside = phi.slice(2, box.max()[2]);
  wside = phi.slice(2, box.min()[2]);

  /*
    foreach(p, rbDomain.domain()) {
    rbDomain[p] = my_box::ngsrbbox(box.shrink(1), p);
    }
  */
  /*faster 1*/
  rbdSW = my_box::ngsrbbox(box.shrink(1), POINT(0, 0));
  rbdNW = my_box::ngsrbbox(box.shrink(1), POINT(0, 1));
  rbdSE = my_box::ngsrbbox(box.shrink(1), POINT(1, 0));
  rbdNE = my_box::ngsrbbox(box.shrink(1), POINT(1, 1));

  bool coarsen = false;

  /* Check whether we should coarsen more. */
  if ( (box.max()[1] - box.min()[1]) > 2
       || (box.max()[2] - box.min()[2]) > 2 ) {
    coarsen = true;
  }

  if (coarsen) {
    next = new nodal_multigrid(bcoarse, 2.0*h);
    /* The domain of restmp is simply bcoarse refined by a factor
       of 2, in a node-centered way. */
    restmp = ndarray<double, 2>(my_box::nrefine(bcoarse, 2));
  }
  else {
    next = NULL;
    //restmp = NULL;
  }
}

/** A v-cycle relaxation. */
void nodal_multigrid::relax() {
  gsrb();
  gsrb();
  if (next != NULL) {
    next->phi.set(0.);
    resid();
    next->rhs.set(0.);
    restmp.set(0.);
    restmp.copy(res);
    average(restmp, next->rhs);

    next->relax();

    restmp.set(0.);
    interp(next->phi, restmp);
    foreach(t, phi.domain()*restmp.domain()) {
      phi[t] = phi[t] + restmp[t];
    }
  }
  gsrb();
  gsrb();
}

/** A w-cycle relaxation. */
void nodal_multigrid::wrelax() {
  gsfc();
  gsfc();
  if (next != NULL) {
    next->phi.set(0.);
    resid();
    next->rhs.set(0.);
    restmp.set(0.);
    restmp.copy(res);
    average(restmp, next->rhs);

    next->wrelax();
    next->wrelax();

    restmp.set(0.);
    interp(next->phi, restmp);
    foreach(t, phi.domain().shrink(1)*restmp.domain()) {
      phi[t] = phi[t] + restmp[t];
    }
  }
  gsfc();
  gsfc();
}

void nodal_multigrid::wrelax5() {
  gsrb5();
  gsrb5();
  if (next != NULL) {
    next->phi.set(0.);
    resid5();
    next->rhs.set(0.);
    restmp.set(0.);
    restmp.copy(res);
    average(restmp, next->rhs);

    next->wrelax5();
    next->wrelax5();

    restmp.set(0.);
    interp(next->phi, restmp);
    foreach(t, phi.domain().shrink(1)*restmp.domain()) {
      phi[t] = phi[t] + restmp[t];
    }
  }
  gsrb5();
  gsrb5();
}

void nodal_multigrid::pointRelax(Point<2> offset,
                                 /*faster 1*/ RectDomain<2> d,
                                 double hsquared) {
  /*take out for faster 2*/
  /*
    ndarray<double, 2> thisphi = phi.translate(offset);
    ndarray<double, 2> thisrhs = rhs.translate(offset);
    ndarray<double, 2> thisres = res.translate(offset);
  */

  foreach (p, /*rbDomain[offset]*/ /*faster 1*/ d) {
    /*replaced by faster 2*/
    /*
      thisres[p*2] = 0.05*(thisphi[p*2 + POINT(1, 1)]
      + thisphi[p*2 + POINT(1, -1)]
      + thisphi[p*2 + POINT(-1, 1)]
      + thisphi[p*2 + POINT(-1, -1)]
      + 4.0*(thisphi[p*2 + POINT(0, 1)]
      + thisphi[p*2 + POINT(1, 0)]
      + thisphi[p*2 + POINT(0, -1)]
      + thisphi[p*2 + POINT(-1, 0)])
      - 6.0*hsquared*thisrhs[p*2]);
    */
    /*faster 2*/
    res[p*2 - offset] = (phi[p*2 - offset + POINT(1, 1)]
                         + phi[p*2 - offset + POINT(1, -1)]
                         + phi[p*2 - offset + POINT(-1, 1)]
                         + phi[p*2 - offset + POINT(-1, -1)]
                         + 4.0*(phi[p*2 - offset + POINT(0, 1)]
				+ phi[p*2 - offset + POINT(0, -1)]
				+ phi[p*2 - offset + POINT(1, 0)]
				+ phi[p*2 - offset + POINT(-1, 0)])
			 - 6.0*rhs[p*2 - offset]*hsquared)*0.05;
  }
}				   

void nodal_multigrid::point_relax_fc(Point<2> offset, RectDomain<2> d,
                                     double hsquared) {
  foreach (p, d) {
    phi[p*2 - offset] = (phi[p*2 - offset + POINT(1, 1)]
                         + phi[p*2 - offset + POINT(1, -1)]
                         + phi[p*2 - offset + POINT(-1, 1)]
                         + phi[p*2 - offset + POINT(-1, -1)]
                         + 4.0*(phi[p*2 - offset + POINT(0, 1)]
				+ phi[p*2 - offset + POINT(0, -1)]
				+ phi[p*2 - offset + POINT(1, 0)]
				+ phi[p*2 - offset + POINT(-1, 0)])
			 - 6.0*rhs[p*2 - offset]*hsquared)*0.05;
  }
}				   

void nodal_multigrid::pointRelax5(Point<2> offset,
                                  /*faster 1*/ RectDomain<2> d,
                                  double hsquared) {

  foreach (p, d) {
    phi[p*2 - offset] = 0.25*(phi[p*2 - offset + POINT(0, 1)]
                              + phi[p*2 - offset + POINT(1, 0)]
                              + phi[p*2 - offset + POINT(0, -1)]
                              + phi[p*2 - offset + POINT(-1, 0)]
                              - hsquared*rhs[p*2 - offset]);
  }
}				   

void nodal_multigrid::pointCopy(Point<2> offset /*faster 1*/,
                                RectDomain<2> d) {
  /*take out for faster 2*/
  /*
    ndarray<double, 2> thisphi = phi.translate(offset);
    ndarray<double, 2> thisres = res.translate(offset);
  */

  foreach (p, /*rbDomain[offset]*/ /*faster 1*/ d) {
    /*
      thisphi[p*2] = thisres[p*2];
    */
    /*faster 2*/
    phi[p*2 - offset] = res[p*2 - offset];
  }
}

void nodal_multigrid::setBC() {
  /*
    ndarray<double, 1> side;
    Point<2> mm;
    int ii, sign;
    for (mm = interior.min(), ii= 0; ii < 2; 
    mm = interior.max(), ii++) {
    for (int d = 1; d <= 2; d++) {
    side = phi.slice(d, mm[d]);
    foreach (p, side.domain()) {
    side[p] = 0.;
    }
    }
    }
  */
  /*faster 3*/
  foreach (p, ibn) {
    nside[p] = 0.0;
  }
  foreach (p, ibs) {
    sside[p] = 0.0;
  }
  foreach (p, ibe) {
    eside[p] = 0.0;
  }
  foreach (p, ibw) {
    wside[p] = 0.0;
  }
}

void nodal_multigrid::average(ndarray<double, 2> fromArray,
                              ndarray<double, 2> toArray) {
  foreach (p, toArray.domain()
           *((fromArray.domain().accrete(1, 1).accrete(1, 2)/
              POINT(2, 2)).shrink(1))) {
    toArray[p] = 0.0625*(4.0*fromArray[p*2] 
                         + 2.0*(fromArray[p*2 + POINT(1, 0)] 
                                + fromArray[p*2 + POINT(0, 1)] 
                                + fromArray[p*2 + POINT(-1, 0)]
                                + fromArray[p*2 + POINT(0, -1)])
                         + fromArray[p*2 + POINT(1, 1)]
                         + fromArray[p*2 + POINT(1, -1)]
                         + fromArray[p*2 + POINT(-1, 1)]
                         + fromArray[p*2 + POINT(-1, -1)]);
  }
}

void nodal_multigrid::interp(ndarray<double, 2> fromArray,
                             ndarray<double, 2> toArray) {
  foreach (p, fromArray.domain().shrink(1, -1).shrink(1, -2)
           *(toArray.domain()/POINT(2, 2))) {
    toArray[p*2] += fromArray[p];
    toArray[p*2 + POINT(-1, 0)] += 0.5*(fromArray[p] +
                                        fromArray[p + POINT(-1, 0)]);
    toArray[p*2 + POINT(0, -1)] += 0.5*(fromArray[p] +
                                        fromArray[p + POINT(0, -1)]);
    toArray[p*2 + POINT(-1, -1)] +=
      0.25*(fromArray[p] +
            fromArray[p + POINT(0, -1)] +
            fromArray[p + POINT(-1, 0)] +
            fromArray[p + POINT(-1, -1)]);
  }
}

void nodal_multigrid::resid() {
  setBC();
  /*
    ndarray<double, 2> thisres = res;
    ndarray<double, 2> thisrhs = rhs;
    ndarray<double, 2> thisphi = phi;
  */
  double hsquared = h*h;
  foreach (p, interior.shrink(1)) {
    /*
      thisres[p] = (20.0*thisphi[p] - 4.0*(thisphi[p + POINT(1, 0)] 
      + thisphi[p + POINT(-1, 0)]
      + thisphi[p + POINT(0, 1)] 
      + thisphi[p + POINT(0, -1)])
      - (thisphi[p + POINT(1, 1)] + thisphi[p + POINT(1, -1)]
      + thisphi[p + POINT(-1, 1)] + thisphi[p + POINT(-1, -1)]))
      /(6.0*hsquared)	+ thisrhs[p];
    */
    res[p] = (20.0*phi[p] - 4.0*(phi[p + POINT(1, 0)] 
                                 + phi[p + POINT(-1, 0)]
                                 + phi[p + POINT(0, 1)] 
                                 + phi[p + POINT(0, -1)])
              - (phi[p + POINT(1, 1)] + phi[p + POINT(1, -1)]
                 + phi[p + POINT(-1, 1)] + phi[p + POINT(-1, -1)]))
      /(6.0*hsquared)	+ rhs[p];
  }
}

void nodal_multigrid::resid5() {
  setBC();
  double hsquared = h*h;
  foreach (p, interior.shrink(1)) {
    res[p] = (4.0*phi[p] - (phi[p + POINT(1, 0)] 
                            + phi[p + POINT(-1, 0)]
                            + phi[p + POINT(0, 1)] 
                            + phi[p + POINT(0, -1)]))/hsquared 
      + rhs[p];
  }
}
