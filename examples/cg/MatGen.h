#pragma once

#include "globals.h"
#include "SparseMat.h"
#include "Random.h"

class MatGen {
 public:
  //-----------------------------------------------------------------------------
  // Initialization routine
  //-----------------------------------------------------------------------------
  static SparseMat *Initialize(int na, int nonzer, double shift, double rcond);

 private:
  //---------------------------------------------------------------------
  //       generate the test problem for benchmark 6
  //       makea generates a sparse matrix with a
  //       prescribed sparsity distribution
  //
  //       parameter    type        usage
  //
  //       input
  //
  //       n            i           number of cols/rows of matrix
  //       nz           i           nonzeros as declared array size
  //       rcond        r*8         condition number
  //       shift        r*8         main diagonal shift
  //
  //       output
  //
  //       a            r*8         array for nonzeros
  //       colidx       i           col indices
  //       rowstr       i           row pointers
  //
  //       workspace
  //
  //       iv, arow, acol i
  //       v, aelt        r*8
  //---------------------------------------------------------------------
  static void makea(int n, int nz, int firstrow, int lastrow, int firstcol,
                    int lastcol, ndarray<double, 1 UNSTRIDED> a,
                    ndarray<int, 1 UNSTRIDED> colidx,
                    ndarray<int, 1 UNSTRIDED> rowstr,
                    int nonzer, double rcond, int arow[], int acol[],
                    double aelt[], double v[], int iv[], double shift,
                    double amult, Random &rng);
    
  //----------------------------------------------------------------------
  // method sprvnc
  //----------------------------------------------------------------------
  static void sprnvc(int n, int nz, double v[], int iv[],
                     ndarray<int, 1 UNSTRIDED> nzloc,
                     int nzloc_offst, ndarray<int, 1 UNSTRIDED> mark,
                     int mark_offst, double amult, Random &rng);

  //---------------------------------------------------------------------
  // method vecset
  //---------------------------------------------------------------------
  static int vecset(int n, double v[], int iv[],
                    int nzv, int ival, double val);

  //---------------------------------------------------------------------
  // method sparse
  //---------------------------------------------------------------------
  static void sparse(ndarray<double, 1 UNSTRIDED> a,
                     ndarray<int, 1 UNSTRIDED> colidx,
                     ndarray<int, 1 UNSTRIDED> rowstr,
                     int n, int firstrow, int lastrow, int arow[], int acol[], 
                     double aelt[],
                     double x[], int mark[],
                     int mark_offst, int nzloc[], int nzloc_offst,
                     int nnza);
};
