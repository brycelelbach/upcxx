/**
 * Sparse matrix-vector multiplication example
 */

#include <upcxx.h>
#include <forkjoin.h>

using namespace upcxx;

#include <algorithm>
#include <cstdlib>

size_t NNZPR = 64;  // 512;  // number of non-zero elements per row
size_t M = 2048;  // 1024*1024; // matrix size (M-by-M)

#define TIME() gasnett_ticks_to_us(gasnett_ticks_now())

template<typename T>
void print_sparse_matrix(T *A,
                         size_t *col_indices,
                         size_t *row_offsets,
                         size_t nrows)
{
  size_t i, j, k;

  for (i = 0; i < nrows; i++) {
    for (j = 0; j < col_indices[i]; j++) {
      printf("%lg(llu)\t ", A[j]);
    }
    printf("\n");
  }
}

int random_int()
{
  //void srand ( unsigned int seed );
  return rand() % M;
}

double random_double()
{
  long long tmp;

  tmp = rand();
  tmp = (tmp << 32) | rand();
  return (double) tmp;
}

// if the compiler doesn't support the "restrict" type qualifier then 
// pass "-Drestrict " to CXXFLAGS.
#define restrict /* Don't have the restrict keyword */
template<typename T>
void spmv(const T * restrict A,
          const size_t * restrict col_indices,
          const size_t * restrict row_offsets,
          size_t nrows,
          const T * restrict x,
          T * restrict y)

{
  // len(row_offsets) == nrows + 1
#pragma omp parallel for firstprivate(A, col_indices, row_offsets, x, y)
  for (size_t i = 0; i < nrows; i++) {
    T tmp = 0;
    for (size_t j = row_offsets[i]; j < row_offsets[i + 1]; j++) {
      tmp += A[j] * x[col_indices[j]];
    }
    y[i] = tmp;
  }
}

template<typename T>
void gen_matrix(T *my_A,
                size_t *my_col_indices,
                size_t *my_row_offsets,
                size_t nrows)
{
  int num_nnz = NNZPR * nrows;

  // generate the sparse matrix
  // each place only generates its own portion of the sparse matrix
  my_row_offsets[0] = 0;
  for (size_t row = 0; row < nrows; row++) {
    my_row_offsets[row + 1] = (row + 1) * NNZPR;
  }

  // Generate the data and the column indices
  for (int i = 0; i < num_nnz; i++) {
    my_col_indices[i] = random_int();
    my_A[i] = random_double();
  }
}

template<typename T>
void gen_matrix2(T *my_A,
                 size_t *my_col_indices,
                 size_t *my_row_offsets,
                 size_t nrows)
{
  // generate the sparse matrix
  // each place only generates its own portion of the sparse matrix
  my_row_offsets[0] = 0;
  for (size_t row = 0; row < nrows; row++) {
    my_row_offsets[row + 1] = row * NNZPR;
    // Set the matrix with a simple pattern
    for (size_t i = 0; i < NNZPR; i++) {
      my_A[row * NNZPR + i] = 1;
      my_col_indices[row * NNZPR + i] = i;
    }
  }
}

void print_usage(char *s)
{
  assert(s != NULL);
  printf("usage: %s (matrix_size) (nonzeros_per_row) \n", s);
}

int main(int argc, char **argv)
{
  double start_time;
  int i;
  int P = ranks();

  global_ptr<double> *A= new global_ptr<double> [P];
  assert(A != NULL);
  global_ptr<size_t> *col_indices = new global_ptr<size_t> [P];
  assert(col_indices != NULL);
  global_ptr<size_t> *row_offsets = new global_ptr<size_t> [P];
  assert(row_offsets != NULL);
  size_t nrows;
  global_ptr<double> *x = new global_ptr<double> [P];
  assert(x != NULL);
  global_ptr<double> *y = new global_ptr<double> [P];
  assert(y != NULL);

  // parsing the command line arguments
  if (argc > 1) {
    if (strcmp(argv[1], "-h") == 0) {
      print_usage(argv[0]);
      return 0;
    }
    int tmp;
    tmp = atoi(argv[1]);
    if (tmp > 2) M = tmp;
  }

  if (argc > 2) {
    int tmp;
    tmp = atoi(argv[2]);
    if (tmp > 0) NNZPR = tmp;
  }

  // read NNZPR and M from argv

  nrows = M / P;  // assume M can be divided by P

  // allocate memory
  for (i = 0; i < P; i++) {
    A[i] = allocate<double>(i, NNZPR * nrows);
    col_indices[i] = allocate<size_t>(i, NNZPR * nrows);
    row_offsets[i] = allocate<size_t>(i, nrows + 1);
    x[i] = allocate<double>(i, M);
    y[i] = allocate<double>(i, nrows);
  }

  // range all_p(0, ranks());
  for (i = 0; i < P; i++) {
    async(i)(gen_matrix<double>,
             A[i].raw_ptr(),
             col_indices[i].raw_ptr(),
             row_offsets[i].raw_ptr(),
             nrows);
  }

  // Generate the data
  double *my_x = (double *) x[0].raw_ptr();
  std::generate(my_x, my_x + M, random_double);

  start_time = TIME();

  // replicate x to all places with async_copy for overlapping (it's
  // essentially a broadcast.)
  for (i = 1; i < P; i++) {
    async_copy(x[0], x[i], M);
  }
  async_wait();

  // perform matrix multiplication in parallel
  for (i = 0; i < P; i++) {
    async(i)(spmv<double>,
             A[i].raw_ptr(),
             col_indices[i].raw_ptr(),
             row_offsets[i].raw_ptr(),
             nrows,
             x[i].raw_ptr(),
             y[i].raw_ptr());
  }
  async_wait();

  double elapsed_time = TIME() - start_time;
  printf("spmv NNZ %lu, M %lu, np %d, time = %f s\n",
         NNZPR * M, M, P, elapsed_time / 1e+6);

#if 0
  printf("spmv_cpu result is:\n");
  for (i=0; i<P; i++) {
    printf("cpu place %d: ", i);
    for(int j=0; j<MIN(nrows,20); j++) {
      printf("y[%llu] = %4.1f\n", i*nrows+j, y[i][j]);
    }
  }
#endif

  delete [] A;
  delete [] col_indices;
  delete [] row_offsets;
  delete [] x;
  delete [] y;

  return 0;
}

