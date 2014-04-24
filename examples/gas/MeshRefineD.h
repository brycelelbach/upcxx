#pragma once

/**
 * For internal use by MeshRefine.
 * This class contains only uniprocessor code.
 *
 * MeshRefineD mrd(ndarray<ndarray<rectdomain<SPACE_DIM>, 1>,
 *                 1> &oldGrids,
 *                 ndarray<domain<SPACE_DIM>, 1> &Tagged,
 *                 rectdomain<SPACE_DIM> DProblemBase,
 *                 ndarray<int, 1> &Ref_Ratio);
 *
 * ndarray<ndarray<rectdomain<SPACE_DIM>, 1>,
 *         1> newgrids = mrd.refine();
 *
 * @version  13 Dec 1999
 * @author   Peter McCorquodale, PWMcCorquodale@lbl.gov
 */

#include "GridGenerator.h"
#include "Util.h"

class MeshRefineD {
private:
  /*
    non-constant fields
  */

  int base, top;
  ndarray<domain<SPACE_DIM>, 1> ProperNestingDomain;  // [base:top]
  ndarray<rectdomain<SPACE_DIM>, 1> dProblem;         // [base:top]
  ndarray<int, 1> ref_ratio;                          // [base:top]
  ndarray<domain<SPACE_DIM>, 1> tagged;               // [base:top]

public:
  /**
   * Constructor:
   * Given grids at levels base:top
   * and domains containing tagged cells at levels base:top.
   */
  MeshRefineD(ndarray<ndarray<rectdomain<SPACE_DIM>, 1>,
              1> &oldGrids,                           // [base:top][1d]
              ndarray<domain<SPACE_DIM>, 1> &Tagged,  // [base:top]
              rectdomain<SPACE_DIM> DProblemBase,     // at base
              ndarray<int, 1> &Ref_Ratio) {           // [base:top]
    rectdomain<1> levels = oldGrids.domain();
    base = levels.min()[1];
    top = levels.max()[1];
    ref_ratio = Ref_Ratio;
    tagged = Tagged;

    getProblemDomains(DProblemBase);

    // (1) Compute proper nesting domains for levels base:top
    getPNDs(oldGrids);

    // (2) Turn off tags at points outside PND.
    clipTagged(tagged);
  }

  ~MeshRefineD() {
    ProperNestingDomain.destroy();
    dProblem.destroy();
  }

  /**
   * Return arrays of disjoint rectangles indexed by [base+1:top+1]
   * such that the union of the disjoint rectangles indexed by [level+1]
   * covers the domains at level [level].
   */
  ndarray<ndarray<rectdomain<SPACE_DIM>, 1>, 1> refine();

private:
  /**
   * Find physical domain for each level.
   */
  void getProblemDomains(rectdomain<SPACE_DIM> DProblemBase) {
    dProblem.create(RD(base, top+1));
    Point<SPACE_DIM> extent = DProblemBase.max() + Util::ONE;
    for (int h = base; h <= top; h++) {
      dProblem[h] = RD(Util::ZERO, extent);
      extent = extent * Point<SPACE_DIM>::all(ref_ratio[h]);
    }
  }

  /**
   * Compute proper nesting domains.
   *
   * From Chombo:
   * The proper nesting domain (PND) of level h+1 is defined as the
   * interior of the PND for level h (the next coarser) except at the
   * boundaries of the problem domain (ie, the level 0 box), in which case
   * the PNDs of levels h and h+1 both contain the boundary.  The PND of
   * the base level is defined as the interior of the union of all the grid
   * boxes in the base level.  Given ProperNestingDomain[h], this function
   * computes ProperNestingDomain[h+1] by iterating over the coordinate
   * directions and computing the cells to remove from the proper nesting
   * domain due to the boundaries in that direction.  For each direction
   * the domain is shifted (first up, then down) and only the part that is
   * shifted _out_ of the PND but remains _inside_ the problem boundary is
   * kept and then shifted back into the PND and subtracted (removed) from
   * it.  So if the shift moves part of the PND outside the problem
   * boundary then no cells will be removed.  If the cells that get shifted
   * out of the PND remain inside the boundary then they will be removed
   * from the PND when they get shifted back.  When all the directions have
   * been handled, the PND is refined to make it correct for level h+1.
   * The PND of the base level is the union of the mesh boxes in the base
   * level and serves as the starting point.  In set notation the operation
   * is:
   *
   *       D - ( ( ( (D << d) - D ) * B ) >> d )
   *
   * where:
   *  d is the current direction (+i,-i,+j,etc)
   *  B is the boundary of the problem domain at the current level
   *  - is the subtraction (removal) operator
   *  * is the intersection operator
   * << is the down-shift operator (shift in negative direction)
   * >> is the up-shift operator (shift in positive direction)
   *
   * In the following graphic, the PND for two mesh boxes with BufferSize=1
   * is shown.
   *
   *      ###################################################
   *      #     |   |   |   |   |   |                       #
   *      #     |   | o | o | o |   |                       #
   *      #     |   |   |   |   |   |                       #
   *      #     +---+---+---+---+---%---+---+---+---+---+---#
   *      #     |   |   |   |   |   %   |   |   |   |   |   #
   *      #     |   | o | o | o |   %   |   |   |   |   |   #
   *      #     |   |   |   |   |   %   |   |   |   |   |   #
   *      #     +---+---+---+---+---%---+---+---+---+---+---#
   *      #     |   |   |   |   |   %   |   |   |   |   |   #
   *      #     |   | o | o | o | o % o | o | o | o | o | o #
   *      #     |   |   |   |   |   %   |   |   |   |   |   #
   *      #     +---+---+---+---+---%---+---+---+---+---+---#
   *      #     |   |   |   |   |   %   |   |   |   |   |   #
   *      #     |   |   |   |   |   %   | o | o | o | o | o #
   *      #     |   |   |   |   |   %   |   |   |   |   |   #
   *      #     +---+---+---+---+---%---+---+---+---+---+---#
   *      #                         |   |   |   |   |   |   #
   *      #                         |   | o | o | o | o | o #
   *      #                         |   |   |   |   |   |   #
   *      ###################################################
   *
   *  where:
   *   #    are physical boundaries
   *   | +  are cell boundaries of the two mesh boxes
   *   %    are box boundaries shared between boxes
   *   o    are cells in the proper nesting domain
   *
   */
  void getPNDs(ndarray<ndarray<rectdomain<SPACE_DIM>, 1>,
               1> &oldGrids);

  /**
   * At each level, remove points outside proper nesting domain
   * from the tagged domain.
   */
  void clipTagged(ndarray<domain<SPACE_DIM>, 1> &tagged) {
    foreach (h, tagged.domain()) {
      // println("Before clipping, tagged" << h << " = " << tagged[h]);
      // domain<SPACE_DIM> tagged_before = tagged[h];
      tagged[h] *= ProperNestingDomain[h];
      // println("After  clipping, tagged" << h << " = " << tagged[h]);
      // println("Clipped out " << (tagged_before - tagged[h]));
    }
  }
};
