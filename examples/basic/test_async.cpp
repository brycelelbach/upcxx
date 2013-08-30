/**
 * \example test_async.cpp
 *
 * Test the asynchronous task execution
 *
 * + use single-thread execution emulation
 * + use the launcher object template for async statement to generate
 * function argument packing and unpacking
 *
 */

#include <upcxx.h>
#include <forkjoin.h> // for single-thread execution model emulation

#include <iostream>

using namespace upcxx;

void print_task(int task_id)
{
  cout << "MYTHREAD " << MYTHREAD <<  ": task_id " << task_id << "\n";
}

int main(int argc, char **argv)
{
  printf("MYTHREAD %d will spawn %d tasks...\n",
         MYTHREAD, THREADS);

  for (int i = 0; i < THREADS; i++) {
    printf("thread %d spawns a task at node %d\n", MYTHREAD, i);

    async(i)(print_task, 1000+i);
  }
  
  upcxx::wait();

  return 0;
}
