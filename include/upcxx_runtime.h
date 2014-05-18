/**
 * UPC++ runtime interface
 */


/**
 * \section intro Introduction
 */

#ifndef UPCXX_RUNTIME_H_
#define UPCXX_RUNTIME_H_

#ifdef UPCXX_THREAD_SAFE
#include <pthread.h>
#define upcxx_mutex_lock      pthread_mutex_lock
#define upcxx_mutex_unlock    pthread_mutex_unlock
#else
#define upcxx_mutex_lock(X)
#define upcxx_mutex_unlock(X)
#endif

#include <cstring>
#include <cassert>
#include <ios>
#include <stdint.h>

#include "upcxx_types.h"
#include "queue.h"

namespace upcxx
{
  typedef uint32_t rank_t;

  /**
   * \defgroup initgroup Job initialization and query
   * This group of API is for initialization and self-query.
   */

  /** 
   * \ingroup initgroup
   * Initialize the GASNet runtime, which should be called
   * only once at the beginning of the client program.
   *
   * \param argc the pointer to the number of arguments
   * \param argv the pointer to the values of the arguments
   *
   * \see hello.cpp
   */
  int init(int *argc, char ***argv);

  /**
   * \ingroup initgroup
   * Finalize the GASNet runtime, which should be called only
   * once at the end of the client program.
   */
  int finalize();

  rank_t ranks();

  rank_t myrank();

  extern queue_t *in_task_queue;

  extern queue_t *out_task_queue;

  /**
   * Default maximum number of task to dispatch for every time
   * the incoming task queue is polled/advanced.
   */
  #define MAX_DISPATCHED_IN   1

  /**
   * Default maximum number of task to dispatch for every time
   * the outgoing task queue is polled/advanced.  We choose a
   * relatively large number 99 to try to send out remote tasks
   * requests as soon as possible.
   */
  #define MAX_DISPATCHED_OUT  99

  /**
   * \ingroup asyncgroup
   * Advance both the incoming and outgoing task queues
   * Note that advance() shouldn't be be called in any GASNet AM
   * handlers because it calls gasnet_AMPoll() and may result in
   * a deadlock.
   *
   * \param max_in maximum number of incoming tasks to be processed before returning
   * \param max_out maximum number of outgoing tasks to be processed before returning
   *
   * \return the total number of tasks that have been processed in the incoming
   * and outgoing task queues
   */
  int advance(int max_in = MAX_DISPATCHED_IN, int max_out = MAX_DISPATCHED_OUT);

  // An alias of the advance function for backward compatibility.
  // The advance function is recommended.
  inline int progress() { return advance(); }

  // drain() is superceded by avdance(), provided for backward compatibility.
  // The advance function is recommended.
  inline int drain(int max_dispatched = 0)
  {
    return advance(max_dispatched, max_dispatched);
  } 

  /**
   * \ingroup asyncgroup
   * Poll the task queue, returning 0 if it is empty and 1 otherwise.
   * This function does not modify the task queue. However, like
   * progress(), peek() cannot be called in any GASNet AM handlers
   * because it calls gasnet_AMPoll().
   */
  int peek();

  /**
   * \defgroup syncgroup Synchronization primitives
   * Functions for node synchronization
   */

  /**
   * \ingroup syncgroup
   * Barrier synchronization of all nodes
   */
  int barrier();
} // namespace upcxx

#endif /* UPCXX_RUNTIME_H_ */
