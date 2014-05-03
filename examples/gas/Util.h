#pragma once

/**
 * Utility methods and data for use with grids.
 */

#include "globals.h"

class Util {
public:
  /*
    constants
  */

  static rectdomain<1> DIMENSIONS;
  static rectdomain<1> DIRECTIONS;

  static point<SPACE_DIM> ZERO;
  static point<SPACE_DIM> ONE;
  static point<SPACE_DIM> MINUS_ONE;

  static rectdomain<SPACE_DIM> EMPTY;

  /**
   * cell2edge:
   * If  c  is a CELL, then
   * c + cell2edge[dir * dim]
   * is the EDGE in dimension  dim  (1:SPACE_DIM) in
   * direction  dir (-1, 1) from  c.
   *
   * If  A  is a cell-centered grid, then
   * B = A.translate(cell2edge[dir * dim])
   * will produce an edge-centered grid with
   * B[c + cell2edge[dir * dim]] = A[c].
   */
  static ndarray<point<SPACE_DIM>, 1> cell2edge;

  /**
   * edge2cell:
   * If  e  is an EDGE in dimension  dim  (1:SPACE_DIM), then
   * e + edge2cell[dir * dim]
   * is the CELL in direction  dir (-1, 1) from  e.
   *
   * If  A  is an edge-centered grid, then
   * B = A.translate(edge2cell[dir * dim])
   * will produce a cell-centered grid with
   * B[e + edge2cell[dir * dim]] = A[e].
   */
  static ndarray<point<SPACE_DIM>, 1> edge2cell;

  // Convention:
  // cell      1 2 3 4
  // edge     0 1 2 3 4

  static void set();

  /**
   * Return domain for inner edges of a box in a particular dimension.
   * According to our convention, this means removing the last row.
   */
  static rectdomain<SPACE_DIM> innerEdges(rectdomain<SPACE_DIM> box,
                                          int dim) {
    return box.shrink(1, +dim);
  }


  /**
   * Return domain for outer edges of a box in a particular dimension.
   * According to our convention, this means appending the first row.
   */
  static rectdomain<SPACE_DIM> outerEdges(rectdomain<SPACE_DIM> box,
                                          int dim) {
    return box.accrete(1, -dim);
  }


  /** Return indices of subcells of cellC at the next finer level. */
  static rectdomain<SPACE_DIM> subcells(point<SPACE_DIM> cellC,
                                        int nRefine) {
    rectdomain<SPACE_DIM> subBase(ZERO, point<SPACE_DIM>::all(nRefine));
    return (subBase + (cellC * nRefine));
  }


  /** Refine a rectdomain by a given ratio. */
  static rectdomain<SPACE_DIM> refine(rectdomain<SPACE_DIM> grid,
                                      int nRefine) {
    point<SPACE_DIM> nRefineP = point<SPACE_DIM>::all(nRefine);
    return RD(grid.min() * nRefineP, (grid.max() + ONE) * nRefineP);
  }


  /** Coarsen a rectdomain by a given ratio. */
  static rectdomain<SPACE_DIM> coarsen(rectdomain<SPACE_DIM> grid,
                                       int nRefine) {
    point<SPACE_DIM> nRefineP = point<SPACE_DIM>::all(nRefine);
    return (grid / nRefineP);
  }


  /** Coarsen a Domain by a given ratio. */
  static domain<SPACE_DIM> coarsen(domain<SPACE_DIM> dom,
                                   int nRefine) {
    point<SPACE_DIM> nRefineP = point<SPACE_DIM>::all(nRefine);
    return (dom / nRefineP);
  }


  /** Refine all rectdomains in an array by the same ratio. */
  static void refine(ndarray<rectdomain<SPACE_DIM>, 1> grids,
                     int nRefine) {
    point<SPACE_DIM> nRefineP = point<SPACE_DIM>::all(nRefine);
    foreach (ind, grids.domain())
      grids[ind] = RD(grids[ind].min() * nRefineP,
                      (grids[ind].max() + ONE) * nRefineP);
  }


  /** Coarsen all rectdomains in an array by the same ratio. */
  static void coarsen(ndarray<rectdomain<SPACE_DIM>, 1> grids,
                      int nRefine) {
    point<SPACE_DIM> nRefineP = point<SPACE_DIM>::all(nRefine);
    foreach (ind, grids.domain())
      grids[ind] = grids[ind] / nRefineP;
  }


  /** Coarsen all Domains in an array by the same ratio. */
  static void coarsen(ndarray<domain<SPACE_DIM>, 1> doms,
                      int nRefine) {
    point<SPACE_DIM> nRefineP = point<SPACE_DIM>::all(nRefine);
    foreach (ind, doms.domain())
      doms[ind] = doms[ind] / nRefineP;
  }
};
