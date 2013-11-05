///
/// Parallel distributed dense matrix-matrix multiplication by the
/// SUMMA algorithm.  
///
///


#include <upcxx.h>
#include "matrix.h"

using namespace upcxx;

#include <cstdlib>
#include <algorithm>  // for std::generate

#include <time.h>
#include <sys/time.h>

// For debugging and testing
#include "verify.cpp"

// #define VERIFY

// matrix A is M-by-L and matrix B is L-by-N and C is M-by-N

const int default_matrix_sz = 1024;
const int default_blk_sz = 512;
const int default_pgrid_nrow = 1;

int M;
int N;
int L;
int blk_sz;
int pgrid_nrow;
int pgrid_ncol;

// Global matrices A, B and C
shared_var< global_matrix<double> > A_shared, B_shared, C_shared;

// local copy of the global matrix views
global_matrix<double> A, B, C;

double mysecond()
{
  struct timeval tv;
  gettimeofday(&tv, 0);
  return tv.tv_sec + ((double) tv.tv_usec / 1000000);
}

double random_double(void)
{
  long long tmp;

  tmp = rand();
  tmp = (tmp << 32) | rand();

  return (double)tmp;

  //return 1.0;
}

// Function for generating the matrices. No function argument is
// needed as all data the shared through the global address space
void gen_matrix()
{
  int foo = 1;
  
  // copy the global matrix view to local
  /*
  global_ptr<global_matrix<double> > pA(&A);
  global_ptr<global_matrix<double> > pA_shared = &A_shared;
  */

  // cerr << "pA: " << pA << ", pA_shared: " << pA_shared << "\n";

  // upcxx::copy(&A_shared, global_ptr<global_matrix<double> >(&A), 1);
  // upcxx::copy(&B_shared, global_ptr<global_matrix<double> >(&B), 1);
  // upcxx::copy(&C_shared, global_ptr<global_matrix<double> >(&C), 1);
  A = A_shared.get();
  B = B_shared.get();
  C = C_shared.get();

  // Generate the data only for my local blocks
  for (int i = 0; i < A.m_blks(); i++) {
    for (int j = 0; j < A.n_blks(); j++) {
      global_ptr<double> curr_block = A(i, j);
      if (curr_block.where().islocal()) {
        std::generate((double *)curr_block.raw_ptr(), // begin
                      (double *)curr_block.raw_ptr() + (A.blk_sz() * A.blk_sz()),
                      random_double);

        // printf("generate i %d, j %d\n", i, j);
        // matrix<double> Ablk(A.blk_sz(), A.blk_sz(), curr_block.raw_ptr());
      }
    }
  }

  for (int i = 0; i < B.m_blks(); i++) {
    for (int j = 0; j < B.n_blks(); j++) {
      global_ptr<double > curr_block = B(i, j);
      if (curr_block.where().islocal()) {
        std::generate((double *)curr_block.raw_ptr(), // begin
                      (double *)curr_block.raw_ptr() + (B.blk_sz() * B.blk_sz()),
                      random_double);
      }
    }
  }
  
  for (int i = 0; i < C.m_blks(); i++) {
    for (int j = 0; j < C.n_blks(); j++) {
      global_ptr<double > curr_block = C(i, j);
      if (curr_block.where().islocal()) {
        memset(curr_block.raw_ptr(), 0, sizeof(double) * C.blk_sz() * C.blk_sz()); // begin
      }
    }
  }
}

// Implement the SUMMA algorithm for parallel matrix multiplication   
// C = A * B
template<typename T>
void summa(global_matrix<T> &A, global_matrix<T> &B, global_matrix<T> &C)
{
  // Should check if the size and distribution of A, B and C are conforming

	matrix<T> tmp1(blk_sz, blk_sz), tmp2(blk_sz, blk_sz);
  global_ptr<T> pTmp1(tmp1.data());
  global_ptr<T> pTmp2(tmp2.data());

  int m_blks = A.m_blks();
  int l_blks = A.n_blks();
  assert(l_blks == B.m_blks());
  int n_blks = B.n_blks();
  int blk_sz = A.blk_sz();
  int pgrid_nrow = A.pgrid_nrow();
  int pgrid_ncol = A.pgrid_ncol();
  int myrow, mycol;

  A.node_to_pgrid(my_node, myrow, mycol);

  for (int k = 0; k < l_blks; k++) {
    for (int i = 0; i < m_blks; i += pgrid_nrow) {
      // move the A(i+myrow, k) block locally
      upcxx::copy<T>(A(i+myrow, k), pTmp1, blk_sz * blk_sz);
      
      for (int j = 0; j < n_blks; j += pgrid_ncol) {
        // move the B(k, j+mycol) block locally
        upcxx::copy<T>(B(k, j+mycol), pTmp2, blk_sz * blk_sz);
        matrix<T> tmp3(blk_sz, blk_sz, C(i+myrow, j+mycol).raw_ptr());

        // This barrier would slow down BG/P performance
        // substantially.  But it can improve performance on Carver
        // significantly.  
        /* barrier(); */

        // local matrix multiplication: tmp3 += tmp1 * tmp2;       
        matrix<T>::gemm(tmp1, tmp2, tmp3);
      } // close of the j loop
    } // close of the i loop
  } // close of the k loop
}

