/**
 * \example test_dist_array.cpp
 *
 * Test the 1-D global distributed array implementation
 */
#include <upcxx.h>
#include <forkjoin.h> // for single-thread execution model emulation

#include <iostream>

using namespace upcxx;

const size_t ARRAY_SIZE = 16;

// cyclic distribution of global array A
shared_array<unsigned long> A(ARRAY_SIZE);

void update()
{
  int np = upcxx::ranks();
  int myid = upcxx::myrank();

  printf("Rank %d in update\n", myid);
  A.init();
  printf("Rank %d after shared array A.init\n", myid);
  barrier();
  printf("myid %d is updating...\n", myid);

  for (size_t i=myid; i<ARRAY_SIZE; i+=np) {
    A[i] = i + myid * 1000;
  }

  printf("myid %d finishes updates.\n", myid);
}

int main(int argc, char **argv)
{
  printf("I'm in main\n");
  // upcxx::init(&argc, &argv);
  for (int i = THREADS-1; i>=0; i--) {
    printf("async to %d for array updates.\n", i);
    async(i)(update);
  }
  printf("After async, before async_wait\n");
  upcxx::async_wait();

  for (size_t i=0; i<ARRAY_SIZE; i++) {
    // printf("A[%lu]=%lu ", i, (unsigned long)A[i]);
    // printf("A[%lu]=%lu ", i, A[i]);
    printf("A[%lu]=", i); cout << A[i] << " ";
  }
  printf("\n");

  int a = A[3] + 1;
  printf("A[3] + 1 = %d\n", a);

  // finalize();
  return 0;
}
