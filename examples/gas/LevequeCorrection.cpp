#include "LevequeCorrection.h"

LevequeCorrection::LevequeCorrection(BoxArray &Ba,
                                     rectdomain<SPACE_DIM> DProblem,
                                     double Dx, int NumFluxes) {
  ba = Ba;
  indices = ba.indices();
  dProblem = DProblem;
  dx = Dx;
  numFluxes = NumFluxes;
  haveDims = ndarray<domain<1>, 1>(indices);

  ghosts.create(Util::DIMENSIONS);
  foreach (p, ghosts.domain()) {
    ghosts[p].create(Util::DIRECTIONS);
    foreach (q, ghosts[p].domain()) {
      ghosts[p][q].create(indices);
    }
  }

  correction.create(Util::DIMENSIONS);
  foreach (p, correction.domain()) {
    correction[p].create(Util::DIRECTIONS);
  }

  ndarray<int, 1> proclist = ba.proclist();

  foreach (pdim, Util::DIMENSIONS) {
    foreach (pdir, Util::DIRECTIONS) {
      ndarray<domain<SPACE_DIM>, 1> ghostsDimDir =
        ghosts[pdim][pdir];
      ndarray<rectdomain<SPACE_DIM>, 1> RghostsDimDir(indices);

      foreach (indbox, indices) {
        RghostsDimDir[indbox] =
          dProblem * ba[indbox].border(pdim[1]*pdir[1]);
        ghostsDimDir[indbox] = RghostsDimDir[indbox];

        // Remove cells belonging to other grids at this level.
        foreach (indother, indices)
          ghostsDimDir[indbox] = ghostsDimDir[indbox] -
            ba[indother];
      }

      BoxArray baDimDir(BoxArray(RghostsDimDir, proclist));

      correction[pdim][pdir] = LevelArray<Quantities>(baDimDir);
    }
  }
}

void LevequeCorrection::increment(ndarray<Quantities, SPACE_DIM> &Flux,
                                  double Dt, point<1> PatchIndex,
                                  int sweepDim) {
  // println("Entering lc.increment");
  double DtDx = Dt / dx;
  haveDims[PatchIndex] =
    haveDims[PatchIndex] + RD(sweepDim, sweepDim+1);

  /*
    Loop over the dimensions that have NOT yet been updated.
    In these dimensions, increment the corrections in the ghost cells,
    according to the change in flux in the dimension of the current sweep.

    e.g. forward in 3 dimensions:
    [1] increment sides in dimensions 2, 3
    [2] increment sides in dimension 3
    [3] nothing
  */
  // foreach (pdim, Util::DIMENSIONS - haveDims[PatchIndex])
  foreach (pdim, Util::DIMENSIONS) {
    if (! haveDims[PatchIndex].contains(pdim)) {
      foreach (pdir, Util::DIRECTIONS) {
        point<SPACE_DIM> unit =
          point<SPACE_DIM>::direction(pdir[1] * pdim[1]);
        /*
          If  p  is a ghost cell, then  p-unit  is a non-ghost
          cell on the boundary.
          Also recall that LevelGodunov.update() does
          state[p] = state[p] - ((flux[p + Util::cell2edge[+sweepDim]] -
                                  flux[p + Util::cell2edge[-sweepDim]]) *
                                  DtDx);
        */
        foreach (p, ghosts[pdim][pdir][PatchIndex]) {
          correction[pdim][pdir][PatchIndex][p] =
            correction[pdim][pdir][PatchIndex][p] -
            (Flux[p-unit + Util::cell2edge[+sweepDim]] -
             Flux[p-unit + Util::cell2edge[-sweepDim]]) * DtDx;
        }
      }
    }
  }
}
