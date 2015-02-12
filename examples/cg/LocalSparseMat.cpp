#include "LocalSparseMat.h"

LocalSparseMat::LocalSparseMat(ndarray<double, 1 UNSTRIDED> la,
                               ndarray<int, 1 UNSTRIDED> lcidx,
                               ndarray<int, 1 UNSTRIDED> lrow, int paramN,
                               int numProcRows, int numProcCols)
  : N(paramN), procRowPos(Util::procRowPos), procColPos(Util::procColPos),
    procRowSize(Util::procRowSize) {
  int rowStart = Util::rowStart;
  int rowEnd = Util::rowEnd;

  int numNonZeros = lrow[procRowSize+1]-1;
	
	// "rowptr", "colidx", and "a" are all in global coordinates and zero-based
	// In contrast, "la", "lcidx", and "lrow" are all one-based
  ndarray<int, 1 UNSTRIDED> rowptr(RECTDOMAIN((rowStart), (rowEnd+2)));
  colidx = ndarray<int, 1 UNSTRIDED>(RECTDOMAIN((0), (numNonZeros)));
  a = ndarray<double, 1 UNSTRIDED>(colidx.domain());

  // populate the "rowptr", "colidx", and "a" arrays
  FOREACH (p, rowptr.domain()) {
    rowptr[p] = lrow.translate(POINT(rowStart-1))[p]-1;
  };
  foreach (p, colidx.domain()) {
    colidx[p] = lcidx[p[1]+1]-1;
    a[p] = la[p[1]+1];
  };

  // populate the "rowRectDomains" array
  rowRectDomains =
    ndarray<RectDomain<1>, 1 UNSTRIDED>(RectDomain<1>(rowptr.domain().min(),
                                                      rowptr.domain().max()));
  FOREACH (i, rowRectDomains.domain()) {
    rowRectDomains[i] = RECTDOMAIN((rowptr[i]), (rowptr[i+1]));
  };
}

void LocalSparseMat::multiply(ndarray<double, 1 UNSTRIDED> output,
                              ndarray<double, 1 UNSTRIDED> input) {
  double sum;

  // make instance arrays into local arrays (BIG performance difference)
  ndarray<RectDomain<1>, 1 UNSTRIDED> lrowRectDomains = rowRectDomains;
  ndarray<int, 1 UNSTRIDED> lcolidx = colidx;
  ndarray<double, 1 UNSTRIDED> la = a;

  // actual SpMV
  FOREACH (i, lrowRectDomains.domain()) {
    sum = 0;
    FOREACH (j, lrowRectDomains[i]) {
      sum += la[j] * input[lcolidx[j]];
    };
    output[i] = sum;
  };
}
