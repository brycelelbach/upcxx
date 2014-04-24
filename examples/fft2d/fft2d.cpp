/**
 * FFT mini-benchmark
 *
 * Distributed FFT on CPUs:
 * The main thread in CPU place 0 does the following:
 *   1) allocate global memory on remote CPU and GPU places
 *   2) spawn async tasks on remote CPU places
 *   3) wait for the completion of the async tasks (events)
 *
 */

#include <upcxx.h>
#include <forkjoin.h> // for single-thread execution model emulation

#include "myfft.h"

#include <cstdlib> // for rand
#include <iostream>
#include <vector>
//#include <algorithm>
#include <cassert>
#include <time.h>
#include <sys/time.h>

//#define DEBUG
//#define DEBUG1
#define VERIFY

using namespace upcxx;

#include <cstdlib>

size_t NX_default = 1024; // = 8*1024; 
size_t NY_default = 1024; //  = NX;

global_ptr<complex_t> *Input, *Output;

double mysecond()
{
  struct timeval tv;
  gettimeofday(&tv, 0);
  return tv.tv_sec + ((double) tv.tv_usec / 1000000);
}


template<typename T>
static inline
void print_matrix(std::ostream &out, T *A, size_t nrows, size_t ncols)
{
  size_t i,j;

  for (i=0; i<nrows; i++) {
    for (j=0; j<ncols; j++) {
      // out << A[i*ncols+j] << " ";
      out << "(" << A[i*ncols+j].real << " + " << A[i*ncols+j].imag << "i)\t ";
    }
    out << "\n";
  }
}

void init_array(complex_t *A, size_t sz, double val=0.0)
{
  assert(A != NULL);
  for (size_t i = 0; i < sz; i++) {
    A[i].real = (double)i + val;
    A[i].imag = 0;
  }
}

static inline
double random_double(void)
{
  long long tmp;

  tmp = rand();
  tmp = (tmp << 32) | rand();
  return (double)tmp;
}

static inline
complex_t random_complex(void)
{
  complex_t tmp;
  tmp.real = random_double();
  tmp.real = random_double();
  return tmp;
}

template<typename T>
void alltoall(global_ptr<T> src[],
              global_ptr<T> dst[],
              size_t count[],
              int np)
{
  int i;
  int myrank = MYTHREAD;

  for (i=0; i<np; i++) {
    int j = (myrank+i) % np;
#ifdef DEBUG
    fprintf(stderr, "myrank %d: i %d, j %d ", myrank, i, j);
    cerr << "src[j] " << src[j] << " dst[j] " << dst[j] 
         << " count[j] " << count[j] << "\n";
#endif
    async_copy(src[j], dst[j], count[j]);
  }
  async_copy_fence();

  barrier();
}

template<typename T>
static inline 
void local_transpose(T *A, size_t N, size_t row_stride, size_t col_stride)
{
  size_t i, j;
  T tmp;
  // assume A is a square matrix stored in row-major storage format
  for (i=0; i<N; i++) {
    for (j=i+1; j<N; j++) { // no need to swap the diagonal elements
      tmp = A[i*row_stride + j*col_stride];
      A[i*row_stride + j*col_stride] = A[j*row_stride + i*col_stride];
      A[j*row_stride + i*col_stride] = tmp;
    }
  }
}

template<typename T>
static inline 
void local_transpose1(T *A, size_t N)
{
  size_t i, j;
  T tmp;
  // assume A is a square matrix stored in row-major storage format
  for (i=0; i<N; i++) {
    for (j=i+1; j<N; j++) { // no need to swap the diagonal elements
      tmp = A[i*N+j];
      A[i*N+j] = A[j*N+i];
      A[j*N+i] = tmp;
    }
  }
}

template<typename T>
static inline 
void local_transpose2(T *A, T *B, size_t n, size_t m)
{
  size_t i, j;
  T tmp;

#ifdef DEBUG
  int myrank = my_cpu_place.id();
  cerr << "myrank " << myrank  << " A: " << A << " B: " << B << "\n";
#endif

  // assume A anb B are in row-major storage format
  // A is n-by-m and B is m-by-n
  for (i=0; i<n; i++) {
    for (j=0; j<m; j++) {
      B[j*n+i] = A[i*m+j];
    }
  }
}

