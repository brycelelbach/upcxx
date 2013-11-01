/**
 * \example hello.cpp
 *
 * Simple "hello world" example
 *
 */

#include <upcxx.h>
#include <iostream>

int main (int argc, char **argv)
{
  upcxx::init(&argc, &argv);
  std::cout << "I'm thread " << MYTHREAD << " of " << THREADS << " threads \n";
  upcxx::finalize();
  return 0;
}
