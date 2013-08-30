/**
 * \example test_async2.cpp
 *
 * Test the asynchronous task execution
 *
 * + use single-thread execution emulation
 * + use the launcher object template for async statement to generate
 * function argument packing and unpacking
 *
 */

#if (__cplusplus < 201103L)
#warning __cplusplus
//#error A C++ 11 compiler is required!
#endif

#include <iostream>
#include <thread>

// #include <future>

#include "degas.h"
#include "startup.h" // for single-thread execution model emulation

int main(int argc, char **argv)
{
  printf("node %d will spawn %d tasks...\n",
         my_node.id(), global_machine.node_count());

  for (int i = 0; i < global_machine.node_count(); i++) {
    printf("node %d spawns a task at node %d\n",
           my_node.id(), i);

    // std::async([] (int num) {
    //            printf("num:  %d\n", num);
    //            }, // end of the lambda function
    //            1000+i // argument to the lambda function
    //            );

    upcxx::async(i)([] (int num) {
                      printf("num:  %d\n", num);
                    }, // end of the lambda function
                    1000+i // argument to the lambda function
                   );
  }
  
  upcxx::wait();

  return 0;
}
