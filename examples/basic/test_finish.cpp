/**
 * \example test_new_async.cpp
 *
 * Test the asynchronous task execution with finish
 *
 */

#include <upcxx.h>
#include <upcxx/finish.h>

#include <iostream>

using namespace std;
using namespace upcxx;

void print_task(int task_id)
{
  cout << "myrank() " << myrank() <<  ": task_id " << task_id << "\n";
}

int main(int argc, char **argv)
{
  upcxx::init(&argc, &argv);
  printf("myrank() %d will spawn %d tasks...\n",
         myrank(), ranks());

  upcxx_finish {
    for (uint32_t i = 0; i < ranks(); i++) {
      printf("Rank %d spawns a task at node %d\n", myrank(), i);
      async(i)(print_task, 1000+i);
    }
  }

  upcxx::barrier();

  if (myrank() == 0) {
    printf("All async tasks are done.\n");
  }

  upcxx::finalize();

  return 0;
}
