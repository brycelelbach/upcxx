/**
 * \example test_asymmetric_partition.cpp
 *
 */
#include <upcxx.h>
#include <iostream>

using namespace upcxx;

int main(int argc, char **argv)
{
  // Can only use global_myrank() and/or global_ranks() before init()
  size_t request_size = ((global_myrank()%8)+1) * 64*1024*1024;
  request_my_global_memory_size(request_size);
  init();

  if (myrank() == 0) {
    global_ptr<double> *ptrs = new global_ptr<double>[ranks()];
    for (uint32_t i = 0; i < ranks(); i++) {
      printf("Rank %u global_memory_size %lu bytes.\n",
             i, query_global_memory_size(i));
      ptrs[i] = allocate<double>(0, ((i%8)+1) * 1024);
#ifdef UPCXX_HAVE_CXX11
      assert(ptr[i] != nullptr);
#else
      assert(ptr[i].raw_ptr() != NULL);
#endif
      deallocate(ptrs[i]);
    }
    delete [] ptrs;
  }

  barrier();
  if (myrank() == 0) {
    printf("test_asymmetric_partition passed!\n");
  }

  finalize();
  return 0;
}
