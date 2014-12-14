/*
 * Active collective operations
 */

#include <iostream>

#include <stdio.h>
#include <stdlib.h> // for malloc
#include <string.h> // for memcpy
#include <assert.h> // for assert
// #include <stdint.h> // for uint32_t

#include <upcxx.h>

// #define DEBUG

using namespace std;

#ifdef DEBUG
upcxx::shared_lock output_lock;
#endif

namespace upcxx
{
  /*
   * Launch a group of tasks with a binomial tree algorithm
   * void am_bcast_launch(node_range_t &target, kernel_t kernel,
   *                      void *async_args, size_t arg_sz)
   *
   */
  void am_bcast_launch(void *args)
  {
    am_bcast_arg *bcast_arg = (am_bcast_arg *) args;
    uint32_t root_index = bcast_arg->root_index;

    // If am_bcast_launch is called from a remote node, I need to
    // create a new event locally and add an event callback function
    // to signal the originator's event when the local event is
    // done.

    // Recursively broadcast to the right half of the node partition
    upcxx::range new_target = bcast_arg->target;
    async_task &task = bcast_arg->task;
    upcxx::range original_target = new_target;
    bool islocal = bcast_arg->target.contains(myrank());

#ifdef DEBUG0
    std::cerr << myrank() << " in am_bcast_launch, target "
              << bcast_arg->target << " task " << task << "\n";
#endif

    if (original_target[0] != myrank()) {
      async_task bcast_task;
      bcast_task.init_async_task(myrank(),
                                 bcast_arg->target[0],  // the first node
                                 NULL,
                                 am_bcast_launch,  // bcast kernel
                                 bcast_arg->nbytes(),
                                 bcast_arg);
      submit_task(&bcast_task);
      return;
    }

    
    while (new_target.count() > 1) {
#ifdef DEBUG
      output_lock.lock();
      std::cerr << "bcast to my right: new target " << new_target
                << "\n new_target[0] " << new_target[0] << "\n";
      output_lock.unlock();
#endif
      async_task bcast_task;
      bcast_arg->root_index += new_target.count() / 2;
      // am_bcast_arg_t bcast_arg;
      bcast_arg->target = range(new_target.begin() + new_target.count() / 2 * new_target.step(),
                                new_target.end(),
                                new_target.step());
      bcast_task.init_async_task(myrank(),
                                 bcast_arg->target[0],  // the first node
                                 NULL,
                                 am_bcast_launch,  // bcast kernel
                                 bcast_arg->nbytes(),
                                 bcast_arg);
      submit_task(&bcast_task);
      bcast_arg->root_index = root_index;  // restore the root_index;
      new_target._end = bcast_arg->target.begin();
    }

    // If I'm in the targets, insert the task into the async queue.
    // Do this after sending the AM bcast packets to maximize parallelism
#ifdef DEBUG
    output_lock.lock();
    std::cerr << "\nRank " << myrank() << " is in " << original_target << " "
    << original_target.contains(myrank()) << endl;
    output_lock.unlock();
#endif
    
    if (islocal) {
      task._callee = myrank();
      submit_task(&task);
    }
  }  // am_bcast_launch

  void am_bcast(range target,
                event *ack,
                generic_fp fp,
                size_t arg_sz,
                void * args,
                event *after,
                int root_index,
                int use_domain)
  {
    assert(ack != NULL);

    am_bcast_arg bcast_arg;

    /* prepare bcast_arg */
    bcast_arg.target = target;
    bcast_arg.root_index = root_index;
    bcast_arg.use_domain = use_domain;
    bcast_arg.task.init_async_task(myrank(),
                                   myrank(),
                                   ack,
                                   fp,
                                   arg_sz,
                                   args);
    am_bcast_launch(&bcast_arg);
  } // am_bcast

} // name upcxx


