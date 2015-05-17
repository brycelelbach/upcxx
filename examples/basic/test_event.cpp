/**
 * \example test_event.cpp
 *
 * Test the functions of events
 * + Test multiple dependencies for event completion
 * + Test callback functions after event completion
 *
 */

#include <upcxx.h>
#include <forkjoin.h> // for single-thread execution model emulation

#include <iostream>
#include <stdio.h>

using namespace std;
using namespace upcxx;

void callback()
{
  cout << "All events are completed, myrank: " << myrank();
  cout << "\n";
}

void print_task(int task_id)
{
  cout << "myrank: " << myrank() <<  ", task_id: " << task_id << "\n";
}

int main(int argc, char **argv)
{
  event e;
  async_task cb = async_task(myrank(),
                             myrank(),
                             NULL,
                             callback);
  submit_task(&cb, &e);

  cerr << e << "\n";

  printf("Node %d will spawn %d tasks...\n",
         myrank(), ranks());

  for (rank_t i = 0; i < ranks(); i++) {
    printf("Node %d spawns a task at place %d\n",
           myrank(), i);

    async(i, &e)(print_task, 1000+i);
  }

  e.wait();

  if (myrank() == 0)
    printf("test_event passed!\n");

  return 0;
}
