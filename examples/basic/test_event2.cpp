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

void done_compute()
{
  printf("myrank() %d finishes computing...\n", myrank());
}

void compute_task(int i, event *e, global_ptr<double> src, global_ptr<double> dst, size_t sz)
{
  printf("myrank() %d: finished async_copy, src[%lu] = %f, dst[%lu] = %f\n",
         myrank(), sz-1, src[sz-1].get(), sz-1, dst[sz-1].get());
  assert(e->isdone());
  cout << "myrank(): " << myrank() << " starts computing...\n";
}

int main(int argc, char **argv)
{
  init(&argc, &argv);

  global_ptr<double> src, dst;
  size_t sz = 1024*1024;
  src = allocate<double>((myrank()+1)%THREADS, sz);
  assert(src != NULL);
  dst = allocate<double>(myrank(), sz);
  assert(dst != NULL);
  src[sz-1] = 123456.789 + (double)myrank();
  dst[sz-1] = 0.0;
    
  barrier();

  event copy_e, compute_e ,done_e;

  async_after(myrank(), &copy_e, &compute_e)(compute_task, myrank()*100, &copy_e, src, dst, sz);
  async_after(myrank(), &compute_e, &done_e)(done_compute);

  printf("Process %d starts async_copy...\n", myrank());

  async_copy(src, dst, sz, &copy_e);

  upcxx::wait(); // wait for all tasks to be done!

  printf("Process %d finishes waiting...\n", myrank());

  finalize();

  return 0;
}
