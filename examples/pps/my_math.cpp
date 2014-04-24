#include "my_math.h"
#include "location.h"

/** Cell-centered averaging.  This is no longer used in my code, but
    it is useful to have for other implementations.  Specifically,
    this code calculates the average of the four fine cells at the
    center of each projected coarse cell, and assigns that value to
    the coarse cell. */
void my_math::ccaverage(ndarray<double, 2, global> fine,
                        ndarray<double, 2, global> coarse,
                        int nref) {
  double sum;
  foreach (p, coarse.domain()
           *(my_box::coarsen(fine.domain().accrete(1), nref).shrink(1))) {
    sum = 0;
    foreach (fp, RectDomain<2>(p*nref + POINT(nref/2 - 1, nref/2 - 1),
                               p*nref + POINT(nref/2, nref/2) +
                               POINT(1, 1))) {
      sum += fine[fp];
    }
    coarse[p] = sum/4.0;
  }
}

/** Set the boundary values of an array to zero. */
void my_math::zerobc(ndarray<double, 2, global> a) {
  ndarray<double, 1, global> side;
  Point<2> mm;
  int ii;
  for (mm = a.domain().min(), ii = 0; ii < 2; mm = a.domain().max(), ii++) {
    for (int d = 1; d <=2 ; d++) {
      side = a.slice(d, mm[d]);
      foreach (p, side.domain()) {
        side[p] = 0.;
      }
    }
  }
}

/** Calculate the center of the charge, or the first moment of the
    charge. */
ndarray<double, 1> my_math::centerofcharge(ndarray<double, 2, global> a,
                                           double q, double h) {
  ndarray<double, 1> center(RECTDOMAIN((1), (3)));
  center.set(0.);
  if (q != 0.0) {
    foreach (p, a.domain()) {
      foreach (i, center.domain()) {
        center[i] += (p[i[1]])*h*a[p];
      }
    }
    foreach (i, center.domain()) {
      center[i] = center[i]*h*h/q;
    }
  }

  return center;
}

/** Set the boundary of a solution array with an estimate of an
    infinite domain boundary condition.  This estimate is calculated
    by approximating the charge field as a single point charge
    located at the first moment of the charge field. */
void my_math::nidbound(ndarray<double, 2, global> soln,
                       ndarray<double, 2, global> rhs, double h) {
  /* Find the sum of the charges. */
  double q = sumcharge(rhs, h);

  /* Find the center of the charge. */
  ndarray<double, 1> center = centerofcharge(rhs, q, h);

  double pi = PI;
  ndarray<double, 1, global> side;
  double distsq;
  int i, j;

  /* Top boundary */
  j = soln.domain().max()[2];
  side = soln.slice(2, j);
  foreach (p, side.domain().shrink(1)) {
    distsq = (p[1]*h - center[1])*(p[1]*h - center[1]) 
      + (j*h - center[2])*(j*h - center[2]);
    side[p] += q*log(distsq)/(4.0*pi);
  }

  /* Bottom boundary */
  j = soln.domain().min()[2];
  side = soln.slice(2, j);
  foreach (p, side.domain().shrink(1)) {
    distsq = (p[1]*h - center[1])*(p[1]*h - center[1]) 
      + (j*h - center[2])*(j*h - center[2]);
    side[p] += q*log(distsq)/(4.0*pi);
  }

  /* Right boundary */
  i = soln.domain().max()[1];
  side = soln.slice(1, i);
  foreach (p, side.domain()) {
    distsq = (i*h - center[1])*(i*h - center[1])
      + (p[1]*h - center[2])*(p[1]*h - center[2]);
    side[p] += q*log(distsq)/(4.0*pi);
  }
    
  /* Left boundary */
  i = soln.domain().min()[1];
  side = soln.slice(1, i);
  foreach (p, side.domain()) {
    distsq = (i*h - center[1])*(i*h - center[1])
      + (p[1]*h - center[2])*(p[1]*h - center[2]);
    side[p] += q*log(distsq)/(4.0*pi);
  }

  center.destroy();
}

/** This method calculates the nine-point laplacian of an input
    grid. */
