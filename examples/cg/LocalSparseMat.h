#pragma once

#include "globals.h"
#include "Util.h"

/*  This class represents a local sparse matrix stored in CSR format.  It contains
    a number of rows of the full sparse matrix, which is represented in the
    SparseMat object as an array of LocalSparseMat objects. */

class LocalSparseMat {
 private:
  // all of the following use global coordinates and are zero-based
  ndarray<RectDomain<1>, 1 UNSTRIDED> rowRectDomains;  // generated from rowptr, indexed by row
  ndarray<int, 1 UNSTRIDED> colidx;                    // column indices
  ndarray<double, 1 UNSTRIDED> a;                      // sparse matrix values
  int N;                                     // number of columns in this sparse matrix
  int procRowPos, procColPos;                // position within processor grid
  int procRowSize;                           // number of rows in a processor row

 LocalSparseMat(int paramN, int numProcRows, int numProcCols)
   : rowRectDomains(ndarray<RectDomain<1>, 1 UNSTRIDED>(RD(Util::rowStart,
                                                           Util::rowEnd+1))),
    N(paramN), procRowPos(Util::procRowPos), procColPos(Util::procColPos),
    procRowSize(Util::procRowSize) {}

 public:
  /*  This populates the local arrays above (which are zero-based) from the passed
      arrays (which are one-based).  "l" in the passed arrays means local.
      A quick translation: rowptr[p] = lrow[p+1]-1, colidx[p] = lcidx[p[1]+1]-1, a[p] = la[p[1]+1] */
  LocalSparseMat(ndarray<double, 1 UNSTRIDED> la, ndarray<int, 1 UNSTRIDED> lcidx,
                 ndarray<int, 1 UNSTRIDED> lrow, int paramN,
                 int numProcRows, int numProcCols);

  /*  This method does sparse matrix-vector multiply, multiplying "this"
      object times "input" and putting the result in "output".  */
  void multiply(ndarray<double, 1 UNSTRIDED> output,
                ndarray<double, 1 UNSTRIDED> input);
};
