/**
 * \example test_remote_inc.cpp
 *
 */
#include <upcxx.h>

#include <iostream>

using namespace upcxx;

const size_t ARRAY_SIZE = 16;

// cyclic distribution of global array A
shared_array<long> A(ARRAY_SIZE);

int main(int argc, char **argv)
{
  init(&argc, &argv);
  A.init();

  for (int j=0; j<10; j++) {
    for (size_t i=0; i<ARRAY_SIZE; i++) {
      remote_inc(&A[i]);
    }
  }   
  barrier();

  if (MYTHREAD == 0) {
    for (size_t i=0; i<ARRAY_SIZE; i++) {
      // printf("A[%lu]=%lu ", i, (unsigned long)A[i]);
      // printf("A[%lu]=%lu ", i, A[i]);
      printf("A[%lu]=", i); cout << A[i] << " ";
    }
    printf("\n");
  }
  barrier();

  finalize();
  return 0;
}
