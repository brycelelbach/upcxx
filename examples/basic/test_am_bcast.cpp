/**
 * \example test_am_bcast.cpp
 *
 * test active message broadcast
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

shared_lock sl;

void print_task(int task_id, int arg2)
{
  sl.lock();
  cerr << "MYTHREAD " << MYTHREAD <<  ": task_id:" << task_id
       << " arg2: " << arg2 << endl;
  sl.unlock();
}

int main(int argc, char **argv)
{
  printf("Node %d spawns %d tasks with AM bcast...\n",
         MYTHREAD, THREADS);

  range all(0, THREADS);

  cerr << "all nodes: " << all << "\n";

  async(all)(print_task, 123, 456);

  upcxx::wait();

  printf("Node %d spawns %d tasks sequentially...\n",
         MYTHREAD, THREADS);

  for (int i = 0; i < THREADS; i++) {
    printf("Node %d spawns a task at place %d\n",
           MYTHREAD, i);

    async(i)(print_task, 1000+i, 456);
  }

  upcxx::wait();

  return 0;
}
