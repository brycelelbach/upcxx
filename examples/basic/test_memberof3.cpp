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

  pointerArray.init(THREADS);

  pointerArray[myrank()] = allocate<TaskQueue>(myrank(), 1); 

  memberof(pointerArray[1].get(), tid) = 1;
  int tid = memberof(pointerArray[0].get(), tid);

  finalize();
  return 0;
}
