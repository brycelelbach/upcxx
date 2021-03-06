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
  std::cout << "I'm rank " << upcxx::myrank() << " of "
            << upcxx::ranks() << " ranks.\n";
  upcxx::finalize();
  return 0;
}
