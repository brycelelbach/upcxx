/**
 * \example test_dynamic_finish.cpp
 *
 * Test dynamically scoped finish
 *
 */

#include <upcxx.h>
#include <finish.h>
#include <forkjoin.h> // for single-thread execution model emulation

using namespace upcxx;

const int task_iters = 1000000;

shared_var<int> task_count[3];
shared_lock count_lock[3];

int task(int group)
{
  int r = 3;
  for (int i = 0; i < task_iters; i++)
    r = (r << 8) + r; // eat up some time
  count_lock[group].lock();
  task_count[group] = task_count[group] + 1;
  count_lock[group].unlock();
  return r;
}

int spawn_tasks()
{
  finish {
    int spawned = 0;
    for (int i = 0; i < THREADS; i++) {
      printf("thread %d spawns a task at node %d\n", MYTHREAD, i);
      async(i)(task, 2);
      spawned += 1;
    }
    return spawned;
  }
  return -1; // unreachable
}

int main(int argc, char **argv)
{
  printf("MYTHREAD %d will spawn %d tasks...\n",
         MYTHREAD, 3*THREADS);

  int spawned = 0;
  for (int i = 0; i < 3; i++)
    task_count[i] = 0;

  finish {
    for (int i = 0; i < THREADS; i++) {
      printf("thread %d spawns a task at node %d\n", MYTHREAD, i);
      async(i)(task, 0);
      spawned += 1;
    }

    printf("group %d complete? %d\n", 0, task_count[0] == THREADS);

    finish {
      for (int i = 0; i < THREADS; i++) {
        printf("thread %d spawns a task at node %d\n", MYTHREAD, i);
        async(i)(task, 1);
        spawned += 1;
      }
    }

    printf("group %d complete? %d\n", 1, task_count[1] == THREADS);

    spawned += spawn_tasks();

    printf("group %d complete? %d\n", 2, task_count[2] == THREADS);
  }

  printf("group %d complete? %d\n", 0, task_count[0] == THREADS);

  printf("All async tasks are done.\n");

  return 0;
}
