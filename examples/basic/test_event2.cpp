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
  printf("Rank %d Step 4 finishes computing...\n", myrank());
}

void compute_task(int i, event *e, global_ptr<double> src, global_ptr<double> dst, size_t sz)
{
  printf("Rank %d Step 2 finishes async_copy, src[%lu] = %f, dst[%lu] = %f\n",
         myrank(), sz-1, src[sz-1].get(), sz-1, dst[sz-1].get());
  assert(e->isdone());
  cout << "Rank " << myrank() << " Step 3 starts computing...\n";
  sleep(1);
}

int main(int argc, char **argv)
{
  global_ptr<double> src, dst;
  size_t sz = 1024*1024;
  src = allocate<double>((myrank()+1)%ranks(), sz);
#ifdef UPCXX_HAVE_CXX11
  assert(src != nullptr); // assert(src != NULL); is OK but not type-safe
#else  
  assert(src.raw_ptr() != NULL); // or assert(!src.isnull());
#endif
  
  dst = allocate<double>(myrank(), sz);
#ifdef UPCXX_HAVE_CXX11
  assert(dst != nullptr); // assert(dst != NULL); is OK but not type-safe
#else
  assert(dst.raw_ptr() != NULL); // or assert(!dst.isnull());
#endif
  
  src[sz-1] = 123456.789 + (double)myrank();
  dst[sz-1] = 0.0;
    
  barrier();

  event copy_e, compute_e;

  printf("Rank %d Step 1 starts async_copy...\n", myrank());
  async_copy(src, dst, sz, &copy_e);
  async_after(myrank(), &copy_e, &compute_e)(compute_task, myrank()*100, &copy_e, src, dst, sz);
  async_after(myrank(), &compute_e)(done_compute);

  async_wait(); // wait for all tasks to be done!

  printf("Rank %d Step 5 finishes waiting...\n", myrank());

  barrier();
  if (myrank() == 0)
    printf("test_event2 passed!\n");

  return 0;
}