void my_math::nlaplacian(ndarray<double, 2, global> input,
                         ndarray<double, 2, global> lapl, double h) {
  RectDomain<2> interior;
  interior = lapl.domain()*input.domain().shrink(1);

  foreach (p, interior) {
    lapl[p] = (4.0*(input[p - POINT(1, 0)] + input[p + POINT(1, 0)]
                    + input[p - POINT(0, 1)] + input[p + POINT(0, 1)])
               + (input[p + POINT(1, 1)] + input[p + POINT(1, -1)]
                  + input[p + POINT(-1, 1)] + input[p + POINT(-1, -1)])
               - 20.0*input[p])/(6.0*h*h);
  }
}

/** This method calculates the five-point laplacian of an input
    grid. */
void my_math::nlaplacian5(ndarray<double, 2, global> input,
                          ndarray<double, 2, global> lapl, double h) {
  RectDomain<2> interior;
  interior = lapl.domain()*input.domain().shrink(1);

  foreach (p, interior) {
    lapl[p] = (input[p - POINT(1, 0)] + input[p + POINT(1, 0)]
               + input[p - POINT(0, 1)] + input[p + POINT(0, 1)]
               - 4.0*input[p])/(h*h);
  }
}

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
void my_math::nodal_potential_calc(ndarray<double, 2, global> phiH,
                                   ndarray<double, 2, global> phi,
                                   ndarray<double, 2, global> extra,
                                   ndarray<double, 2, global> rhs,
                                   double h) {
  double q = sumcharge(rhs, h);
  ndarray<double, 1> center = centerofcharge(rhs, q, h);
  double pi = PI;
  ndarray<double, 1, global> onein;
  ndarray<double, 1, global> twoin;
  ndarray<double, 1, global> threein;
  ndarray<double, 1, global> fourin;
  int i, j, d, dH;

  /* The following loops calculate a fourth-order estimate of
     d(Phi)/d(n) at the boundary, where n is the unit normal. */

  /* In an attempt to make things go faster, I want to put all 
     boundary values in a [1d] array.  This change is still under
     development. */

  // int Hboundsize = 2*(phiH.domain().max()[1] - phiH.domain().min()[1]
  //                     + phiH.domain().max()[0] - phiH.domain().min()[0]);
  // ndarray<double, 1> Hbound(RECTDOMAIN((0), (Hboundsize)));
  // ndarray<location, 1> hbLocation(RECTDOMAIN((0), (Hboundsize)));
  // ndarray<int, 1> dir(RECTDOMAIN((1), (3)));
  int ii, sign;
  double fixedboundary, cx, cy;
  Point<2> mm;
  //    Point<1> offset = [0];

  for (d = 1; d <= 2; d++) {
    for (mm = phiH.domain().min(), ii = 0, sign = 1; ii < 2;
         mm = phiH.domain().max(), ii++, sign *= -1) {

      /* This is what you start doing when you wish too much that
         your code were smaller: cx and cy remap center so that
         when you're working on the left and right boundaries cx =
         center[1] and cy = center[2], but when you're working on
         the top and bottom boundaries, cx = center[2] and cy =
         center[1].  Certainly I could have written clearer code in
         less space than this comment, but maybe this will be a
         little faster.  hahahahaha */
      cx = center[d];
      cy = center[3 - d];
      fixedboundary = mm[d]*h;
      ndarray<double, 1, global> side = phiH.slice(d, mm[d]);
      onein = phiH.slice(d, mm[d] + sign*1);
      twoin = phiH.slice(d, mm[d] + sign*2);
      threein = phiH.slice(d, mm[d] + sign*3);
      fourin = phiH.slice(d, mm[d] + sign*4);
      RectDomain<1> bd = side.domain().shrink(d-1);
      //	foreach (p, bd) {
      foreach (p, side.domain().shrink(d - 1)) {
        side[p] = h*( ( -25.0*side[p]/12.0 + 4.0*onein[p] - 3.0*twoin[p]
                        + 4.0*threein[p]/3.0 - 0.25*fourin[p] )/h
                      - q*sign*(mm[d]*h - center[d]
                                )/(2.0*pi*( (fixedboundary - cx)
                                            *(fixedboundary - cx)
                                            + (p[1]*h - cy)
                                            *(p[1]*h - cy) )) );
        /* Hbound should be faster to use in the n^2 algorithm below. */
        //  NOT DONE YET!!!
        //  	  Hbound[ p - [bd.min()[1]] + offset ] = 
        //  	    h*( ( -25.0*side[p]/12.0 + 4.0*onein[p] - 3.0*twoin[p]
        //  		  + 4.0*threein[p]/3.0 - 0.25*fourin[p] )/h
        //  		- q*sign*(mm[d]*h - center[d]
        //  			  )/(2.0*pi*( (fixedboundary - cx)
        //  				      *(fixedboundary - cx)
        //  				      + (p[1]*h - cy)
        //  				      *(p[1]*h - cy) )) );
        //  	  hbLocation[ p - [bd.min()[1]] + offset ] = 
        //  	    new location(h*((2 - d)*mm[d] + (d - 1)*p[1]),
        //  			 h*((d - 1)*mm[d] + (2 - d)*p[1]));
      }
      //	offset = offset + POINT(phiH.domain().max()[d] - phiH.domain().min()[d]);
    }	
  }

  phi.set(0.0);

  if (q != 0.0) {

    /* We are doing Richardson-Romberg integration here.  To get the
       accuracy that we want, we only need to make two passes.  The
       first pass uses every second point; the second uses every
       point not used by the first pass.  Also, the second pass uses
       the value from the previous pass.  Finally, all these
       estimates are used to extrapolate a high order estimate of
       the integral. 
	 
       In reality, we're just using Simpson's rule, so we could
       definitely do this in a clearer way, and probably in a faster
       way, too. */
    
    /* First Pass */
    int iiH;
    double dist, hsquared;
    hsquared = h*h;
    Point<2> mmH, mHmin, mHmax, mmin, mmax;
    Point<2> xHPoint, xPoint, xHminPoint, xHmaxPoint, xminPoint, xmaxPoint;
    RectDomain<1> sidecounterH;
    ndarray<double, 1, global> sideH;
    ndarray<double, 1, global> side;
    ndarray<double, 1, global> sideHmin;
    ndarray<double, 1, global> sideHmax;
    ndarray<double, 1, global> sidemin;
    ndarray<double, 1, global> sidemax;
    double distmax, distmin;

    for (dH = 1; dH <= 2; dH++) {
      for (mmH = phiH.domain().min(), iiH = 0; iiH < 2;
           mmH = phiH.domain().max(), iiH++) {
        sidecounterH = RECTDOMAIN((phiH.domain().min()[3 - dH] + 2*(dH - 1)),
                                  (phiH.domain().upb()[3 - dH] - 2*(dH - 1) + 1),
                                  (2));
        sideH = phiH.slice(dH, mmH[dH]);
        foreach (pH, sidecounterH) {
          xHPoint = POINT((2 - dH)*mmH[dH] + (dH - 1)*pH[1],
                          (dH - 1)*mmH[dH] + (2 - dH)*pH[1]);
          for (d = 1; d <= 2; d++) {
            for (mm = phi.domain().min(), ii = 0; ii < 2;
                 mm = phi.domain().max(), ii++) {
              side = phi.slice(d, mm[d]);
              foreach (p, side.domain().shrink(d - 1)) {
                xPoint = POINT((2 - d)*mm[d] + (d - 1)*p[1],
                               (d - 1)*mm[d] + (2 - d)*p[1]);
                xPoint = ((xPoint - xHPoint)*(xPoint - xHPoint));
                dist = (xPoint[1] + xPoint[2])*hsquared;
                side[p] = side[p] - 2.0*sideH[pH]*log(dist)/(4.0*pi);
              }
            }
          }
        }
      }
    }

    /* Second Pass */
    for (d = 1; d <= 2; d++) {
      for (mm = phi.domain().min(), ii = 0; ii < 2;
           mm = phi.domain().max(), ii++) {
        side = phi.slice(d, mm[d]);
        sideH = extra.slice(d, mm[d]);
        foreach (p, side.domain()) {
          sideH[p] = 0.5*side[p];
        }
      }
    }

    for (dH = 1; dH <= 2; dH++) {
      for (mmH = phiH.domain().min(), iiH = 0; iiH < 2;
           mmH = phiH.domain().max(), iiH++) {
        sidecounterH = RECTDOMAIN((phiH.domain().min()[3 - dH] + 1),
                                  (phiH.domain().max()[3 - dH]),
                                  (2));
        sideH = phiH.slice(dH, mmH[dH]);
        foreach (pH, sidecounterH) {
          xHPoint = POINT((2 - dH)*mmH[dH] + (dH - 1)*pH[1],
                          (dH - 1)*mmH[dH] + (2 - dH)*pH[1]);
          for (d = 1; d <= 2; d++) {
            for (mm = extra.domain().min(), ii = 0; ii < 2;
                 mm = extra.domain().max(), ii++) {
              side = extra.slice(d, mm[d]);
              foreach (p, side.domain().shrink(d - 1)) {
                xPoint = POINT((2 - d)*mm[d] + (d - 1)*p[1],
                               (d - 1)*mm[d] + (2 - d)*p[1]);
                xPoint = ((xPoint - xHPoint)*(xPoint - xHPoint));
                dist = (xPoint[1] + xPoint[2])*hsquared;
                side[p] = side[p] - sideH[pH]*log(dist)/(4.0*pi);
              }
            }
          }
        }
      }
    }

    /* Richardson extrapolation */
    for (d = 1; d <= 2; d++) {
      for (mm = phi.domain().min(), ii = 0; ii < 2;
           mm = phi.domain().max(), ii++) {
        side = phi.slice(d, mm[d]);
        sideH = extra.slice(d, mm[d]);
        foreach (p, side.domain().shrink(d - 1)) {
          side[p] = (4.0*sideH[p] - side[p])/3.0;
        }
      }
    }
  }

  center.destroy();
  // Hbound.destroy();
  // hbLocation.destroy();
  // dir.destroy();
}

