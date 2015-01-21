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

shared_array<shared_lock> sl_arr;
shared_array<int> sd_arr;

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

  barrier();

  sd_arr.init(THREADS);
  sd_arr[MYTHREAD] = 0;

  sl_arr.init(THREADS);

  // initialize the lock with "placement new"
  new (sl_arr[MYTHREAD].raw_ptr()) shared_lock();

  barrier();

  for (uint32_t i = 0; i< 100; i++) {
    (&sl_arr[MYTHREAD])->lock();
    sd_arr[MYTHREAD] += 1;
    (&sl_arr[MYTHREAD])->unlock();

    sl_arr[(MYTHREAD+1)%THREADS].get().lock();
    sd_arr[(MYTHREAD+1)%THREADS] += 1;
    sl_arr[(MYTHREAD+1)%THREADS].get().unlock();

    sl_arr[(MYTHREAD+2)%THREADS].get().lock();
    sd_arr[(MYTHREAD+2)%THREADS] += 1;
    sl_arr[(MYTHREAD+2)%THREADS].get().unlock();
  }

  barrier();

  if (sd_arr[MYTHREAD] != 300) {
    printf("Error: sd_arr[%u] is %d, but is expected to be %d\n",
           MYTHREAD, sd_arr[MYTHREAD].get(), 300);
  }

  barrier();

  if (MYTHREAD == 0) {
    printf("test_lock passed!\n");
  }

  upcxx::finalize();

  return 0;
}
