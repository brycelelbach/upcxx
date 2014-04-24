#pragma once

#include "globals.h"

/**
 * This class represents the Grid object that the data is stored in.
 */
class Grid {
 public:
  // "points" array first indexed by position in 3D proc grid, then
  // individual point (in global coords)
  ndarray<ndarray<double, 3, global GUNSTRIDED>, 3 UNSTRIDED> points;

  // used for grids at level THRESHOLD_LEVEL only
  ndarray<ndarray<double, 3, global GUNSTRIDED>, 3 UNSTRIDED> localBuffersOnProc0; // used only by proc 0
  ndarray<double, 3, global GUNSTRIDED> myRemoteBufferOnProc0; // pointers from other procs to "localBufferOnProc0"
  ndarray<double, 3 UNSTRIDED> myBufferForProc0;

#ifdef CONTIGUOUS_ARRAY_BUFFERS
  // these buffers are indexed by:
  // 1.  3D proc grid point
  // 2. direction of buffer (a 1D point for 6 directions, a 3D point
  //    for 26 directions)
  // 3.  actual point (in global coords)
#if UPDATE_BORDER_DIRECTIONS == 6
  ndarray<ndarray<ndarray<double, 3, global>, 1, global GUNSTRIDED>,
          3 UNSTRIDED> outBuffers, inBuffers;
#elif UPDATE_BORDER_DIRECTIONS == 26
  ndarray<ndarray<ndarray<double, 3, global>, 3, global GUNSTRIDED>,
          3 UNSTRIDED> outBuffers, inBuffers;
#endif
#endif

  static Point<3> numCellsPerBlockSide;  // the number of cells in each block dim
  static Point<3> numBlocksInGridSide;   // the number of blocks in each grid dim
  static Point<3> myBlockPos;            // this block's position

  /**
   * Creates a distributed array that represents a cubic grid. Note
   * that if the level is >= THRESHOLD_LEVEL, the distributed array
   * spreads cells across processors. Otherwise, all cells are placed
   * on processor 0.
   * @param level The multigrid level that this grid will represent
   * @param ghostCellsNeeded Bool that indicates whether ghost cells
   *        will be included in the Grid
   * @param proc0BuffersNeeded
   */
  Grid(int level, bool ghostCellsNeeded, bool proc0BuffersNeeded);

  /**
   * Initializes all the static variables for this class. This is called
   * before the Grid constructor.
   */
  static void init();

  /**
   * Given the Point "numBlocksInGridSide" (already set), this method
   * will return a unique int between 0 and (number of blocks - 1) for
   * each Point "blockPos" that is passed in.
   */
  static int procForBlockPosition(Point<3> blockPos) {
    return (blockPos[1] * numBlocksInGridSide[2] * numBlocksInGridSide[3] +
            blockPos[2] * numBlocksInGridSide[3] +
            blockPos[3]);
  }

  /**
   * Given the Point "pointPosition", this method will return the block
   * position that this point is located in.
   */
  static Point<3> blockPosForPoint(Point<3> pointPosition) {
    return (pointPosition/numCellsPerBlockSide);
  }

  /**
   * Zero out the grid
   */
  void zeroOut() {
    ndarray<double, 3 UNSTRIDED> myPoints =
      (ndarray<double, 3 UNSTRIDED>) points[myBlockPos];
    // foreach (p, myPoints.domain()) {
    //   myPoints[p] = 0.0;
    // }
    myPoints.set(0.0);
  }
};
