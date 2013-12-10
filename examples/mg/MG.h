#pragma once

#include "profiling.h"
#include "globals.h"
#include "constants.h"
#include "MGDriver.h"
#include "Grid.h"
#include "../cg/Timer.h"

// first index in myTimes and myCounts (only first 5 used in myCounts)
#define T_L2NORM 1
#define T_APPLY_SMOOTHER 2
#define T_EVALUATE_RESIDUAL 3
#define T_COARSEN 4
#define T_PROLONGATE 5
#define T_GLOBAL_COMM 6
#define T_THRESHOLD_COMM 7
#define T_BARRIER 8
#define T_PACKING 9
#define T_UNPACKING 10
#define T_SYNC 11

// SECOND INDEX: grid level

/**
 * This class contains all the basic multigrid operators.
 * <p>
 * The potential optimizations (listed in "globals.h") for parallel MG
 * are:
 * <p>
 * Modifying the THRESHOLD_LEVEL.  This is defined to be the
 * multigrid level below which everything is stored on
 * processor 0 instead of distributed across all processors-
 * this helps in eliminating small message communication.
 * <p>
 * Modifying the UPDATE_BORDER_DIRECTIONS.  This can be either
 * 6 or 26 directions.  If communicating with 6 nearest
 * neighbors, three barriers are required to synchronize
 * the communication in each dimension properly.  If
 * communicating with 26 nearest neighbors, much more data
 * is sent, but only one barrier is needed.
 * <p>
 * Modify the PUSH_DATA flag.  If this is defined, data
 * is pushed from each processor.  If not defined, each
 * processor pulls data instead.
 * <p>
 * Use CONTIGUOUS_ARRAY_BUFFERS.  This flag uses buffers to
 * communicate with each block's nearest neighbors so that
 * copied arrays are stored contiguously in memory both when
 * sending and receiving.  However, extra work is involved
 * in doing local copies to/from these buffers to the
 * appropriate border cells.
 * <p>
 * Turning on NONBLOCKING_ARRAYCOPY.  This flag enables
 * a processor to overlap computation with communication.
 * Currently, the CONTIGUOUS_ARRAY_BUFFERS flag is required
 * for this functionality to work.
 */
class MG {
 private:
  // the following are copied from MGDriver
  int startLevel;
  int numIterations;

 public:
  // profiling tools
  int numTimers, numCounters;
  Timer myTimer;
  // "myTimes" is indexed by (MG Component #), level, and timing number
  ndarray<ndarray<ndarray<double, 1>, 1>, 1> myTimes;
#ifdef COUNTERS_ENABLED
  PAPICounter myCounter;
  // "myCounts" is indexed by (MG Component #), level, and counting number
  ndarray<ndarray<ndarray<long, 1>, 1>, 1> myCounts;
#endif

  /**
   * Constructor- initializes all timers and counters
   */
  MG();

  /**
   * Returns the L2 norm of gridA.
   *
   * @param gridA  The grid for which the L2 norm is being calculated
   * @param callNumber Used for profiling
   * @return The L2 norm of gridA
   */
  double getL2Norm(Grid &gridA, int callNumber);

  /**
   * Used in updateBorder()
   */
  int mod(int a, int b);

  /**
   * Updates the ghost cells surrounding each block in gridA. For
   * block points on a face of the grid cube, periodic boundary
   * conditions are used. For other points on a face of a block, the
   * correct values from the neighboring block are used for updating.
   *
   * @param gridA The grid for which the borders are being updated
   * @param level The multigrid level of the grid arguments
   * @param iterationNum The number of the iteration
   * @param callNumber Used for profiling
   */
  void updateBorder(Grid &gridA, int level, int iterationNum, int callNumber);
    
  /**
   * Applies the smoother to gridA according to the formula (gridA =
   * gridA + S(gridB)), where S is the smoother operator.
   * 
   * @param gridA The grid that is being updated
   * @param gridB The input grid that the smoother is being applied to
   * @param level The multigrid level of the grid arguments
   * @param iterationNum Used for profiling
   */
  void applySmoother(Grid &gridA, Grid &gridB, int level, int iterationNum);

  /**
   * Updates gridC according to the formula (gridC = gridA - A(gridB)),
   * where A is the residual operator.
   * 
   * @param gridA One of the input grids
   * @param gridB The input grid that the residual is being calculated for
   * @param gridC The calculated residual grid
   * @param level The multigrid level of the grid arguments
   * @param iterationNum Used for profiling
   * @param callNumber Used for profiling
   */
  void evaluateResidual(Grid &gridA, Grid &gridB, Grid &gridC, int level,
                        int iterationNum, int callNumber);

  /**
   * Coarsens gridA according to the formula (gridB = P(gridA)),
   * where P is the coarsening operator.
   * 
   * @param gridA The grid that will be coarsened
   * @param gridB The resulting coarsened grid
   * @param level The multigrid level for gridA
   * @param iterationNum Used for profiling
   */
  void coarsen(Grid &gridA, Grid &gridB, int level, int iterationNum);

  /**
   * Prolongates gridA according to the formula (gridB = gridB + Q(gridA)),
   * where Q is the prolongating operator.
   * 
   * @param gridA The grid that will be prolongated
   * @param gridB The resulting prolongated grid
   * @param level The multigrid level of gridB
   * @param iterationNum Used for profiling
   */
  void prolongate(Grid &gridA, Grid &gridB, int level, int iterationNum);
    
  /**
   * Performs one multigrid V-cycle.  This is the heart of multigrid.
   * 
   * @param residualGrids An array of the residual grids at each
   *        multigrid level
   * @param correctionGrids An array of the correction grids at each
   *        multigrid level
   * @param rhsGrid The original right-hand side grid
   * @param iterationNum Used for profiling
   */
  void vCycle(ndarray<Grid *, 1> residualGrids,
              ndarray<Grid *, 1> correctionGrids,
              Grid &rhsGrid, int iterationNum);

  /**
   * Resets the performance profiling summary.
   */
  void resetProfile();

  /**
   * Prints a performance profiling summary.
   */
  void printSummary();

  /**
   * Prints a full performance profile.
   */
  void printProfile();
};
