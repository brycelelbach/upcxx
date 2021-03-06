#include <upcxx.h>
#include <stdio.h>
#include <iostream>

using namespace upcxx;

class TaskQueue
{
public:
  int tid; // thread id of the owner
};

typedef global_ptr<TaskQueue> TaskQueuePtr;
shared_array<TaskQueuePtr> pointerArray;

int main(int argc, char **argv)
{
  init(&argc, &argv);

  if (ranks() < 2) {
    std::cerr << "test requires at least 2 threads" << std::endl;
    finalize();
    return 1;
  }

  pointerArray.init(ranks());

  pointerArray[myrank()] = allocate<TaskQueue>(myrank(), 10); 
  barrier();

  upcxx_memberof(pointerArray[1].get(), tid) = 1;
  int tid0 = upcxx_memberof(pointerArray[0].get(), tid);
  int tid1 = upcxx_memberof(pointerArray[1].get(), tid);

  std::cout << myrank() << ": results are " << tid0 << ", " << tid1
            << std::endl;

  finalize();
  return 0;
}
