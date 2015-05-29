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

shared_var < global_ptr<double> > tmp_ptr;

int main (int argc, char **argv)
{
  // \Todo: group all init functions together in a global init function
  init(&argc, &argv);
  
  size_t count = 128;

  global_ptr<double> ptr1;
  global_ptr<double> ptr2;

  if (myrank() == 0) {
    ptr1 = allocate<double>(0, count);
    ptr2 = allocate<double>(1, count);

    cerr << "ptr1 " << ptr1 << "\n";
    cerr << "ptr2 " << ptr2 << "\n";
    tmp_ptr = ptr2;

    // Initialize data pointed by ptr by a local pointer
    double *local_ptr1 = (double *)ptr1;
    for (size_t i=0; i<count; i++) {
      local_ptr1[i] = (double)i + myrank() * 1e6;
    }
    
    upcxx::copy(ptr1, ptr2, count);
  }

  barrier();
  
  if (myrank() == 1) {
    ptr2 = tmp_ptr;
    // verify data
    double *local_ptr2 = (double *)ptr2;
    for (int i=0; i<16; i++) {
      cout << local_ptr2[i] << " ";
    }
    cout << endl;
  }

  barrier();

  finalize();

  return 0;
}
