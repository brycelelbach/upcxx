/**
 * \file test_global.cpp
 * \example test_global.cpp
 *
 * test_global: test statically allocated global variables
 *
 * + Use statically allocate global shared variables
 * + Use locks to avoid data races for shared variables
 *
 */

#include <upcxx.h>
#include <iostream>

using namespace upcxx;
using namespace std;

shared_var<int> sv = 0;

shared_lock sv_lock;

int main (int argc, char **arsv)
{
  init(&argc, &arsv);
  
  if (MYTHREAD == 0) {
    sv = 1;
  }
  barrier();

  /*
  cout << "MYTHREAD: " << MYTHREAD << " global shared variable ga = " << sv << endl;
  barrier();
  */

  for (uint32_t i=0; i<THREADS; i++) {
    if (MYTHREAD == i) {
      sv = sv + 1;
      cout << "MYTHREAD: " << MYTHREAD << " ga = " << sv << "\n";
      barrier();
    } else {
      barrier();
    }
  }
  
  // test global lock
  barrier();
  sv = 0;
  barrier();

  // shared data access without a lock
  for (int i=0; i<100; i++) {
    sv = sv + 1;
  }
  barrier();
  if (MYTHREAD == 0) {
    printf("shared data access without a lock: expected result %d, actual %d\n",
           100*THREADS, (int)sv);
  }
  barrier();

  sv = 0;
  barrier();

  // shared data access with a lock
  for (int i=0; i<100; i++) {
    sv_lock.lock();
    sv = sv + 1;
    sv_lock.unlock();
  }
  barrier();
  if (MYTHREAD == 0) {
    printf("shared data access with a lock: expected result %d, actual %d\n",
           100*THREADS, (int)sv);
  }
  barrier();

  finalize();

  return 0;
}