/** This is a lower-accuracy version of the nodal_potential_calc
    method, designed to provide sufficient accuracy to be compatible
    with a five-point laplacian operator. */
void my_math::nodal_potential_calc5(ndarray<double, 2, global> phiH,
                                    ndarray<double, 2, global> phi,
                                    ndarray<double, 2, global> extra,
                                    ndarray<double, 2, global> rhs,
                                    double h) {
  double q = sumcharge(rhs, h);
  ndarray<double, 1> center = centerofcharge(rhs, q, h);
  double pi = PI;
  ndarray<double, 1, global> onein;
  ndarray<double, 1, global> twoin;
  int i, j, d, dH;

  // ndarray<int, 1> dir(RECTDOMAIN((1), (3)));
  int ii, sign;
  double fixedboundary, cx, cy;
  Point<2> mm;
  for (d = 1; d <= 2; d++) {
    for (mm = phiH.domain().min(), ii = 0, sign = 1; ii < 2;
         mm = phiH.domain().max(), ii++, sign *= -1) {
      /** This is what you start doing when you wish too much that
          your code were smaller: cx and cy remap center so that
          when you're working on the left and right boundaries cx =
          center[1] and cy = center[2], but when you're working on
          the top and bottom boundaries, cx = center[2] and cy =
          center[1].  Certainly I could have written clearer code in
          less space than this comment, but maybe this will be a
          little faster.  hahahahaha */
      cx = center[d];
      cy = center[3 - d];
      fixedboundary = mm[d]*h;
      ndarray<double, 1, global> side = phiH.slice(d, mm[d]);
      onein = phiH.slice(d, mm[d] + sign*1);
      twoin = phiH.slice(d, mm[d] + sign*2);
      foreach (p, side.domain().shrink(d - 1)) {
        side[p] = h*( ( -1.5*side[p] + 2.0*onein[p] 
                        - 0.5*twoin[p])/h
                      - q*sign*(mm[d]*h - center[d]
                                )/(2.0*pi*( (fixedboundary - cx)
                                            *(fixedboundary - cx)
                                            + (p[1]*h - cy)
                                            *(p[1]*h - cy) )) );
      }
    }	
  }

  phi.set(0.0);

  if (q >= 1.0e-10) {

    /* We are _not_ doing Richardson-Romberg integration here. */
    
    /* First and Only Pass */
    int iiH;
    double dist, hsquared;
    hsquared = h*h;
    Point<2> mmH, mHmin, mHmax, mmin, mmax;
    Point<2> xHPoint, xPoint, xHminPoint, xHmaxPoint, xminPoint, xmaxPoint;
    RectDomain<1> sidecounterH;
    ndarray<double, 1, global> sideH;
    ndarray<double, 1, global> side;
    ndarray<double, 1, global> sideHmin;
    ndarray<double, 1, global> sideHmax;
    ndarray<double, 1, global> sidemin;
    ndarray<double, 1, global> sidemax;
    double distmax, distmin;

    for (dH = 1; dH <= 2; dH++) {
      for (mmH = phiH.domain().min(), iiH = 0; iiH < 2;
           mmH = phiH.domain().max(), iiH++) {
        sidecounterH = RECTDOMAIN((phiH.domain().min()[3 - dH] + (dH - 1)),
                                  (phiH.domain().upb()[3 - dH] - (dH - 1) + 1));
        sideH = phiH.slice(dH, mmH[dH]);
        foreach (pH, sidecounterH) {
          xHPoint = POINT((2 - dH)*mmH[dH] + (dH - 1)*pH[1],
                          (dH - 1)*mmH[dH] + (2 - dH)*pH[1]);
          for (d = 1; d <= 2; d++) {
            for (mm = phi.domain().min(), ii = 0; ii < 2;
                 mm = phi.domain().max(), ii++) {
              side = phi.slice(d, mm[d]);
              foreach (p, side.domain().shrink(d - 1)) {
                xPoint = POINT((2 - d)*mm[d] + (d - 1)*p[1],
                               (d - 1)*mm[d] + (2 - d)*p[1]);
                xPoint = ((xPoint - xHPoint)*(xPoint - xHPoint));
                dist = (xPoint[1] + xPoint[2])*hsquared;
                side[p] = side[p] - sideH[pH]*log(dist)/pi;
              }
            }
          }
        }
      }
    }
  }

  center.destroy();
  // dir.destroy();
}

