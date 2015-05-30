/**
 * \example test_fetch_add.cpp
 *
 * test atomic fetch-and-add
 */

#include <upcxx.h>
#include <iostream>

using namespace upcxx;

upcxx::shared_array<upcxx::atomic<uint64_t> >counters(ranks());

uint64_t dummy = 0;

int main(int argc, char **argv)
{
  printf("Rank %u of %u starts test_fetch_add...\n",
         myrank(), ranks());

  // counters[myrank()] = 0;

  barrier();

  uint64_t old_val;
  
  for (int i = 0; i < 100; i++) {
    for (int t = 0; t < ranks(); t++) {
      global_ptr<upcxx::atomic<uint64_t> > obj = &counters[t];
      old_val = fetch_add(obj, 10);
      if (old_val % 1000 == 0) {
        printf("Rank %u, t = %d fetch_add old value =  %llu\n",
               myrank(), t, old_val);
      }
      for (int j = 0; j < 1000; j++) dummy ^= old_val + j;
    }
  }

  barrier();

  if (myrank() == 0) {
    for (int t = 0; t < ranks(); t++) {
      if (counters[t].get().load() != (uint64_t)1000 * ranks()) {
        printf("Verification error, counters[%d]=%llu, but expected %llu\n",
               t, counters[t].get().load(), (uint64_t)1000 *ranks());
      }
    }
  }

  printf("Rank %u passed test_fetch_add!\n", (unsigned int) myrank());

  barrier();
  if (myrank() == 0)
    printf("test_fetch_add passed!\n");

  return 0;
}