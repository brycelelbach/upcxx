/*
 * testperf2: test the performance of statically allocated global
 * variables and locks
 *
 * 1) use statically allocate global shared variables
 * 2) use locks to avoid data races for shared variables
 *
 */

#include <upcxx.h>

#include <iostream>
#include <cassert>

using namespace upcxx;
using namespace std;

#define TIME() gasnett_ticks_to_us(gasnett_ticks_now()) 

shared_var<int> sv = 0;
shared_lock sl;

int main (int argc, char **arsv)
{
  init(&argc, &arsv);

  int niters = 100;
  int64_t start_time, total_time1, total_time2, total_time3;

  barrier();

  // shared data access with a lock
  total_time1 = 0;
  total_time2 = 0;
  total_time3 = 0;
  for (int i = 0; i < niters; i++) {
    start_time = TIME();
    sl.lock();
    total_time1 += TIME() - start_time;
    start_time = TIME();
    sv = sv + 1;
    total_time2 += TIME() - start_time;
    start_time = TIME();
    sl.unlock();
    total_time3 += TIME() - start_time;
  }
  barrier();
  if (MYTHREAD == 0) {
    assert((int)sv == niters * THREADS);
  }
  
  printf("MYTHREAD %d: lock time %lg (us), unlock time % lg (us), shared variable access time (read+write) %lg (us)\n",
         MYTHREAD, (double)total_time1/niters, (double)total_time3/niters, (double)total_time2/niters);

  barrier();
  finalize();

  return 0;
}