/** Code to make a star-shaped right hand side. */
void my_math::make_star_rhs(ndarray<double, 2, global> rhs, double h,
                            ndarray<double, 1, global> center,
                            double r0, int m) {
  double rtemp, r_star, theta, s;
  double pi = PI;
  double w = 2.0*pi*((double) m);

  foreach(p, rhs.domain()) {
    rtemp = sqrt( (p[1]*h - center[1])
                  *(p[1]*h - center[1])
                  + (p[2]*h - center[2])
                  *(p[2]*h - center[2]) );
    theta = acos((p[1]*h - center[1])/rtemp);
    r_star = r0*(cos(6.0*theta) + 3.0)/4.0;
    if (rtemp > r_star) {
      rhs[p] += 0;
    }
    else {
      s = (sin(w*rtemp/r_star))*(1.0 - rtemp/r_star)*rtemp/r_star;
      s *= s;
      rhs[p] += s;
    }
  }
}
  
/** Code to make a nice, round right hand side. */
void my_math::makeRhs(ndarray<double, 2, global> rhs, double h, 
                      ndarray<double, 1, global> center,
                      double r0, int m) {
  double rtemp, s;
  double pi = PI;
  double w = 2.0*pi*((double) m)/r0;
  foreach(p, rhs.domain()) {
    rtemp = sqrt( (p[1]*h - center[1])
                  *(p[1]*h - center[1])
                  + (p[2]*h - center[2])
                  *(p[2]*h - center[2]) );
    if (rtemp > r0) { /* rhs is zero */
      rhs[p] += 0; /* For fake load balancing: change to
                      h*h*h*h*h*h*h; */ 
    }
    else {
      switch (m) {
      case -1: /* very simple rhs to test theory */
        rhs[p] += r0 - rtemp;
        break;
      case 0: /* smooth rhs */
        rhs[p] += pow(( (1.0 - rtemp/r0)*(rtemp/r0) ), 4);
        break;
      default: /* high wavenumber right hand side */
        s = (sin(w*rtemp))*(1.0 - rtemp/r0)*(rtemp/r0);
        s *= s;
        rhs[p] += s;
        break;
      }
    }
  }
}  

