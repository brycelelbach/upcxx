#include <cmath>
#include "MatGen.h"

SparseMat *MatGen::Initialize(int na, int nonzer, double shift, double rcond) {
  ndarray<int, 1 UNSTRIDED> colidx, rowstr;
  ndarray<double, 1 UNSTRIDED> a;

  int *iv, *arow, *acol;
  double *v, *aelt;
	
  double *m;

  // declarations
  double amult = 1220703125.0;
  Random rng;	       
	
  int i, j, k, it;
  double zeta;

  // Retrieve values from Util class.
  int numProcRows = Util::numProcRows;
  int numProcCols = Util::numProcCols;
  // 1-indexed values
  int firstrow = Util::rowStart + 1;
  int lastrow = Util::rowEnd + 1;
  int firstcol = Util::colStart + 1;
  int lastcol = Util::colEnd + 1;

  // initialization of problem
  int nz = na*(nonzer+1)*(nonzer+1)/ranks() + na*(nonzer+2+ranks()/256)/numProcCols;
  colidx = ndarray<int, 1 UNSTRIDED>(RD(0, nz+1));
  rowstr = ndarray<int, 1 UNSTRIDED>(RD(0, na+2));
  iv = new int[2*na+2];
  arow = new int[nz+1]; 
  acol = new int[nz+1];
  v = new double[na+2]; 
  aelt = new double[nz+1]; 
  a = ndarray<double, 1 UNSTRIDED>(RD(0, nz+1));
  // rng=new Random();

  int naa = na;
  int nzz = nz;
	
  //---------------------------------------------------------------------
  //  Inialize random number generator
  //---------------------------------------------------------------------
  zeta = rng.randlc(amult);
	
  //---------------------------------------------------------------------
  //  Creates matrix A
  //---------------------------------------------------------------------
  makea(naa, nzz, firstrow, lastrow, firstcol, lastcol, a,
        colidx, rowstr, nonzer, rcond, arow, acol, aelt, v, iv,
        shift, amult, rng);
	
  for(j = na+2; j <= rowstr.domain().max()[1]; ++j){
    rowstr[j] = rowstr[na+1];
  }

  delete[] iv;
  delete[] arow;
  delete[] acol;
  delete[] v;
  delete[] aelt;

  SparseMat *dsm = SparseMat::makeSparseMat(a, colidx, rowstr, numProcRows,
                                            numProcCols, na);
  return dsm;
}
//--------------------------------------------------------------------------------
// End Initialization routine
//--------------------------------------------------------------------------------

void MatGen::makea(int n, int nz, int firstrow, int lastrow, int firstcol,
                   int lastcol, ndarray<double, 1 UNSTRIDED> a,
                   ndarray<int, 1 UNSTRIDED> colidx,
                   ndarray<int, 1 UNSTRIDED> rowstr,
                   int nonzer, double rcond, int arow[], int acol[],
                   double aelt[], double v[], int iv[], double shift,
                   double amult, Random &rng) {

  int i, nnza,  ivelt, ivelt1, irow, nzv,jcol;

  //---------------------------------------------------------------------
  //      nonzer is approximately  (int(sqrt(nnza /n)));
  //---------------------------------------------------------------------

  double size, ratio, scale;

  size = 1.0;
  ratio = pow(rcond , (1.0/(float)n) );
  nnza = 0;

  //---------------------------------------------------------------------
  //  Initialize colidx(n+1 .. 2n) to zero.
  //  Used by sprnvc to mark nonzero positions
  //---------------------------------------------------------------------

  for (i=1;i<=n;i++) {
    colidx[n+i] = 0;
  }
  for (int iouter=1;iouter<=n;iouter++) {
    nzv = nonzer;
    sprnvc(n, nzv, v, iv, colidx, 0, colidx, n, amult, rng);
    nzv = vecset(n, v, iv, nzv, iouter, .5);
	    
    for (ivelt=1;ivelt<=nzv;ivelt++) {
      jcol = iv[ivelt];
      if (jcol>=firstcol && jcol<=lastcol) {
        scale = size * v[ivelt];
        for(ivelt1=1;ivelt1<=nzv;ivelt1++) {
          irow = iv[ivelt1];
          if (irow>=firstrow && irow<=lastrow) {
            nnza = nnza + 1;
            if (nnza > nz){
              println("Space for matrix elements exceeded in makea");
              println("nnza, nzmax = " << nnza << ", " << nz);
              println(" iouter = " << iouter);
              abort();
            }
            acol[nnza] = jcol;
            arow[nnza] = irow;
            aelt[nnza] = v[ivelt1] * scale;
          }
        }
      }
    }
    size = size * ratio;
  }
	
  //---------------------------------------------------------------------
  //       ... add the identity * rcond to the generated matrix to bound
  //           the smallest eigenvalue from below by rcond
  //---------------------------------------------------------------------
  for(i=firstrow; i<=lastrow;i++) {
    if (i >= firstcol && i <= lastcol) {
      int iouter = n + i;
      nnza = nnza + 1;
      if (nnza>nz) { 
        println("Space for matrix elements exceeded in makea");
        println("nnza, nzmax = " << nnza << ", " << nz);
        println(" iouter = " << iouter);
        abort();
      }
      acol[nnza] = i;
      arow[nnza] = i;
      aelt[nnza] = rcond - shift;
    }
  }
	
  //---------------------------------------------------------------------
  //       ... make the sparse matrix from list of elements with
  //       duplicates (v and iv are used as  workspace)
  //---------------------------------------------------------------------
  sparse(a, colidx, rowstr, n, firstrow, lastrow, arow, acol, aelt,
         v, iv, 0 , iv, n, nnza);
  return;
}

