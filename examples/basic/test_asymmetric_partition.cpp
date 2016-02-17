/**
 * \example test_asymmetric_partition.cpp
 *
 */
#include <upcxx.h>
#include <iostream>
#include <assert.h>

using namespace upcxx;
shared_lock output_mutex;

int main(int argc, char **argv)
{
  // Can only use global_myrank() and/or global_ranks() before init()
  size_t request_size = ((global_myrank()%8)+1) * 64*1024*1024;
  request_my_global_memory_size(request_size);
  init();

  output_mutex.lock();
  std::cout << "Rank " << myrank () << " my_usable_global_memory_size after init is "
            << my_usable_global_memory_size() << " bytes\n";
  output_mutex.unlock();

  barrier();

  global_ptr<double> *ptrs;
  if (myrank() == 0) {
    ptrs = new global_ptr<double>[ranks()];
    for (uint32_t i = 0; i < ranks(); i++) {
      printf("Rank %u global_memory_size %lu bytes.\n",
             i, global_memory_size_on_rank(i));
      // each rank allocates a multiple of 32MB space (4M doubles)
      size_t count = ((i%8)+1) * 6*1024*1024;
      ptrs[i] = allocate<double>(i, count);
#ifdef UPCXX_HAVE_CXX11
      if (ptrs[i] == nullptr) {
#else
      if (ptrs[i].raw_ptr() == NULL) {
#endif
        std::cout << "Failed to allocate " << count*sizeof(double)
                  << " bytes of data on rank " << myrank() << "!\n";
        exit(1);
      }
      std::cout << "ptrs[" << i << "] " << ptrs[i] << std::endl;
    }
  }

  barrier();

  output_mutex.lock();
  std::cout << "Rank " << myrank () << " my_usable_global_memory_size after allocate is "
            << my_usable_global_memory_size() << " bytes\n";
  output_mutex.unlock();

  barrier();

  if (myrank() == 0) {
    for (uint32_t i = 0; i < ranks(); i++) {

      deallocate(ptrs[i]);
    }
    delete [] ptrs;
  }

  barrier();

  output_mutex.lock();
  std::cout << "Rank " << myrank () << " my_usable_global_memory_size after deallocate is "
              << my_usable_global_memory_size() << " bytes\n";
  output_mutex.unlock();

  barrier();

  if (myrank() == 0) {
    printf("test_asymmetric_partition passed!\n");
  }

  finalize();
  return 0;
}
