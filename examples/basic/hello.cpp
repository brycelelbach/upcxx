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
  std::cout << "I'm thread " << upcxx::myrank() << " of "
            << upcxx::ranks() << " threads \n";
  return 0;
}