void MatGen::sprnvc(int n, int nz, double v[], int iv[],
                    ndarray<int, 1 UNSTRIDED> nzloc,
                    int nzloc_offst, ndarray<int, 1 UNSTRIDED> mark,
                    int mark_offst, double amult, Random &rng) {
  int nzrow=0,nzv=0,idx;
  int nn1 = 1;
	
  while (true) {
    nn1 = 2 * nn1;
    if (nn1 >= n ) break;
  } 
	
  while (true) {
    if(nzv >= nz){
      for(int ii = 1;ii<=nzrow;ii++) {
        idx = nzloc[ii+nzloc_offst];
        mark[idx+mark_offst] = 0;
      }
      return;
    }
    double vecelt = rng.randlc(amult);
    double vecloc = rng.randlc(amult);
    idx = (int) (vecloc*nn1)+1;
    if(idx > n) continue;
	    
    if(mark[idx+mark_offst] == 0){
      mark[idx+mark_offst] = 1;
      nzrow = nzrow + 1;
      nzloc[nzrow + nzloc_offst] = idx;
      nzv = nzv + 1;
      v[nzv] = vecelt;
      iv[nzv] = idx;
    }
  }
}

int MatGen::vecset(int n, double v[], int iv[],
                   int nzv, int ival, double val) {
  bool set = false; 
  for(int k=1; k<=nzv;k++){
    if (iv[k] == ival){
      v[k]=val;
      set=true;
    }
  }
  if(!set){
    nzv     = nzv + 1;
    v[nzv]  = val;
    iv[nzv] = ival;
  }
  return nzv;    
}

void MatGen::sparse(ndarray<double, 1 UNSTRIDED> a,
                    ndarray<int, 1 UNSTRIDED> colidx,
                    ndarray<int, 1 UNSTRIDED> rowstr,
                    int n, int firstrow, int lastrow, int arow[], int acol[],
                    double aelt[],
                    double x[], int mark[],
                    int mark_offst, int nzloc[], int nzloc_offst,
                    int nnza) {

  //---------------------------------------------------------------------
  //       rows range from firstrow to lastrow
  //       the rowstr pointers are defined for nrows = lastrow-firstrow+1
  //       values
  //---------------------------------------------------------------------
  int nrows;
  //---------------------------------------------------
  //       generate a sparse matrix from a list of
  //       [col, row, element] tri
  //---------------------------------------------------  
  int i, j, jajp1, nza, k, nzrow;
  double xi;	
  //---------------------------------------------------------------------
  //    how many rows of result
  //---------------------------------------------------------------------
  nrows = lastrow - firstrow + 1;
	
  //---------------------------------------------------------------------
  //     ...count the number of triples in each row
  //---------------------------------------------------------------------
  for (j=1;j<=n;j++) {
    rowstr[j] = 0;
    mark[j+mark_offst] = 0;
  }
  rowstr[n+1] = 0;
	
  for (nza=1;nza<=nnza;nza++) {
    j = (arow[nza] - firstrow + 1)+1;
    rowstr[j] = rowstr[j] + 1;
  }
	
  rowstr[1] = 1;
  for (j=2;j<=nrows+1;j++) {
    rowstr[j] = rowstr[j] + rowstr[j-1];
  }
  //---------------------------------------------------------------------
  //     ... rowstr(j) now is the location of the first nonzero
  //           of row j of a
  //---------------------------------------------------------------------
	
  //---------------------------------------------------------------------
  //     ... do a bucket sort of the triples on the row index
  //---------------------------------------------------------------------
  for (nza=1;nza<=nnza;nza++) {
    j = arow[nza] - firstrow + 1;
    k = rowstr[j];
    a[k] = aelt[nza]; // a is here !!!
    colidx[k] = acol[nza];
    rowstr[j] = rowstr[j] + 1;
  }    
	
  //---------------------------------------------------------------------
  //       ... rowstr(j) now points to the first element of row j+1
  //---------------------------------------------------------------------
  for (j=nrows-1;j>=0;j--) {
    rowstr[j+1] = rowstr[j];
  }
  rowstr[1] = 1;
  //---------------------------------------------------------------------
  //       ... generate the actual output rows by adding elements
  //---------------------------------------------------------------------
  nza = 0;
  for (i=1;i<=n;i++) {
    x[i]    = 0.0;
    mark[i+mark_offst] = 0;
  }
	
  jajp1 = rowstr[1];
	
  for(j=1;j<=nrows;j++){
    nzrow = 0;
	    
    //---------------------------------------------------------------------
    //          ...loop over the jth row of a
    //---------------------------------------------------------------------
    for(k = jajp1; k<=(rowstr[j+1]-1);k++){
      i = colidx[k];
      x[i] = x[i] + a[k];  // a is here!
		
      if ( (mark[i+mark_offst]==0) && (x[i] != 0)){
        mark[i+mark_offst] = 1;
        nzrow = nzrow + 1;
        nzloc[nzrow+nzloc_offst] = i;
      }
    }
	    
    //---------------------------------------------------------------------
    //          ... extract the nonzeros of this row
    //---------------------------------------------------------------------
    for (k=1;k<=nzrow;k++) {
      i = nzloc[k+nzloc_offst];
      mark[i+mark_offst] = 0;
      xi = x[i];
      x[i] = 0;
      if (xi != 0){
        nza = nza + 1;
        a[nza] = xi;   // a is here !!
        colidx[nza] = i;
      }
    }
    jajp1 = rowstr[j+1];
    rowstr[j+1] = nza + rowstr[1];
  }	
  return;
}
