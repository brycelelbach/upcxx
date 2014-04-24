#pragma once

/**
 * For internal use by MeshRefineD, which in turn is for internal use
 * by MeshRefine.
 * This class contains only uniprocessor code.
 *
 * Usage:  GridGenerator object must be created in a region.
 * GridGenerator gg = new (Region myRegion)
 *   GridGenerator(myRegion, domain<SPACE_DIM> tagged, domain<SPACE_DIM> pnd);
 * rectdomain<SPACE_DIM> [1d] grids = gg.makeGrids(myRegion);
 *
 * Given a set of tagged cells within a specified domain,
 * return an array of rectangles that cover the tagged cells
 * and are all contained in the domain.
 *
 * Input
 * domain<SPACE_DIM> tagged:  tagged cells
 * domain<SPACE_DIM> pnd:  proper nesting domain to contain all rectangles
 * tagged must be a subset of pnd.
 *
 * Output
 * rectdomain<SPACE_DIM> [1d] grids:  set of covering rectangles.
 *
 * Uses algorithm of Berger & Rigoutsos,
 * ``An Algorithm for Point Clustering and Grid Generation'',
 * IEEE Trans. Syst., Man, Cybern., vol. 21, pp. 1278--1286 (1991).
 *
 * @version  15 Dec 1999
 * @author   Peter McCorquodale, PWMcCorquodale@lbl.gov
 */

#include "boxedlist.h"
#include "Util.h"

class GridGenerator {
private:
  /*
    constants
  */

  /** minimum fraction of tagged cells for a rectangle to be accepted */
  static double MIN_FILL_RATIO;

  /** minimum inflection to split a rectangle */
  enum { MIN_INFLECTION = 3};

  /*
    non-constant fields
  */

  /** domain that must contain all rectangles returned */
  const domain<SPACE_DIM> &properDomain;

  /** domain of tagged cells */
  const domain<SPACE_DIM> &taggedDomain;

  /** list of rectangles to be returned */
  boxedlist<rectdomain<SPACE_DIM> > *recsGood;

public:
  /**
   * constructor
   */
  GridGenerator(const domain<SPACE_DIM> &tagged,
                const domain<SPACE_DIM> &pnd) :
    properDomain(pnd), taggedDomain(tagged) {
    // println("tagged:  " << tagged);
    // println("There are " << tagged.size() << " tagged cells");
  }

  ~GridGenerator() {
    delete recsGood;
  }

  /*
    methods
  */

  /**
   * Return a set of rectangles covering taggedDomain,
   * all contained in properDomain.
   */
  ndarray<rectdomain<SPACE_DIM>, 1> makeGrids() {
    // Initialize recsGood with an empty list.
    // println("getting recsGood");
    recsGood = new boxedlist<rectdomain<SPACE_DIM> >();

    // println("makeGridsRecur");
    makeGridsRecur(taggedDomain);

    // println("toArray");
    ndarray<rectdomain<SPACE_DIM>, 1> recsArray = recsGood->toArray();

    // Don't forget recsGood = new boxedlist<rectdomain<SPACE_DIM> >();
    delete recsGood;
    recsGood = NULL;
    // println("done with makeGrids");
    return recsArray;
  }

private:
  void makeGridsRecur(const domain<SPACE_DIM> &tagged);

  /**
   * Look for a hole in the tagged domain.
   * Split at the first hole in the first dimension where you find one.
   * Return true if split at hole, false if not.
   */
  bool cutAtHole(const domain<SPACE_DIM> &tagged,
                 ndarray<ndarray<int, 1>, 1> &sig,
                 point<SPACE_DIM> recMin, point<SPACE_DIM> recMax) {
    foreach (pdim, Util::DIMENSIONS) {
      int dim = pdim[1];
      for (int hole = recMin[dim]; hole <= recMax[dim]; hole++) {
        if (sig[dim][hole] == 0) {
          cut(tagged, dim, hole-1, hole+1);
          return true;
        }
      }
    }
    return false;
  }

  /**
   * Look for a strong enough inflection point (set MIN_INFLECTION).
   * Split at the point of greatest inflection.
   * Return true if split at inflection point, false if not.
   */
  bool cutAtInflection(const domain<SPACE_DIM> &tagged,
                       ndarray<ndarray<int, 1>, 1> &sig,
                       point<SPACE_DIM> recMin,
                       point<SPACE_DIM> recMax);

  /**
   * Find longest side, and split there.
   */
  void cutAtHalf(const domain<SPACE_DIM> &tagged,
                 ndarray<ndarray<int, 1>, 1> &sig,
                 point<SPACE_DIM> recMin, point<SPACE_DIM> recMax) {
    point<SPACE_DIM> lengths = recMax - recMin;
    int dimSplit = 1;
    for (int dim = 2; dim <= SPACE_DIM; dim++)
      if (lengths[dim] > lengths[dimSplit])
        dimSplit = dim;
    int splitpoint = (recMin[dimSplit] + recMax[dimSplit]) / 2;

    cut(tagged, dimSplit, splitpoint, splitpoint + 1);
  }

  /**
   * Cut the domain tagged along the dimension dim,
   * with the first part ending at coordinate end1 and the second
   * part beginning at coordinate start2.
   */
  void cut(const domain<SPACE_DIM> &tagged, int dim, int end1,
           int start2) {

    point<SPACE_DIM> tagMin = tagged.min(), tagMax = tagged.max();
    point<SPACE_DIM> unit = point<SPACE_DIM>::direction(dim);
    point<SPACE_DIM> wtlim = Util::ONE - unit;

    point<SPACE_DIM> pend1 = wtlim * tagMax + unit * end1;
    point<SPACE_DIM> pstart2 = wtlim * tagMin + unit * start2;

    makeGridsRecur(tagged * RD(tagMin, pend1+Util::ONE));
    makeGridsRecur(tagged * RD(pstart2, tagMax+Util::ONE));
  }

  /**
   * Compute signature in each dimension for the domain of tagged cells:
   * this is the number of tagged cells in each cross-section.
   */
  ndarray<ndarray<int, 1>, 1> signature(const domain<SPACE_DIM> &tagged);
};