// x - row coordinate
// y - column coordinate
template<typename T>
void blockize(T *in, T *out, size_t nx, size_t ny, 
              size_t blk_nx, size_t blk_ny)
{
  size_t x, y, row_major_offset, blk_cyclic_offset;
  size_t blk_offset_x, blk_offset_y, blk_grid_x, blk_grid_y;

  assert(in != out);
  assert(in != NULL);
  assert(out != NULL);

  for (x = 0; x < nx; x++) {
    for (y = 0; y < ny; y++) {
      blk_offset_x = x % blk_nx;
      blk_offset_y = y % blk_ny;
      blk_grid_x = x / blk_nx;
      blk_grid_y = y / blk_ny;

      // linear offset in the row-major storage format
      row_major_offset = x * ny + y;

      // linear offset in the block-cyclic storage format
      blk_cyclic_offset = 
          blk_grid_x * (blk_nx * ny)
          + blk_grid_y * (blk_nx * blk_ny)
          + blk_offset_x * blk_ny
          + blk_offset_y;

      out[blk_cyclic_offset]
          = in[row_major_offset];
    }
  }
}

template<typename T>
static inline
void transpose(T *in,
               T *out,
               global_ptr< global_ptr<T> > all_Ws,
               size_t nx, // # of rows
               size_t ny, // # of columns
               int nprocs)
{
  global_ptr<T> *src = new global_ptr<T> [nprocs];
  assert(src != NULL);
  global_ptr<T> *dst = new global_ptr<T> [nprocs];
  assert(dst != NULL);
  size_t count[nprocs];
  int i;
  size_t msgsz_per_p  = (nx/nprocs) * (ny/nprocs);
  size_t nx_per_p = nx / nprocs;
  int myrank = MYTHREAD;
  global_ptr<T> tmp_in;
  global_ptr<T> *W = new global_ptr<T> [nprocs];
  assert(W != NULL);

  assert(nx == ny); // only handle the square case for now

  // copy all_Ws to local
  upcxx::copy(all_Ws,
              global_ptr< global_ptr<T> >(W),
              nprocs);

#ifdef DEBUG
  for (int i=0; i<nprocs; i++) {
    fprintf(stderr, "transpose: myrank %d, W[%d] ", myrank, i); 
    cerr << W[i] << "\n";
  }
#endif 

  tmp_in = allocate<T>(my_node, nx_per_p * ny);
  assert(tmp_in.raw_ptr() != NULL);

#ifdef DEBUG
  // print the matrix before transpose
  printf("P%d transpose: Before local_transpose2 (row-major storage):\n", myrank);
  print_matrix<T>(cout, in, nx_per_p, ny);
#endif

  local_transpose2<T>(in, tmp_in.raw_ptr(), nx_per_p, ny);

#ifdef DEBUG
  // print the matrix before transpose
  printf("P%d transpose: After local_transpose2 (should be column-major):\n", 
         myrank);
  print_matrix<T>(cout, (T *)tmp_in.raw_ptr(), ny, nx_per_p);
#endif

  for (i = 0; i < nprocs; i++) {
    local_transpose1<T>((tmp_in + msgsz_per_p * i).raw_ptr(), nx_per_p);
  }

#ifdef DEBUG
  // print the matrix before transpose
  printf("P%d transpose: After local_transpose1 (should be block-cyclic):\n", 
         myrank);
  print_matrix<T>(cout, (T *)tmp_in.raw_ptr(), ny, nx_per_p);
#endif

  for (i=0; i<nprocs; i++) {
    src[i] = tmp_in + (i*msgsz_per_p) ;
    dst[i] = W[i] + (myrank*msgsz_per_p);
    count[i] = msgsz_per_p;

#ifdef DEBUG
    fprintf(stderr, "transpose: myrank %d, i %d ", myrank, i); 
    cerr << "W[i] " << W[i] << " src[i] "<< src[i] << " dst[i] " << dst[i] 
                                                                        << "\n";
#endif
  }

  alltoall(src, dst, count, nprocs);

#ifdef DEBUG
  for (i=0; i<nprocs; i++) {
    if (myrank == i) {
      // print the matrix before transpose
      printf("P%d transpose: After alltoall:\n", 
             myrank);
      print_matrix<T>(cout, (T *)W[myrank].raw_ptr(), ny, nx_per_p);
    }
    barrier();
  }
#endif

  local_transpose2<T>((T *)W[myrank].raw_ptr(), out, ny, nx_per_p);

#ifdef DEBUG
  for (i=0; i<nprocs; i++) {
    if (myrank == i) {
      // print the matrix before transpose
      printf("P%d transpose: After the second local_transpose2:\n", 
             myrank);
      print_matrix<T>(cout, out, nx_per_p, ny);
    }
    barrier();
  }
#endif

  delete [] src;
  delete [] dst;
}