/** This method calculates the exact solution expected for several
    of the possible right hand sides defined above.  No analytical
    solution is available for the star-shaped right hand side.  For
    the high wavenumber solution, a closed-form analytical solution
    does not exist, so there is a lot of hoop jumping to get an
    accurate representation of the solution in the high wavenumber
    case. */
void my_math::exactSoln(ndarray<double, 2, global> soln, double h,
                        ndarray<double, 1, global> center,
                        double r0, int m) {
  double pi = PI;
  double r, rtemp, logr, logr0,  a, phi1, phi2, phi3, phi4;
  double w;
  double lterm, lterm_r0, sterm, cterm;

  //soln.set(0.);

  switch (m) {
  case -1:
    foreach (p, soln.domain()) {
      r = sqrt( (p[1]*h - center[1])*(p[1]*h - center[1])
                + (p[2]*h - center[2])*(p[2]*h - center[2]) );
      if (r < r0) {
        soln[p] += (r*r - r0*r0)*r0/4.0 - (r*r*r - r0*r0*r0)/9.0
          + 3.0*pi*r0*r0*r0*logr0/3.0;
      }
      else {
        soln[p] += pi*r0*r0*r0*log(r)/3.0;
      }
    }
    break;
  case 0:
    foreach (p, soln.domain()) {
      r = sqrt( (p[1]*h - center[1])*(p[1]*h - center[1])
                + (p[2]*h - center[2])*(p[2]*h - center[2]) );
      if (r == 0.) {
        logr = 0.;
      }
      else {
        logr = log(r);
      }
      if (r < r0) {
        a = r/r0;
        phi1 = (0.1*a*a*a*a - 4.0*a*a*a/9.0 + 0.75*a*a - 4.0*a/7.0
                + 1.0/6.0)*a*a*a*a*r*r*logr;
        phi2 = (0.1*a*a*a*a*(0.1 - logr) - 4.0*a*a*a*(1.0/9.0 - logr)/9.0
                + 0.75*a*a*(0.125 - logr) - 4.0*a*(1.0/7.0 - logr)/7.0
                + (1.0/6.0 - logr)/6.0)*a*a*a*a*r*r;
        phi3 = (1.0/6.0 - 4.0/7.0 + 0.75 - 4.0/9.0 + 0.1)*r0*r0*log(r0)
          - (1.0/36.0 - 4.0/49.0 + 3.0/32.0 - 4.0/81.0 + 0.01)*r0*r0;
        phi4 = (0.01*a*a*a*a - (4.0/81.0)*a*a*a + (3.0/32.0)*a*a
                - (4.0/49.0)*a + 1.0/36.0)*a*a*a*a*r*r;
        soln[p] += phi3 + phi4;
      }
      else {
        soln[p] += (1.0/6.0 - 4.0/7.0 + 0.75 - 4.0/9.0
                    + 0.1)*r0*r0*logr;
      }
    }
    break;
  default: /* m is the number of waves in the right hand side */
    w = 2.0*pi*((double) m)/r0;
    logr0 = log(r0);
    foreach (p, soln.domain()) {
      r = sqrt( (p[1]*h - center[1])*(p[1]*h - center[1])
                + (p[2]*h - center[2])*(p[2]*h - center[2]) );
      if (r == 0.) {
        logr = 0.;
      }
      else {
        logr = log(r);
      }
      if (r < r0) { /*, support of right hand side */
        cterm = cos(2.0*w*r)*(- 11.0/(32.0*w*w*w*w*r0*r0)
                              + 137.0/(64.0*w*w*w*w*w*w*r0*r0*r0*r0)
                              + (13.0*r)/(8.0*w*w*w*w*r0*r0*r0)
                              + (r*r)/(8.0*w*w*r0*r0)
                              - (r*r*r)/(4.0*w*w*r0*r0*r0)
                              + (r*r*r*r)/(8.0*w*w*r0*r0*r0*r0)
                              - (47.0*r*r)/(32.0*w*w*w*w*r0*r0*r0*r0));
        sterm = sin(2.0*w*r)*(- 25.0/(16.0*w*w*w*w*w*r0*r0*r0)
                              - (5.0*r)/(16.0*w*w*w*r0*r0)
                              + (77.0*r)/(32.0*w*w*w*w*w*r0*r0*r0*r0)
                              + (7.0*r*r)/(8.0*w*w*w*r0*r0*r0)
                              - (9.0*r*r*r)/(16.0*w*w*w*r0*r0*r0*r0));
        /* now we have to integrate the terms that have things like
           cos(r)/r and sin(r)/r */
        lterm = log_type_term((double) m, r, r0);
        lterm_r0 = log_type_term((double) m, r0, r0);
	    
        soln[p] = ( (r*r*r*r)/(32.0*r0*r0) - (r*r*r*r*r)/(25.0*r0*r0*r0)
                    + (r*r*r*r*r*r)/(72.0*r0*r0*r0*r0) - (37.0*r0*r0)/7200.0
                    + cterm - (137.0/(64.0*w*w*w*w*w*w*r0*r0*r0*r0)
                               -3.0/(16.0*w*w*w*w*r0*r0))
                    + sterm 
                    + lterm - lterm_r0 
                    + r0*r0*logr0*(1.0 + 45.0/(w*w*w*w*r0*r0*r0*r0))/120.0 );
      }
      else { /* outside support of right hand side */
        soln[p] = r0*r0*logr*(1.0 + 45.0/(w*w*w*w*r0*r0*r0*r0))/120.0;
      }
    }
    break;
  }
}

