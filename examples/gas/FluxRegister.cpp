#include "FluxRegister.h"

FluxRegister::FluxRegister(BoxArray &Ba, BoxArray &BaCoarse,
                           rectdomain<SPACE_DIM> DProblem,
                           int NRefine) {
  ba = Ba;
  baCoarse = BaCoarse;
  dProblem = DProblem;

  nRefine = NRefine;
  nRefineP = point<SPACE_DIM>::all(nRefine);
  dRefine = (double) nRefine;

  wtFace = 1.0;
  for (int d = 1; d < SPACE_DIM; d++)
    wtFace /= dRefine;

  rectdomain<1> inds = (rectdomain<1>) ba.indices();
  rectdomain<1> indsC = (rectdomain<1>) baCoarse.indices();
  ndarray<int, 1> proclist = ba.proclist();

  registers.create(Util::DIMENSIONS);
  foreach (p, registers.domain()) {
    registers[p].create(Util::DIRECTIONS);
  }

  domains.create(Util::DIMENSIONS);
  foreach (p, domains.domain()) {
    domains[p].create(Util::DIRECTIONS);
    foreach (q, domains[p].domain()) {
      domains[p][q].create(inds);
    }
  }

  fineBoxes.create(Util::DIMENSIONS);
  foreach (p, fineBoxes.domain()) {
    fineBoxes[p].create(Util::DIRECTIONS);
    foreach (q, fineBoxes[p].domain()) {
      fineBoxes[p][q].create(indsC);
    }
  }

  // segmentsDimDir will be reused.
  ndarray<rectdomain<SPACE_DIM>, 1> segmentsDimDir(inds);

  foreach (pdim, Util::DIMENSIONS) {
    foreach (pdir, Util::DIRECTIONS) {
      ndarray<domain<SPACE_DIM>, 1> domainsDimDir =
        domains[pdim][pdir];

      ndarray<domain<1>, 1> fineBoxesDimDir = fineBoxes[pdim][pdir];
      foreach (indboxC, indsC)
        fineBoxesDimDir[indboxC] = RD(0, 0);

      int dimdir = pdim[1] * pdir[1];

      foreach (indbox, inds) {
        // Find the coarse cells that border ba[indbox] in this
        // direction. Put these in the rectdomain<SPACE_DIM>
        // segmentsDimDir[indbox].
        segmentsDimDir[indbox] =
          (ba[indbox].border(dimdir) * dProblem) / nRefineP;

        // Copy segmentsdimDir[indbox] to the domain<SPACE_DIM>
        // domainsDimDir[indbox]. Later, these domains will be
        // modified by removing segments that are common to two
        // different boxes.
        domainsDimDir[indbox] = segmentsDimDir[indbox];

        // If this domain intersects a coarse box, record the index.
        foreach (indboxC, indsC)
          if (! (domainsDimDir[indbox] *
                 baCoarse[indboxC]).is_empty())
            fineBoxesDimDir[indboxC] =
              fineBoxesDimDir[indboxC] + RD(indbox, indbox+1);
      }

      BoxArray baDimDir(segmentsDimDir, proclist);
      registers[pdim][pdir] = LevelArray<Quantities>(baDimDir);
    }
  }

  /*
    Now trim domains:  Remove segments common to two different boxes.

    box:     indL   indR
               |      |
               v      |
          __________  v
         |          |L__      Initially...
         |         R|L  |
         |         R|L__|     L:  domains[1][+1][indL]
         |          |L        R:  domains[1][-1][indR]
         |__________|L

    Check the intersection (cells on the right):
    (domains[dim][-1][indR] + unit) * domains[dim][+1][indL]
    where
    unit = point<SPACE_DIM>.direction(dim)
  */

  foreach (pdim, Util::DIMENSIONS) {
    point<SPACE_DIM> unit = point<SPACE_DIM>::direction(pdim[1]);
    ndarray<ndarray<domain<SPACE_DIM>, 1>, 1> domDim =
      domains[pdim];
    foreach (indR, inds) {
      foreach (indL, inds) {
        domain<SPACE_DIM> intersect =
          (domDim[-1][indR] + unit) * domDim[+1][indL];
        if (! intersect.is_empty()) {
          domDim[-1][indR] = domDim[-1][indR] - (intersect - unit);
          domDim[+1][indL] = domDim[+1][indL] - intersect;
        }
      }
    }
  }
}
