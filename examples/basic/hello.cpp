/**
 * \example hello.cpp
 *
 * Simple "hello world" example
 * + Initialize and finalize degas lib
 * + Look up CPU and GPU places in degas lib
 *
 */

#include <upcxx.h>
#include <forkjoin.h>

#include <iostream>

using namespace std;

int main (int argc, char **argv)
{
  cout << "Global Machine " << upcxx::global_machine 
       << " total number of cpus " << upcxx::global_machine.processor_count()
       << "\n";
  
  for (int i = 0; i < upcxx::global_machine.node_count(); i++) {
    upcxx::node n = upcxx::global_machine.get_node(i);
    cout << "\tNode " << n << ": "
         << "num of local cpus " <<  n.processor_count()
         << "\n";
  }
  
  return 0;
}
