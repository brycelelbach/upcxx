#include <cmath>
#include "RampBC.h"

void RampBC::f(ndarray<Quantities, SPACE_DIM> state,
               ndarray<double, SPACE_DIM, global> difCoefs,
               int sdim,
               ndarray<Quantities, SPACE_DIM> flux,
               double dx) {
  rectdomain<SPACE_DIM> sideDomain = state.domain(),
    fluxDomain = flux.domain();

  int dim = (sdim > 0) ? sdim : -sdim;
  int side = (sdim > 0) ? 1 : -1;

  // rectdomain<SPACE_DIM> fluxDomain = sideDomain + Util::cell2edge[sdim];

  // println("Setting flux at the boundary " << fluxDomain);

  if (dim == LENGTH_DIM) {
    if (side == -1) { // Left side
      foreach (e, fluxDomain) flux[e] = q1flux[dim];
    } else if (side == +1) { // Right side
      foreach (e, fluxDomain) flux[e] = q0flux[dim];
    }

  } else if (dim == HEIGHT_DIM) {

    int VELOCITY_N = velocityIndex(dim);
    if (side == -1) { // Bottom side:  solid wall to right of maxOffWall
      // change ceil to floor and then p[LENGTH_DIM] + 1 to p[LENGTH_DIM]?
      int maxOffWall = (int) ceil(shockLocBottom / dx);
      foreach (e, fluxDomain) {
        Quantities Ucell = state[e + Util::edge2cell[+dim]];
        Quantities qcell = lg0->consToPrim(Ucell);

        if (e[LENGTH_DIM] + 1 <= maxOffWall) {
          flux[e] = lg0->interiorFlux(qcell, dim);
        } else {
          Quantities artViscosity(VELOCITY_N,
                                  (Ucell[dim]/Ucell[0]) * difCoefs[e]);
          flux[e] =
            lg0->solidBndryFlux(qcell, sdim) + artViscosity * side;
        }
      }
    } else if (side == +1) { // Top side
      double sloc = shockLocTop + shockSpeed * amrAll->getFinetime();
      foreach (e, fluxDomain) {
        double leftEnd = (double) e[LENGTH_DIM] * dx;
        double rightEnd = leftEnd + dx;
        if (rightEnd <= sloc) {
          flux[e] = q1flux[dim];  // doesn't come out right?
        } else if (leftEnd >= sloc) {
          flux[e] = q0flux[dim];
        } else {
          double a1 = rightEnd - sloc;
          double a0 = sloc - leftEnd;
          flux[e] = (q1flux[dim] * a1 + q0flux[dim] * a0) / (a1 + a0);
        }
      }
    }

  } else {
    println("Bogus dimension " << dim);
  }
}


void RampBC::slope(ndarray<Quantities, SPACE_DIM> q,
                   const domain<SPACE_DIM> &domainSide,
                   int sdim,
                   ndarray<Quantities, SPACE_DIM> dq,
                   double dx) {

  int dim = (sdim > 0) ? sdim : -sdim;
  int side = (sdim > 0) ? 1 : -1;

  if (dim == LENGTH_DIM) {

    slope1(q, domainSide, sdim, dq);

  } else if (dim == HEIGHT_DIM) {

    if (side == -1) { // Bottom side:  solid wall to right of maxOffWall
      // change ceil to floor and then p[LENGTH_DIM] + 1 to p[LENGTH_DIM]?
      int maxOffWall = (int) ceil(shockLocBottom / dx);

      point<SPACE_DIM> unit = point<SPACE_DIM>::direction(LENGTH_DIM);
      point<SPACE_DIM> domainSideMax =
        (Util::ONE - unit) * domainSide.max() + unit * maxOffWall;
      domain<SPACE_DIM> domainFree = domainSide *
        RD(domainSide.min(), domainSideMax + Util::ONE);

      slope1(q, domainFree, sdim, dq);

      domain<SPACE_DIM> domainWall = domainSide - domainFree;
      slopeWall(q, domainWall, sdim, dq);

    } else if (side == +1) { // Top side

      slope1(q, domainSide, sdim, dq);

    }

  } else {
    println("Bogus dimension " << dim);
  }
}


void RampBC::slope1(ndarray<Quantities, SPACE_DIM> q,
                    const domain<SPACE_DIM> &domainSide,
                    int sdim,
                    ndarray<Quantities, SPACE_DIM> dq) {
  int dim = (sdim > 0) ? sdim : -sdim;
  int side = (sdim > 0) ? 1 : -1;

  point<SPACE_DIM> unitDir = point<SPACE_DIM>::direction(sdim);
  foreach (p, domainSide) {
    dq[p] = q[p] * (side * 1.5) +
      q[p - unitDir] * (-side * 2.0) +
      q[p - unitDir - unitDir] * (side * 0.5);
  }
}


void RampBC::slopeWall(ndarray<Quantities, SPACE_DIM> q,
                       const domain<SPACE_DIM> &domainSide,
                       int sdim,
                       ndarray<Quantities, SPACE_DIM> dq) {
  int dim = (sdim > 0) ? sdim : -sdim;
  int side = (sdim > 0) ? 1 : -1;
  point<SPACE_DIM> unitDir = point<SPACE_DIM>::direction(sdim);

  int VELOCITY_N = velocityIndex(dim);
  foreach (p, domainSide) {
    double u0 = q[p][VELOCITY_N];
    double u1 = q[p - unitDir][VELOCITY_N];
    double newterm = 0.0;
    if (u0 * (u1 - u0) > 0.0) {
      double sgnu0 = 0.0;
      if (u0 > 0.0) {
        sgnu0 = 1.0;
      } else if (u0 < 0.0) {
        sgnu0 = -1.0;
      }
      newterm = (-side * 2.0) * sgnu0 * fmin(fabs(u1 - u0), fabs(u0));
    }
    dq[p] = Quantities(VELOCITY_N, newterm);
  }
}

