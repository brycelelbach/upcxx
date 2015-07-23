/**
 * \example test_dynamic_finish.cpp
 *
 * Test dynamically scoped finish
 *
 */

#include <upcxx.h>
#include <upcxx/finish.h>

using namespace upcxx;

const int task_iters = 1000000;

shared_var<uint32_t> task_count[3];
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
  upcxx_finish {
    int spawned = 0;
    for (uint32_t i = 0; i < ranks(); i++) {
      printf("thread %d spawns a task at node %d\n", myrank(), i);
      async(i)(task, 2);
      spawned += 1;
    }
    return spawned;
  }
  return -1; // unreachable
}

int main(int argc, char **argv)
{
  upcxx::init(&argc, &argv);

  if (myrank() == 0) {
    printf("myrank() %d will spawn %d tasks...\n",
           myrank(), 3*ranks());

    int spawned = 0;
    for (int i = 0; i < 3; i++)
      task_count[i] = 0;

    upcxx_finish {
      for (uint32_t i = 0; i < ranks(); i++) {
        printf("thread %d spawns a task at node %d\n", myrank(), i);
        async(i)(task, 0);
        spawned += 1;
      }

      printf("group %d complete? %d\n", 0, task_count[0] == ranks());

      upcxx_finish {
        for (uint32_t i = 0; i < ranks(); i++) {
          printf("thread %d spawns a task at node %d\n", myrank(), i);
          async(i)(task, 1);
          spawned += 1;
        }
      }

      printf("group %d complete? %d\n", 1, task_count[1] == ranks());

      spawned += spawn_tasks();

      printf("group %d complete? %d\n", 2, task_count[2] == ranks());
    }

    printf("group %d complete? %d\n", 0, task_count[0] == ranks());

    printf("All async tasks are done.\n");
  }

  upcxx::finalize();
  return 0;
}
