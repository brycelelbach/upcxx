/**
 * \example test_async.cpp
 *
 * Test the asynchronous task execution
 *
 * + use single-thread execution emulation
 * + use the launcher object template for async statement to generate
 * function argument packing and unpacking
 *
 */

#include <upcxx.h>
#include <forkjoin.h> // for single-thread execution model emulation

#include <iostream>

using namespace upcxx;

void print_task(int n)
{
  cout << "MYTHREAD " << MYTHREAD <<  ": n " << n << "\n";
}

int main(int argc, char **argv)
{
  printf("MYTHREAD %d will spawn %d tasks...\n",
         MYTHREAD, THREADS);

  for (int i = 0; i < THREADS; i++) {

#ifndef UPCXX_HAVE_CXX11
    printf("Rank %d calls a named function on rank %d\n", MYTHREAD, i);
    async(i)(print_task, 1000+i);
#else
    printf("Rank %d calls a lambda function on rank %d\n", MYTHREAD, i);
    async(i)([] (int n) 
             { cout << "MYTHREAD " << MYTHREAD <<  ": n " << n << "\n"; },
             1000+i);
#endif
  }

  
  upcxx::wait();

  return 0;
}
