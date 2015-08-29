/**
 * \example test_progress_thread.cpp
 *
 * Test asynchronous task execution by the progress thread
 */

#include <upcxx.h>
#include <iostream>
#include <unistd.h>

using namespace upcxx;

shared_var<int> guard = 0;

void print_task(int n)
{
  printf("Print_Task execution on Rank %u n %d\n", myrank(), n);
}

int main(int argc, char **argv)
{
  upcxx::init(&argc, &argv);

  printf("Rank %u will spawn %d tasks...\n",
         myrank(), ranks());

  for (uint32_t i = 0; i < ranks(); i++) {

#ifndef UPCXX_HAVE_CXX11
    printf("Rank %u calls a named function on rank %d\n", myrank(), i);
    async(i)(print_task, 1000+i);
#else
    printf("Rank %u calls a lambda function on rank %d\n", myrank(), i);
    async(i)([=] () { printf("Lambda function executing on Rank %u n %d\n", myrank(), 1000+i); });
#endif
  }

  printf("Rank %u will fall asleep for 1 second and no async tasks should happen before it wakes up...\n",
         myrank());
  sleep(1);
  printf("Rank %u wakes up.\n", myrank());
  async_wait();
  fflush(stdout);

  if (guard == 1) {
    fprintf(stderr, "Barrier error: Rank %u sees someone passes the barrier without waiting!\n",
            myrank());
    exit(1);
  }

  barrier();
  guard = 1;

  if (myrank() == 0) printf("\n\nStart to test progress thread...\n\n");
  barrier();

  for (uint32_t i = 0; i < ranks(); i++) {

#ifndef UPCXX_HAVE_CXX11
    printf("Rank %u calls a named function on rank %d\n", myrank(), i);
    async(i)(print_task, 1000+i);
#else
    printf("Rank %u calls a lambda function on rank %d\n", myrank(), i);
    async(i)([=] () { printf("Lambda function executing on Rank %u n %d\n", myrank(), 1000+i); });
#endif
  }

  printf("Rank %u will fall asleep for 1 second and async tasks should continue to be executed by the progress thread...\n",
         myrank());
  progress_thread_start();
  sleep(1);
  progress_thread_stop();
  printf("Rank %u wakes up.\n", myrank());

  barrier();
  if (myrank() == 0) printf("\ntest_progress_thread done.\n");

  upcxx::finalize();
  return 0;
}
