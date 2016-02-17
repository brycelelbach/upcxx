/**
 * \example test_fetch_add.cpp
 *
 * test atomic fetch-and-add
 */

#include <upcxx.h>
#include <iostream>
#include <inttypes.h>

using namespace upcxx;

shared_array<upcxx::atomic<uint64_t> >counters;

uint64_t dummy = 0;

int main(int argc, char **argv)
{
  upcxx::init(&argc, &argv);
  std::cout << "Rank " << myrank() << " of " << ranks()
            << " starts test_fetch_add...\n";

  counters.init(ranks());
  // counters[myrank()] = 0;

  barrier();

  uint64_t old_val;

  for (int i = 0; i < 100; i++) {
    for (int t = 0; t < ranks(); t++) {
      global_ptr<upcxx::atomic<uint64_t> > obj = &counters[t];
      old_val = fetch_add(obj, 10);
      if (old_val % 1000 == 0) {
        std::cout << "Rank " << myrank() << " t = " << t
                  << " fetch_add old value =  " << old_val << "\n";

      }
      for (int j = 0; j < 1000; j++) dummy ^= old_val + j;
    }
  }

  barrier();

  if (myrank() == 0) {
    for (int t = 0; t < ranks(); t++) {
      if (counters[t].get().load() != (uint64_t)1000 * ranks()) {
        std::cout << "Verification error, counters[ " << t << "]="
                  << counters[t].get().load() << ", but expected " <<
            (uint64_t)1000 *ranks() << "\n";
      }
    }
  }

  std::cout << "Rank " << myrank() << "passed test_fetch_add!\n";

  barrier();
  if (myrank() == 0)
    std::cout << "test_fetch_add passed!\n";

  upcxx::finalize();
  return 0;
}
