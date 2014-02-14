/**
 * \example test_event2.cpp
 *
 * Test the functions of events
 * + Test multiple dependencies for event completion
 * + Test callback functions after event completion
 *
 */

#include <upcxx.h>
#include <iostream>
#include <stdio.h>

using namespace upcxx;
using namespace std;

void compute_task(int i, event *e, global_ptr<double> src, global_ptr<double> dst, size_t sz)
{
  printf("MYTHREAD %d: finished async_copy, src[%lu] = %f, dst[%lu] = %f\n",
         MYTHREAD, sz-1, src[sz-1].get(), sz-1, dst[sz-1].get());
  assert(e->isdone());
  cout << "MYTHREAD: " << MYTHREAD << " starts computing...\n";
}

int main(int argc, char **argv)
{
  init(&argc, &argv);

  global_ptr<double> src, dst;
  size_t sz = 1024*1024;
  src = allocate<double>((MYTHREAD+1)%THREADS, sz);
  assert(src != NULL);
  dst = allocate<double>(MYTHREAD, sz);
  assert(dst != NULL);
  src[sz-1] = 123456.789 + (double)MYTHREAD;
  dst[sz-1] = 0.0;
    
  barrier();

  event copy_e;
  event compute_e;
  async_task cb = async_task(MYTHREAD, // callee id
                             MYTHREAD, // caller id
                             &compute_e, // ack event
                             compute_task, MYTHREAD*100, &copy_e, src, dst, sz);
  submit_task(&cb, &copy_e);

  printf("MYTHREAD %d starts async_copy...\n", MYTHREAD);

  async_copy(src, dst, sz, &copy_e);

  copy_e.wait();

  compute_e.wait();

  printf("MYTHREAD %d finishes computing...\n", MYTHREAD);

  finalize();

  return 0;
}