// verify if Z is the correct 2-D FFT of X where X and Z are nx-by-ny
// matrices
void verify(complex_t *X, complex_t *Z, int nx,  int ny)
{
  complex_t *Y, *tmp; // tmp space
  size_t failed = 0;
  double epsilon = 1e-7;

  Y = (complex_t *)malloc(nx * ny * sizeof(complex_t));
  assert(Y != NULL);
  tmp = (complex_t *)malloc(nx * ny * sizeof(complex_t));
  assert(tmp != NULL);

  // compute the 2-D FFT of X by FFTW
  fft2(tmp, X, nx, ny);

  // transpose the result so that it matches my storage format
  local_transpose2(tmp, Y, nx, ny);

#ifdef DEBUG1
  printf("verify: X: \n");
  print_matrix(cout, X, nx, ny);
  printf("verify: Y: \n");
  print_matrix(cout, Y, nx, ny);
  printf("verify: Z: \n");
  print_matrix(cout, Z, nx, ny);
#endif

  // compare the results 
  double max_diff = 0.0;
  for (size_t i = 0; i < nx*ny; i++) {
    double diff_real = Y[i].real - Z[i].real;
    double diff_imag = Y[i].imag - Z[i].imag;
    double abs_diff = sqrt(diff_real * diff_real + diff_imag * diff_imag);
    double abs_y = sqrt(Y[i].real * Y[i].real + Y[i].real * Y[i].real); 
    if (abs_diff / abs_y  > epsilon * log((double)nx*ny)) {
      failed++;
      if (abs_diff > max_diff) {
        max_diff = abs_diff; 
      }
    }
  }

  free(Y);
  free(tmp);

  if (failed) {
    printf("FFT2D verification failed: %lu elements are different, max_diff: %g.\n", 
           failed, max_diff);
  } else {
    printf("FFT2D verification passed.\n");
  }
}

