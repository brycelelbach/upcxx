#pragma once

/**
 * PieceWise Linear Fill Patch.
 * Lives on LevelGodunov.
 *
 * @version  9 Aug 2000
 * @author   Peter McCorquodale, PWMcCorquodale@lbl.gov
 */

#include "BoxArray.h"
#include "LevelArray.h"
#include "Quantities.h"
#include "Util.h"

class PWLFillPatch {
private:
  /*
    constants
  */

  /** array of boxes at this level and at the next coarser level */
  BoxArray ba, baCoarse;

  /** physical domain */
  rectdomain<SPACE_DIM> dProblem;

  int nRefine, innerRadius, outerRadius;

  point<SPACE_DIM> nRefineP;

  double dRefine;

  ndarray<domain<1>, 1> adjacentBoxes, adjacentCoarseBoxes,
    coarserBoxes;

  /** box within inner radius (isotropically) */
  ndarray<rectdomain<SPACE_DIM>, 1> BInner;

  /** box within outer radius (isotropically) */
  ndarray<rectdomain<SPACE_DIM>, 1> BOuter;

public:
  PWLFillPatch() {}

  /**
   * constructor
   */
  PWLFillPatch(BoxArray &Ba, BoxArray &BaCoarse,
               rectdomain<SPACE_DIM> DProblem, int NRefine,
               int InnerRadius, int OuterRadius);

  // methods

  rectdomain<SPACE_DIM> innerBox(point<1> PatchIndex) {
    return BInner[PatchIndex];
  }


  /**
   * Fill in UPatch, by using patches from U as much as possible,
   * and also interpolating from patches at next coarser level
   * within the inner radius.
   *
   * @param U (cell) patches at this level
   * @param UCOld (cell) patches at next coarser level at old time
   * @param UCNew (cell) patches at next coarser level at new time
   * @param UPatch (cell) patch to be filled
   * @param PatchIndex index in U of the core of UPatch
   */
  void fill(LevelArray<Quantities> &U, LevelArray<Quantities> &UCOld,
            LevelArray<Quantities> &UCNew,
            ndarray<Quantities, SPACE_DIM> &UPatch,  // Patch to be filled
            point<1> PatchIndex, double Time, double TCOld,
            double TCNew);

private:
  ndarray<Quantities, 1> coarseSlopes(LevelArray<Quantities> &UCOld,
                                      LevelArray<Quantities> &UCNew,
                                      point<1> indc,
                                      point<SPACE_DIM> cellC,
                                      double wtOld, double wtNew);

public:
  /**
   * Fill unfilled cells of UPatch by copying from BInner[PatchIndex].
   */
  void extend(ndarray<Quantities, SPACE_DIM> &UPatch,
              point<1> PatchIndex, int Dim, int FlapRadius);

  /**
   * Fill UNew by copying from UTemp and interpolating from UTempCoarse.
   *
   * @param UNew (cell) new patches at this level, to be filled
   * @param UTemp (cell) old patches at this level
   * @param UTempCoarse (cell) old patches at next coarser level
   */
  void regrid(LevelArray<Quantities> &UNew,
              LevelArray<Quantities> &UTemp,
              LevelArray<Quantities> &UTempCoarse);
};
