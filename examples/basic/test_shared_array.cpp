/**
 * \example test_dist_array.cpp
 *
 * Test the 1-D global distributed array implementation
 */
#include <upcxx.h>
#include <forkjoin.h> // for single-thread execution model emulation

#include <iostream>

using namespace upcxx;

const size_t ARRAY_SIZE = 8;

// cyclic distribution of global array A
shared_array<unsigned long> A(ARRAY_SIZE);

void update()
{
  int np = THREADS;
  int myid = MYTHREAD;

  A.init();

  printf("myid %d is updating...\n", myid);

  for (size_t i=myid; i<ARRAY_SIZE; i+=np) {
    A[i] = i + myid * 1000;
  }

  printf("myid %d finishes updates.\n", myid);
}

int main(int argc, char **argv)
{
  int node_count = global_machine.node_count();

  for (int i = node_count-1; i>=0; i--) {
    async(i)(update);
  }

  upcxx::wait();

  for (size_t i=0; i<ARRAY_SIZE; i++) {
    // printf("A[%lu]=%lu ", i, (unsigned long)A[i]);
    // printf("A[%lu]=%lu ", i, A[i]);
    printf("A[%lu]=", i); cout << A[i] << " ";
  }
  printf("\n");

  int a = A[3] + 1;
  printf("A[3] + 1 = %d\n", a);

  return 0;
}
