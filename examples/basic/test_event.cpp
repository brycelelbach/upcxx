/**
 * \example test_event.cpp
 *
 * Test the functions of events
 * + Test multiple dependencies for event completion
 * + Test callback functions after event completion
 *
 */

#include <upcxx.h>

#include <iostream>
#include <stdio.h>

using namespace std;
using namespace upcxx;

void callback()
{
  cout << "Rank " << myrank() << ": all events are completed.\n";
  cout.flush();
}

void print_task(int task_id)
{
  cout << "myrank: " << myrank() <<  ", task_id: " << task_id << "\n";
  cout.flush();
}

int main(int argc, char **argv)
{
  upcxx::init(&argc, &argv);

  event e1, e2;

  printf("Node %d will spawn %d tasks...\n",
         myrank(), ranks());

  for (rank_t i = 0; i < ranks(); i++) {
    printf("Node %d spawns a task at place %d\n",
           myrank(), i);

    async(i, &e1)(print_task, 1000+i);
  }
  async_after(myrank(), &e1, &e2)(callback);
  e2.wait();

  upcxx::barrier();

  if (myrank() == 0)
    printf("\ntest_event passed!\n");

  upcxx::finalize();
  return 0;
}