void print_usage(char *s)
{
  assert(s != NULL);
  printf("usage: %s (matrix size) (block size) (processor grid rows) \n", s);
}

int main(int argc, char **argv)
{
  int rv;
  upcxx::init(&argc, &argv);

  // parsing the command line arguments
  if (argc > 1) {
    if (strcmp(argv[1], "-h") == 0) {
      print_usage(argv[0]);
      return 0;
    }
    M = atoi(argv[1]);
  }
  // try to fix the matrix size if it is incorrect
  if (M < 2) {
    M = default_matrix_sz;
  }
  N = M;
  L = M;

  if (argc > 2) {
    blk_sz = atoi(argv[2]);
  }
  // try to fix blk_sz if it is incorrect
  if (blk_sz < 1) {
    blk_sz = default_blk_sz;
  }
  if (blk_sz > M) {
    blk_sz = M;
  }

  if (argc > 3) {
    pgrid_nrow = atoi(argv[3]);
  }
  // try to fix pgrid_nrow if it is incorrect
  if (pgrid_nrow < 1 || pgrid_nrow > THREADS) {
    pgrid_nrow = default_pgrid_nrow;
  }
  pgrid_ncol = THREADS / pgrid_nrow;
  assert(pgrid_nrow * pgrid_ncol == THREADS);

  int foo;
  int m_blks = M / blk_sz;
  int n_blks= N / blk_sz;

  if (MYTHREAD == 0) {
    // allocate memory for the global matrices
    A.alloc_blocks(m_blks, n_blks, blk_sz, pgrid_nrow, pgrid_ncol);
    B.alloc_blocks(m_blks, n_blks, blk_sz, pgrid_nrow, pgrid_ncol);
    C.alloc_blocks(m_blks, n_blks, blk_sz, pgrid_nrow, pgrid_ncol);
    
#ifdef DEBUG
    cout << "A: " << A;
    cout << "B: " << B;
    cout << "C: " << C;
#endif
    
    A_shared = A; 
    B_shared = B; 
    C_shared = C;
  }

  barrier();
  if (MYTHREAD == 0)
    printf("Generating the matrices...\n");

  gen_matrix();
  barrier();
    
  if (MYTHREAD == 0)
    printf("Performing SUMMA matrix-matrix multiplication...\n");
  
  barrier();
  double starttime = mysecond();
  summa<double>(A, B, C);
  barrier();
  double elapsedtime = mysecond() - starttime;
  
  if (MYTHREAD == 0) {
    printf("matrix multiplication: M %d, blk_sz %d, np %d, pgrid_nrow %d, time %f sec, Gflops %f\n",
           M, blk_sz, THREADS, pgrid_nrow, elapsedtime, 
           2.0*M*L*N/1024/1024/1024/elapsedtime);           
  }
  barrier();

  // debug and verify
#ifdef VERIFY
  if (MYTHREAD == 0) {
    matrix<double> *A_local, *B_local, *C_local;
    A_local = A.gather();
    cout << "local copy of A:" << *A_local;
    B_local = B.gather();
    cout << "local copy of B:" << *B_local;
    C_local = C.gather();
    cout << "local copy of C:" << *C_local;

    matrix<double> Cprime(A_local->m(), B_local->n());
    slow_gemm(*A_local, *B_local, Cprime);
    cout << "Cprime:" << Cprime;

    verify(*C_local, Cprime);
    delete A_local, B_local, C_local;
  }
  barrier();
#endif

  // clean up gasnet before exit
  finalize();

  return rv;
}
