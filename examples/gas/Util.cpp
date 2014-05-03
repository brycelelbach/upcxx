#include "Util.h"

/*
  constants
*/

rectdomain<1> Util::DIMENSIONS = RD(1, SPACE_DIM+1);
rectdomain<1> Util::DIRECTIONS = RD(-1, 2, 2);

point<SPACE_DIM> Util::ZERO = point<SPACE_DIM>::all(0);
point<SPACE_DIM> Util::ONE = point<SPACE_DIM>::all(1);
point<SPACE_DIM> Util::MINUS_ONE = point<SPACE_DIM>::all(-1);

rectdomain<SPACE_DIM> Util::EMPTY = rectdomain<SPACE_DIM>();

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
ndarray<point<SPACE_DIM>, 1> Util::cell2edge;

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
ndarray<point<SPACE_DIM>, 1> Util::edge2cell;

// Convention:
// cell      1 2 3 4
// edge     0 1 2 3 4

void Util::set() {
  cell2edge = ndarray<point<SPACE_DIM>, 1>(RD(-SPACE_DIM,
                                              +SPACE_DIM+1));
  edge2cell = ndarray<point<SPACE_DIM>, 1>(RD(-SPACE_DIM,
                                              +SPACE_DIM+1));
  foreach (pdim, DIMENSIONS) {
    int dim = pdim[1];

    cell2edge[-dim] = point<SPACE_DIM>::direction(-dim);
    edge2cell[+dim] = point<SPACE_DIM>::direction(+dim);

    cell2edge[+dim] = ZERO;
    edge2cell[-dim] = ZERO;
  }
}
