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
// #include <forkjoin.h> // for single-thread execution model emulation

#include <iostream>
#include <string.h>

using namespace std;
using namespace upcxx;

shared_lock sl;

struct my_string {
  char s[32];
  // copy-by-vaule constrcutor is necessary for remote async!
  my_string(const char *src) { strncpy (s, src, 32); }
};

void print_task(int task_id, my_string arg2)
{
  sl.lock();
  cout << "myrank() " << myrank() <<  ": task_id:" << task_id
       << " arg2: " << arg2.s << endl;
  cout.flush();
  sl.unlock();
}

int main(int argc, char **argv)
{
  init(&argc, &argv);
  for (int i = 0; i < ranks(); i++) {
    if (myrank() == i) {
      printf("Node %d spawns %d tasks with AM bcast...\n",
             myrank(), ranks());
      
      range all(0, ranks());
      
      cout << "all nodes: " << all << "\n";
      
      async(all)(print_task, 123, my_string("all nodes"));
      
      upcxx::async_wait();
            
      range odd(1, ranks(), 2);
      
      cout << "Odd nodes: " << odd << "\n";
      
      async(odd)(print_task, 456, my_string("odd nodes"));
      
      upcxx::async_wait();
      
      range even(0, ranks(), 2);
      
      cout << "Even nodes: " << even << "\n";
      
      async(even)(print_task, 789, my_string("even nodes"));
      
      upcxx::async_wait();
    }
    barrier();
  }
  if (myrank() == 0) {
    cout << "test_am_bcast done.\n";
  }
  finalize();
  return 0;
}
