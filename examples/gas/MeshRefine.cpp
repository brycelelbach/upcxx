#include "MeshRefine.h"

ndarray<ndarray<rectdomain<SPACE_DIM>, 1>, 1>
MeshRefine::refine(Amr *amrAll,
                   ndarray<domain<SPACE_DIM>, 1> &tagged) {
  rectdomain<1> levinds = tagged.domain();
  rectdomain<1> levindsnew = broadcast(levinds+PT(1), SPECIAL_PROC);
  int base = levinds.min()[1], top = levinds.max()[1];
  ndarray<ndarray<rectdomain<SPACE_DIM>, 1>, 1> boxes(levindsnew);

  ndarray<ndarray<rectdomain<SPACE_DIM>, 1>, 1> newGrids;

  // println("going to SPECIAL_PROC");
  if (MYTHREAD == SPECIAL_PROC) {

    /*
      General plan:

      Coarsen oldGrids[base:top] by 2.
      Then run MeshRefineD with
      dProblemBase = dProblem[base] / 2, and ref_ratio[base:top],
      to get newGrids[base+1:top+1].
      Then refine newGrids[base+1:top+1] by 2.
    */

    // refinement ratio between this and next finer level
    // $4 = (int [1d]) 0xefffecd8 int[[[-279264380]:[-279264381]:[-279264380]]]
    // println("levinds = " << levinds);
    rectdomain<1> levindsAgain = levinds;
    // println("levindsAgain = " << levindsAgain);
    ndarray<int, 1> ref_ratio(levindsAgain);
    // println("ref_ratio domain size = " << ref_ratio.domain().size());
    foreach (h, levindsAgain)
      ref_ratio[h] = amrAll->getRefRatio(h);

    // physical domain at base level
    rectdomain<SPACE_DIM> dProblemBase = amrAll->getDomain(base);
    // println("base domain = " << dProblemBase);

    // copy existing boxes to oldGrids
    ndarray<ndarray<rectdomain<SPACE_DIM>, 1>,
            1> oldGrids(levindsAgain);
    foreach (h, levindsAgain) {
      ndarray<rectdomain<SPACE_DIM>, 1> levelBoxes =
        amrAll->getLevelNonsingle(h)->getBa().boxes();
      oldGrids[h].create(levelBoxes.domain());
      foreach (indbox, levelBoxes.domain())
        oldGrids[h][indbox] = levelBoxes[indbox];
    }

    /*
      Coarsen domains by COARSENING_RATIO.
    */

    dProblemBase = Util::coarsen(dProblemBase, COARSENING_RATIO);
    // println("coarsened base domain = " << dProblemBase);

    foreach (h, levindsAgain)
      Util::coarsen(oldGrids[h], COARSENING_RATIO);
    Util::coarsen(tagged, COARSENING_RATIO);

    /*
      Use MeshRefineD to retrieve newGrids.
    */

    // println("getting MeshRefineD object");
    MeshRefineD mrd(oldGrids, tagged, dProblemBase, ref_ratio);

    // println("refining MeshRefineD object");
    newGrids = mrd.refine();

    /*
      Refine newGrids by COARSENING_RATIO.
    */

    // println("refineBoxes");
    foreach (h, levindsnew)
      Util::refine(newGrids[h], COARSENING_RATIO);

    ref_ratio.destroy();
    foreach (p, oldGrids.domain())
      oldGrids[p].destroy();
    oldGrids.destroy();
  } // if (Ti.thisProc() == SPECIAL_PROC)

  Logger::barrier();
  Logger::append("got new grids");

  rectdomain<1> srcrdd;
  rectdomain<SPACE_DIM> srcrd;
    
  foreach (h, levindsnew) {
    if (MYTHREAD == SPECIAL_PROC)
      srcrdd = newGrids[h].domain();
    rectdomain<1> boxesIndices = broadcast(srcrdd, SPECIAL_PROC);
    boxes[h].create(boxesIndices);
    foreach (indbox, boxesIndices) {
      if (MYTHREAD == SPECIAL_PROC)
        srcrd = newGrids[h][indbox];
      boxes[h][indbox] = broadcast(srcrd, SPECIAL_PROC);
    }
  }

  newGrids.destroy();
  // println("done with MeshRefine");
  return boxes;
}
