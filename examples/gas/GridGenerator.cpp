#include <cstdlib>
#include "GridGenerator.h"

double GridGenerator::MIN_FILL_RATIO = 0.7;

void GridGenerator::makeGridsRecur(const domain<SPACE_DIM> &tagged) {
  // println("get rec");
  rectdomain<SPACE_DIM> rec = tagged.bounding_box();
  // println("bounding box = " << rec);
  point<SPACE_DIM> recMin = rec.min(), recMax = rec.max();

  // Check if rectangle is full enough.
  // println("fillRatio " << tagged.size() << " / " << rec.size());
  double fillRatio = (double) tagged.size() / (double) rec.size();

  // Need to check also that rectangle does not contain points outside
  // properDomain.
  if (fillRatio >= MIN_FILL_RATIO && (rec - properDomain).is_empty()) {
    // Full enough.  We'll use it.
    // println("push " << rec << " ratio " << tagged.size() <<
    //         " / " << rec.size() << " = " << fillRatio);
    recsGood->push(rec);
    point<SPACE_DIM> ext = recMax - recMin + Util::ONE;
  } else {
    // Compute signatures and then cut somewhere.
    // println("get sig");

    ndarray<ndarray<int, 1>, 1> sig = signature(tagged);
    // println("cut");
    if (!cutAtHole(tagged, sig, recMin, recMax))
      if (!cutAtInflection(tagged, sig, recMin, recMax))
        cutAtHalf(tagged, sig, recMin, recMax);

    foreach (p, sig.domain())
      sig[p].destroy();
    sig.destroy();
  }
}

bool GridGenerator::cutAtInflection(const domain<SPACE_DIM> &tagged,
                                    ndarray<ndarray<int, 1>, 1> &sig,
                                    point<SPACE_DIM> recMin,
                                    point<SPACE_DIM> recMax) {
  ndarray<bool, 1> existInfl(Util::DIMENSIONS);
  bool existInflAny = false;
  ndarray<int, 1> bestLoc(Util::DIMENSIONS);
  ndarray<int, 1> bestVal(Util::DIMENSIONS);
  bool result = false;
  foreach (pdim, Util::DIMENSIONS) {
    int dim = pdim[1];
    rectdomain<1> Ddiff(recMin[dim] + 1, recMax[dim] - 1);
    existInfl[pdim] = false;
    if (!Ddiff.is_empty()) {
      bestVal[pdim] = 0;
      ndarray<int, 1> sigdim = sig[pdim];
      foreach (p, Ddiff) {
        int diffLaplacian =
          abs(sigdim[p + PT(2)] - 3 * sigdim[p + PT(1)]
              + 3 * sigdim[p] - sigdim[p - PT(1)]);
        if (diffLaplacian >= MIN_INFLECTION) {
          existInfl[pdim] = true;
          existInflAny = true;
          if (diffLaplacian > bestVal[pdim]) {
            bestVal[pdim] = diffLaplacian;
            bestLoc[pdim] = p[1];
          }
        }
      }
    }
  }
  if (existInflAny) {
    // Find best inflection point, and split there.
    int dimSplit = 1;
    for (int dim = 2; dim <= SPACE_DIM; dim++)
      if (existInfl[dim] && bestVal[dim] > bestVal[dimSplit])
        dimSplit = dim;

    cut(tagged, dimSplit, bestLoc[dimSplit], bestLoc[dimSplit] + 1);
    result = true;
  }
  existInfl.destroy();
  bestLoc.destroy();
  bestVal.destroy();
  return result;
}

ndarray<ndarray<int, 1>,
        1> GridGenerator::signature(const domain<SPACE_DIM> &tagged) {
  ndarray<ndarray<int, 1>, 1> sig(Util::DIMENSIONS);
  // sig[1:SPACE_DIM] returns an array of int counting the number of
  // filled-in cells in each cross-section.

  point<SPACE_DIM> tagMin = tagged.min();
  point<SPACE_DIM> tagMax = tagged.max();
  foreach (pdim, Util::DIMENSIONS) {
    int dim = pdim[1];
    rectdomain<1> tagProj(tagMin[dim], tagMax[dim]+1);
    sig[dim].create(tagProj);
    foreach (p1, tagProj)
      sig[dim][p1] = 0;
  }

  foreach (p, tagged) {
    foreach (pdim, Util::DIMENSIONS)
      sig[pdim][p[pdim[1]]]++;
  }

  return sig;
}
