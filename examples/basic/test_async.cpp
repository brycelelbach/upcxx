/**
 * \example test_async.cpp
 *
 * Test the asynchronous task execution
 *
 * + spawn tasks from rank 0
 * + use the launcher object template for async statement to generate
 * function argument packing and unpacking
 *
 */

#include <upcxx.h>
#include <iostream>

using namespace upcxx;

void print_task(int n)
{
  printf("Rank %d n %d\n", myrank(), n);
}

int main(int argc, char **argv)
{
  upcxx::init(&argc, &argv);

  if (myrank() == 0) {
    printf("Rank %d will spawn %d tasks...\n",
           myrank(), ranks());

    for (uint32_t i = 0; i < ranks(); i++) {

#ifndef UPCXX_HAVE_CXX11
      printf("Rank %d calls a named function on rank %d\n", myrank(), i);
      async(i)(print_task, 1000+i);
#else
      printf("Rank %d calls a lambda function on rank %d\n", myrank(), i);
      async(i)([=] () { printf("Rank %d n %d\n", myrank(), 1000+i); });
#endif
    }

    async_wait();
  }

  upcxx::finalize();
  return 0;
}
