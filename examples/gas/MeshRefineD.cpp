#include "MeshRefineD.h"

void MeshRefineD::getPNDs(ndarray<ndarray<rectdomain<SPACE_DIM>, 1>,
                          1> &oldGrids) {
  ProperNestingDomain.create(oldGrids.domain());

  foreach (h, oldGrids.domain()) {
    // First find the domain covered by grids at level h.

    // println("getPND for " << h[1]);
    domain<SPACE_DIM> levDomain = Util::EMPTY;
    foreach (indgrid, oldGrids[h].domain())
      levDomain = levDomain + oldGrids[h][indgrid];

    domain<SPACE_DIM> PND = levDomain;
    foreach (pdim, Util::DIMENSIONS) {
      foreach (pdir, Util::DIRECTIONS) {
        // extend one cell in each direction
        Point<SPACE_DIM> shift =
          Point<SPACE_DIM>::direction(pdim[1] * pdir[1], 1);

        domain<SPACE_DIM> remove =
          (((PND + shift) - PND) * dProblem[h]) - shift;
        // println("removing " << remove);
        PND = PND - remove;
      }
    }
    ProperNestingDomain[h] = PND;
  }
}

ndarray<ndarray<rectdomain<SPACE_DIM>, 1>, 1> MeshRefineD::refine() {
  // println("MeshRefineD.refine()");

  ndarray<ndarray<rectdomain<SPACE_DIM>, 1>,
          1> returnGrids(RD(base+1, top+2));

  for (int h = top; h >= base; h--) {

    if (tagged[h].is_empty()) {
      returnGrids[h+1] = NULL;
      break;
    }

    // (3) Find grids covering tagged points in patches at level h.

    // println("Getting GridGenerator for " << h);
    // println("Calling GridGenerator with " << tagged[h]);
    GridGenerator gg(tagged[h], ProperNestingDomain[h]);
    // println("makeGrids");
    ndarray<rectdomain<SPACE_DIM>, 1> newGrids = gg.makeGrids();

    // (4) Refine grids and store in returnGrids[h+1].

    // println("refineGrids for "+ h);
    Util::refine(newGrids, ref_ratio[h]);
    returnGrids[h+1].create(newGrids.domain());
    foreach (indgrid, newGrids.domain())
      returnGrids[h+1][indgrid] = newGrids[indgrid];

    // (5) Expand grids by one cell in each direction (clipping at
    // physical boundaries) and then coarsen and union with tags
    // at level h-1.
    // Omit this step for the base level.

    if (h > base) {
      Util::coarsen(newGrids, ref_ratio[h]);  // back to level h

      // Extend each grid one cell in each direction.
      foreach (indgrid, newGrids.domain()) {
        foreach (pdim, Util::DIMENSIONS) {
          foreach (pdir, Util::DIRECTIONS) {
            Point<SPACE_DIM> shift =
              Point<SPACE_DIM>::direction(pdim[1] * pdir[1], 1);
            newGrids[indgrid] = (rectdomain<SPACE_DIM>)
              (newGrids[indgrid] +
               ((newGrids[indgrid] + shift) * dProblem[h]));
          }
        }
      }

      Util::coarsen(newGrids, ref_ratio[h-1]);  // to level h-1

      foreach (indgrid, newGrids.domain())
        tagged[h-1] = tagged[h-1] + newGrids[indgrid];
    } // if (h > base)

    newGrids.destroy();
  } // for (int h = top; h >= base; h--)

  // This method is called only once.
  // After it's called, all you retain is returnGrids.

  return returnGrids;
}
