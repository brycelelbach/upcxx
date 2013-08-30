/**
 * \example test_future.cpp -- Test futures
 */

#include <iostream>
#include <stdio.h>

#include "degas.h"
#include "startup.h" // for single-thread execution model emulation

void callback()
{
  cout << "All events are completed, my node: " << upcxx::my_node;
  cout << "\n";
}

int task(int task_id)
{
  cout << "my node: " << upcxx::my_node <<  ", task_id: " << task_id << "\n";
}

int main(int argc, char **argv)
{
  future ft;
  async_task cb = async_task(upcxx::my_node.id(),
                             upcxx::my_node.id(),
                             NULL,
                             callback);
  submit_task(cb, &e);

  cerr << e << "\n";

  printf("Node %d will spawn %d tasks...\n",
         upcxx::my_node.id(), upcxx::global_machine.node_count());

  for (int i = 0; i < upcxx::global_machine.node_count(); i++) {
    printf("Node %d spawns a task at place %d\n",
           my_node.id(), i);

    ft = async(i, &e)(task, 1000+i);
  }

  ft.wait();

  cout << "Return value from future: " << ft.get();
  
  return 0;
}
