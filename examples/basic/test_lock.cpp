/**
 * \example test_locks.cpp
 *
 * test shared_lock
 */

#include <upcxx.h>
#include <iostream>
#include <unistd.h> // for sleep, usleep

using namespace std;
using namespace upcxx;

shared_lock sl;

shared_array<shared_lock> sl_arr;
shared_array<int> sd_arr;

void print_task(int task_id, int arg2)
{
  sl.lock();
  cout << "myrank() " << myrank() <<  ": task_id:" << task_id
       << " arg2: " << arg2 << endl;
  usleep(1000);
  sl.unlock();
}

int main(int argc, char **argv)
{
  upcxx::init(&argc, &argv);
  sl_arr.init(ranks());
  sd_arr.init(ranks());


  printf("Rank %u of %u starts test_lock...\n",
         myrank(), ranks());

  barrier();

  for (uint32_t i = 0; i < 10; i++) {
    print_task(1000+i, 456);
  }

  barrier();

  // sd_arr.init(ranks());
  sd_arr[myrank()] = 0;

  // sl_arr.init(ranks());

  // initialize the lock with "placement new"
  new (sl_arr[myrank()].raw_ptr()) shared_lock(myrank());

  barrier();

  for (uint32_t i = 0; i< 100; i++) {
    (&sl_arr[myrank()])->lock();
    sd_arr[myrank()] += 1;
    (&sl_arr[myrank()])->unlock();

    sl_arr[(myrank()+1)%ranks()].get().lock();
    sd_arr[(myrank()+1)%ranks()] += 1;
    sl_arr[(myrank()+1)%ranks()].get().unlock();

    sl_arr[(myrank()+2)%ranks()].get().lock();
    sd_arr[(myrank()+2)%ranks()] += 1;
    sl_arr[(myrank()+2)%ranks()].get().unlock();
  }

  barrier();

  if (sd_arr[myrank()] != 300) {
    printf("Error: sd_arr[%u] is %d, but is expected to be %d\n",
           myrank(), sd_arr[myrank()].get(), 300);
  }

  barrier();

  if (myrank() == 0) {
    printf("test_lock passed!\n");
  }

  upcxx::finalize();

  return 0;
}
