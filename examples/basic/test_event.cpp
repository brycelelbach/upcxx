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

using namespace upcxx;

void callback()
{
  cout << "All events are completed, my node: " << my_node;
  cout << "\n";
}

void print_task(int task_id)
{
  cout << "my node: " << my_node <<  ", task_id: " << task_id << "\n";
}

int main(int argc, char **argv)
{
  event e;
  async_task cb = async_task(my_node.id(),
                             my_node.id(),
                             NULL,
                             callback);
  submit_task(cb, &e);

  cerr << e << "\n";

  printf("Node %d will spawn %d tasks...\n",
         my_node.id(), global_machine.node_count());

  for (int i = 0; i < global_machine.node_count(); i++) {
    printf("Node %d spawns a task at place %d\n",
           my_node.id(), i);

    async(i, &e)(print_task, 1000+i);
  }

  e.wait();

  return 0;
}
