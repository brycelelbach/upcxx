/**
 * \example test_locks.cpp
 *
 * test shared_lock
 */

#include <upcxx.h>
#include <iostream>
#include <unistd.h> // for sleep, usleep

using namespace upcxx;

shared_lock sl;

void print_task(int task_id, int arg2)
{
  sl.lock();
  cout << "MYTHREAD " << MYTHREAD <<  ": task_id:" << task_id
       << " arg2: " << arg2 << endl;
  usleep(1000);
  sl.unlock();
}

int main(int argc, char **argv)
{
  upcxx::init(&argc, &argv);

  printf("Rank %u of %u starts test_lock...\n",
         myrank(), ranks());

  barrier();
  
  for (uint32_t i = 0; i < 10; i++) {
    print_task(1000+i, 456);
  }

  upcxx::finalize();

  return 0;
}