void fft2d(complex_t *my_A,
           complex_t *my_Z,
           complex_t *my_W,
           size_t nx, // # of rows
           size_t ny, // # of columns
           global_ptr<global_ptr<complex_t> > all_Ws)
{
  int nprocs = THREADS;
  complex_t *tmp;
  fft_plan_t planX, planY;
  int myrank = MYTHREAD;
  size_t nx_per_p = nx / nprocs;
  size_t ny_per_p = ny / nprocs;
  size_t size_per_p = nx_per_p * ny;

  assert(nx == ny); // only handle square case for now

  // Initialize the underlying FFT library (FFTW3 or others)
  fft_init();

  // generate random data for the 2-D input array
  std::generate(my_A, &my_A[nx_per_p * ny], random_complex);
  init_array(my_A, size_per_p, myrank * size_per_p);

  // Autotune the FFT performance
  complex_t *pIn = my_A;
  complex_t *pOut = my_W;

  // FFT plan for row (y dimension) FFTs 
  fft_plan_1d(FFT_FORWARD, //myfftdir_t direction, 
              ny, // int len, 
              nx_per_p, // int numffts, 
              pOut, // complex_t *sample_outarray,
              1, // int outstride, 
              ny, // int outtdist, 
              pIn, // complex_t *sample_inarray,
              1, // int instride, 
              ny, // int indist,
              &planY);

  // FFT plan for column (x dimension) FFTs
  fft_plan_1d(FFT_FORWARD, //myfftdir_t direction, 
              nx, // int len, 
              ny_per_p, // int numffts, 
              pOut, // complex_t *sample_outarray,
              1, // int outstride, 
              nx, // int outtdist, 
              pIn, // complex_t *sample_inarray,
              1, // int instride, 
              nx, // int indist,
              &planX);

  // y dimension (row) 1-D FFTs, store results in my_Z
  run_1d_fft(my_Z, my_A, &planY);

  // Transpose the matrix, store results in my_W
  tmp = (complex_t *)malloc(sizeof(complex_t) * (nx*ny/nprocs));
  assert(tmp != NULL);

  transpose<complex_t>(my_Z,
                       tmp,
                       all_Ws,
                       nx, 
                       ny, 
                       nprocs);

  // x dimension (column) 1-D FFTs, store results in my_Z
  run_1d_fft(my_Z, tmp, &planY);
  free(tmp);

#ifdef DEBUG
  for (int i=0; i<nprocs; i++) {
    if (myrank == i) {
      printf("myrank %d: A: \n", myrank);
      print_matrix(cout, (complex_t *)my_A, nx_per_p, ny);
      printf("myrank %d: Z: \n", myrank);
      print_matrix(cout, (complex_t *)my_Z, nx_per_p, ny);
    }
    barrier();
  }
#endif

  /* The verify function works when the full array is locally
   * available, which is true when running with only 1 process.  When
   * running with 2 or more processes, the arrays are distributed and
   * need to be copied to local for comparison.
   */
#ifdef VERIFY
  if (myrank == 0) {
    global_ptr<complex_t> all_in, all_out;
    all_in = allocate<complex_t>(myrank(), nx * ny);
    all_out = allocate<complex_t>(myrank(), nx * ny);
    for (int i = 0; i < nprocs; i++) {
      async_copy(Input[i], all_in + i * size_per_p, size_per_p);
      async_copy(Output[i], all_out + i * size_per_p, size_per_p);
    }
    async_copy_fence();
    verify(all_in.raw_ptr(), all_out.raw_ptr(), (int)nx, (int)ny);
  }
  barrier();
#endif
}

int main(int argc, char **argv)
{
  double starttime;
  int i;
  int nprocs = THREADS;
  // event_t *e[nprocs];
  Input = new global_ptr<complex_t> [nprocs];
  Output = new global_ptr<complex_t> [nprocs];
  size_t nx, ny;
  global_ptr<global_ptr<complex_t> > all_Ws;
  all_Ws = allocate< global_ptr<complex_t> > (my_node, nprocs);
  global_ptr<complex_t> *W
  = (global_ptr<complex_t> *)all_Ws.raw_ptr();

  int tmp = 0;

  // parsing the command line arguments
  if (argc > 1) {
    tmp = atoi(argv[1]);
  }
  nx = (tmp < 2) ? NX_default : tmp;
  ny = nx;    
  size_t nx_per_p = nx / nprocs; // assume nx can be divided by nprocs

  // allocate memory with 1-D data partitioning
  for (i=0; i<nprocs; i++) {
    Input[i] = allocate<complex_t>(node(i), nx_per_p * ny);
    Output[i] = allocate<complex_t>(node(i), nx_per_p * ny);
    W[i] = allocate<complex_t>(node(i), nx_per_p * ny);
  }

#ifdef DEBUG
  for (i=0; i<nprocs; i++) {
    fprintf(stderr, "W[%d] ", i); 
    cerr << W[i] << "\n";
  }
#endif 

  starttime = mysecond();
  // perform FFT in parallel
  for (i=0; i<nprocs; i++) {
    async(i)(fft2d,
        Input[i].raw_ptr(),
        Output[i].raw_ptr(),
        W[i].raw_ptr(),
        nx,
        ny,
        all_Ws);
  }
  upcxx::wait();
  
  double elapsedtime = mysecond() - starttime;
  printf("fft2d (nx %lu, ny %lu), np %d, time = %f s\n",
         nx, ny, nprocs, elapsedtime);

  for (i=0; i<nprocs; i++) {
    upcxx::deallocate(Input[i]);
    upcxx::deallocate(Output[i]);
    upcxx::deallocate(W[i]);
  }

  delete [] Input;
  delete [] Output;

  return 0;
}
