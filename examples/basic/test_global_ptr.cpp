/**
 * \example test_global_ptr.cpp
 *
 * Test global address space pointers, dynamic
 * distributed memory allocation and data transfer (copy) functions.
 *
 * This test is expected to be run with two processes.
 * For example, mpirun -np 2 ./test_global_ptr
 *
 */
#include <upcxx.h>
#include <iostream>

using namespace std;
using namespace upcxx;

shared_array < global_ptr<double> > tmp_ptrs;

int main (int argc, char **argv)
{
  size_t count = 128;

  global_ptr<double> ptr1;
  global_ptr<double> ptr2;

  upcxx::init(&argc, &argv);

  tmp_ptrs.init(ranks());
  
  uint32_t dst_rank = (myrank()+1)%ranks();
  
  {
    ptr1 = allocate<double>(myrank(), count);
    ptr2 = allocate<double>(dst_rank, count);

    cerr << "ptr1 " << ptr1 << "\n";
    cerr << "ptr2 " << ptr2 << "\n";
    tmp_ptrs[dst_rank] = ptr2;

    // Initialize data pointed by ptr by a local pointer
    double *local_ptr1 = (double *)ptr1;
    for (size_t i=0; i<count; i++) {
      local_ptr1[i] = (double)i + myrank() * 1e6;
    }

    upcxx::copy(ptr1, ptr2, count);
  }

  barrier();

  {
    ptr2 = tmp_ptrs[myrank()];
    // verify data
    double *local_ptr2 = (double *)ptr2;
    for (int i=0; i<16; i++) {
      cout << local_ptr2[i] << " ";
    }
    cout << endl;
  }

  barrier();
  if (myrank() == 0)
    printf("test_global_ptr passed!\n");

  upcxx::finalize();
  return 0;
}
