#include <assert.h>
#include <math.h>

using namespace upcxx;

/*
 * verify C == Cprime
 */
template<typename T>
int verify(matrix<T> &C, matrix<T> &Cprime)
{
  T epsilon = 1e-5;

  assert(C.m() == Cprime.m());
  assert(C.n() == Cprime.n());

  int count = 0;
  // compare c with c'
   for (int i=0; i<C.m(); i++) {
    for (int j=0; j<C.n(); j++) {
      if (fabs(C(i,j) - Cprime(i,j))/fabs(C(i,j)) > epsilon) {
        count++;
      }
    }
   }

   if (count) {
     printf("Verification failed, found %d errors.\n", count);
   } else {
     printf("Verification succeeded, the results is correct.\n");
   }

   return count;
}

/*
 * computer C = A * B
 */
template <typename T>
void slow_gemm(matrix<T> &A, matrix<T> &B, matrix<T> &C)
{
  // Verify matrix shapes are compatible
  assert(C.m() == A.m());
  assert(A.n() == B.m());
  assert(C.n() == B.n());

  // compute c' = A * B by the definition (a very slow algorithm)
  for (int i=0; i<A.m(); i++) {
    for (int j=0; j<B.n(); j++) {
      T sum = (T)0;
      for (int k=0; k<A.n(); k++) {
        sum += A(i,k) * B(k,j);
      }
      C(i,j) = sum;
    }
  }
}

