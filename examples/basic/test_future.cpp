/**
 * \example test_future.cpp
 *
 * Test returning a future value from an asynchronous task execution
 *
 */

#include <upcxx.h>
#include <iostream>

using namespace upcxx;

int task(int n)
{
  printf("Rank %d n %d\n", myrank(), n);
  return (myrank()*1000 + n); 
}

int main(int argc, char **argv)
{
  upcxx::init(&argc, &argv);

  if (myrank() == 0) {

    upcxx::future<int> *all_futures = new upcxx::future<int> [ranks()];
    
    printf("Rank %u will spawn %d tasks...\n",
           myrank(), ranks());

    for (uint32_t i = 0; i < ranks(); i++) {

#ifndef UPCXX_HAVE_CXX11
      printf("Rank %u calls a named function on rank %d\n", myrank(), i);
      all_futures[i] = async(i)(task, i*2);
#else
      printf("Rank %u calls a lambda function on rank %d\n", myrank(), i);
      all_futures[i]
        = async(i)([=]() -> int
          {
            int n = i*2;
            printf("Rank %d n %d\n", myrank(), n);
            return myrank()*1000 + n;
          }
          );
#endif
    }
    
    for (uint32_t i = 0; i < ranks(); i++) {
      printf("Return value from rank %u: %d\n", i, all_futures[i].get());
    }

    delete [] all_futures;
  }

  upcxx::finalize();
  return 0;
}