/** This is one of the hoops that I have to jump through since a
    closed-form solution does not exist for the high wavenumber
    right hand side.  I need to calculate a good estimate of
    something that behaves like a log but disappears at the
    origin. */
double my_math::log_type_term(double m, double r, double r0) {
  double rtemp;
  double pi = PI;
  double w = 2.0*pi*m/r0;
  double small_amount = pi/8.0;
  int points_per_wave = 10;
  int factor, count, max_count, i, j, i_max, j_max;
  int points;
  double term, lterm, cos_integral, sin_integral;
  double min, max, midpoint, c_sum, s_sum, fours;

  /* I use Taylor expansion only near origin, beyond that
     Richardson-Romberg is a better choice. */ 
  if (2.0*w*r > small_amount) { 
    rtemp = small_amount;
  }
  else { /* just use Taylor expansion */
    rtemp = 2.0*w*r;
  }
  max_count = 61;
  for (count = 1; count < max_count; count++) {
    term = 1.0;
    for (factor = count; factor > 0; factor--) {
      term *= (rtemp)/( (double) factor );
    }
    term /= (double) count;
    term *= ( (count/2)%2 == 0 ) ? 1.0 : -1.0;
    if ( (count%2) == 0 ) { /* increment cosine integral */
      cos_integral += term;
    }
    else { /* increment sine integral */
      sin_integral += term;
    }
  }

  /* Time for Richardson-Romberg integration away from the origin. */
  if (2.0*w*r > small_amount) { 
    min = small_amount/(2.0*w);
    max = r;
    i_max = 1;
    points = 2;
    while ( points < 2*m*points_per_wave) {
      i_max += 1;
      points *= 2;
    }
    j_max = i_max;
    ndarray<double, 2> c_integral(RECTDOMAIN((0, 0), (i_max, j_max)));
    ndarray<double, 2> s_integral(RECTDOMAIN((0, 0), (i_max, j_max)));
    // for (i = 0; i < i_max; i++) {
    //   c_integral[i] = new double [j_max];
    //   s_integral[i] = new double [j_max];
    // }
    c_integral[0][0] = 0.5*(max - min)
      *(cos(2.0*w*min)/min + cos(2.0*w*max)/max);
    s_integral[0][0] = 0.5*(max - min)
      *(sin(2.0*w*min)/min + sin(2.0*w*max)/max);

    for (i = 1; i < i_max; i++) {
      points = 1;
      for (count = i; count > 0; count--) {
        points *= 2;
      }
      c_sum = 0;
      s_sum = 0;
      for (count = 1; count < points; count += 2) {
        midpoint = min + ((double) count)*(max - min)/
          ((double) points);
        c_sum += cos(2.0*w*midpoint)/midpoint;
        s_sum += sin(2.0*w*midpoint)/midpoint;
      }
      c_integral[i][0] += 0.5*(c_integral[i-1][0]
                               + (max - min)*c_sum/
                               ((double) (points/2)));
      s_integral[i][0] += 0.5*(s_integral[i-1][0]
                               + (max - min)*s_sum/
                               ((double) (points/2)));
    }
    for (j = 1; j < j_max; j++) {
      for (i = 0; i < j_max - j; i++) {
        fours = 4.0;
        for (count = 1; count < j; count++) {
          fours *= 4.0;
        }
        c_integral[i][j] = (fours*c_integral[i+1][j-1]
                            - c_integral[i][j-1])/(fours - 1.0);
        s_integral[i][j] = (fours*s_integral[i+1][j-1]
                            - s_integral[i][j-1])/(fours - 1.0);
      }
    }
    /* Add these results to the part of the integral near the
       origin. */ 
    cos_integral += c_integral[0][j_max - 1];
    sin_integral += s_integral[0][j_max - 1];
    /* Subtract off logarithm part from cos_integral. */
    cos_integral -= log(max) - log(min);

    c_integral.destroy();
    s_integral.destroy();
  }

  lterm =( ( 3.0/(4.0*w*w*w*w*w*r0*r0*r0) )*sin_integral
           + ( 3.0/(16.0*w*w*w*w*r0*r0) 
               - 15.0/(16.0*w*w*w*w*w*w*r0*r0*r0*r0)
               )*cos_integral );

  return lterm;
}
