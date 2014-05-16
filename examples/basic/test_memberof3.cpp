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

  if (THREADS < 2) {
    std::cerr << "test requires at least 2 threads" << std::endl;
    finalize();
    return 1;
  }

  pointerArray.init(THREADS);

  pointerArray[myrank()] = allocate<TaskQueue>(myrank(), 1); 

  memberof(pointerArray[1].get(), tid) = 1;
  int tid0 = memberof(pointerArray[0].get(), tid);
  int tid1 = memberof(pointerArray[1].get(), tid);

  std::cout << MYTHREAD << ": results are " << tid0 << ", " << tid1
            << std::endl;

  finalize();
  return 0;
}
