#pragma once

/**
 * Use MeshRefine.refine() to return a new set of rectangles given
 * tagged domains at a number of levels.
 *
 * @version  7 Feb 2000
 * @author   Peter McCorquodale, PWMcCorquodale@lbl.gov
 */

#include "Amr.h"
#include "Logger.h"
#include "MeshRefineD.h"
#include "Util.h"

class MeshRefine {
  /** process used for broadcasts */
  enum { SPECIAL_PROC = 0 };

  enum { COARSENING_RATIO = 2 };

public:
  /**
   * Returns a new array of arrays of rectdomains, indexed by levels.
   *
   * Given an array of domains of tagged cells, indexed by [base:top],
   * the returned array of arrays of rectdomain is indexed by [base+1:top+1].
   * Need Amr object for boxes, physical domains and refinement ratios.
   * @param myRegion region to contain the result
   * @param amrAll parent Amr object
   * @param tagged array of tagged domains at different levels
   */
  static ndarray<ndarray<rectdomain<SPACE_DIM>, 1>, 1>
  refine(Amr *amrAll, ndarray<domain<SPACE_DIM>, 1> &tagged);
};
