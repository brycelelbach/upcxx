#pragma once

/**
 * LevequeCorrection:
 * Objects in this class manage the setting of intermediate boundary
 * conditions in the operator-split method.
 *
 * A LevequeCorrection object is a field of LevelGodunov.
 * This class is used in LevelGodunov only.
 *
 * Given a patch, ghost cells are set as follows:
 * - If covered by another grid at the _same_ level, then copy.
 * - Otherwise:
 *   (1) interpolate from grids at next coarser level;
 *   (2) correct by performing a partial update corresponding to
 *       dimensional sweeps that have already been taken.
 *
 * The LevequeCorrection class is concerned with the correction in (2).
 * The correction is made using a piecewise constant extrapolation of the
 * flux differences from the nearest interior cell.
 *
 *      -------------+
 *                   |x
 *                   |x    <- do correction on these ghost cells
 *                   |x
 *        box        +-----------------
 *                   |
 *                   |   other box
 *                   |
 *      -------------+
 *                   |
 *
 *
 * @version  7 Aug 2000
 * @author   Peter McCorquodale, PWMcCorquodale@lbl.gov
 */

#include "BoxArray.h"
#include "LevelArray.h"
#include "Quantities.h"
#include "Util.h"

class LevequeCorrection {
private:
  /*
    fields that are arguments to constructor
  */

  BoxArray ba;

  /** physical domain */
  rectdomain<SPACE_DIM> dProblem;

  /** mesh spacing */
  double dx;

  int numFluxes;

  /*
    other fields that are set in constructor
  */

  /** indices of boxes in BoxArray ba */
  rectdomain<1> indices;

  /** for each box, the set of dimensions already swept */
  ndarray<domain<1>, 1> haveDims; // [indices]

  /** ghost cells indexed by dimension, direction, box index */
  ndarray<ndarray<ndarray<domain<SPACE_DIM>, 1>, 1>,
          1> ghosts; // [Util::DIMENSIONS][Util::DIRECTIONS][indices]

  /** corrections indexed by dimension, direction */
  ndarray<ndarray<LevelArray<Quantities>, 1>,
    1> correction; // [Util::DIMENSIONS][Util::DIRECTIONS]

public:
  LevequeCorrection() {}

  /**
   * constructor
   */
  LevequeCorrection(BoxArray &Ba, rectdomain<SPACE_DIM> DProblem,
                    double Dx, int NumFluxes);

  /*
    methods
  */

  /**
   * Reinitialize internal state.
   * Must be called prior to first use of object in LevelGodunov.step().
   */
  void clear() {
    foreach (pdim, Util::DIMENSIONS) {
      foreach (pdir, Util::DIRECTIONS) {
        foreach (indbox, indices) {
          correction[pdim][pdir][indbox].set(Quantities::zero());
        }
      }
    }

    foreach (indbox, indices) {
      haveDims[indbox] = RD(0, 0);
    }
  }

  /**
   * Increment registers corresponding to the part of the boundary
   * of ba[PatchIndex] in direction SweepDim.
   *
   * The increment is the divided difference of Flux in direction
   * SweepDim, multiplied by Dt.  The difference is taken across
   * the NON-ghost cell on the boundary.
   */
  void increment(ndarray<Quantities, SPACE_DIM> &Flux,
                 double Dt, point<1> PatchIndex, int sweepDim);

  /**
   * Apply correction to the state in the first row of cells
   * adjacent to ba[PatchIndex] in all of the normal directions
   * that have not yet been updated.  (LevequeCorrection keeps
   * track of that information internally.)
   */
  void correctBoundary(ndarray<Quantities, SPACE_DIM> &UPatch,
                       point<1> PatchIndex, int sweepDim) {
    /*
      UPatch.domain() ==
      stateBox:  ba[PatchIndex] + 1 (isotropic) + 3 (sweepDim)
    */
    foreach (pdir, Util::DIRECTIONS) {
      foreach (p, ghosts[sweepDim][pdir][PatchIndex]) {
        UPatch[p] =
          UPatch[p] + correction[sweepDim][pdir][PatchIndex][p];
      }
    }
  }
};
