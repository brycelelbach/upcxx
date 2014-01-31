/**
 * UPC++ runtime interface
 */


/**
 * \section intro Introduction
 */

#ifndef UPCXX_RUNTIME_H_
#define UPCXX_RUNTIME_H_

#include <cstring>
#include <cassert>
#include <ios>

#include "upcxx_types.h"
#include "queue.h"

namespace upcxx
{

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
   * Advance the incoming task queue by processing local tasks
   *
   * Note that some local tasks may take
   *
   * \param max_dispatched the maximum number of tasks to be processed
   *
   * \return the number of tasks that have been processed
   */
  int advance_out_task_queue(queue_t *outq,
                             int max_dispatched = MAX_DISPATCHED_OUT);

  /**
   * \ingroup asyncgroup
   * Advance the outgoing task queue by sending out remote task requests
   *
   * Note that advance_out_task_queue() shouldn't be be called in
   * any GASNet AM handlers because it calls gasnet_AMPoll() and
   * may result in a deadlock.
   *
   * \param max_dispatched the maximum number of tasks to send
   *
   * \return the number of tasks that have been sent
   */
  int advance_in_task_queue(queue_t *inq,
                            int max_dispatched = MAX_DISPATCHED_IN);
  /**
   * \ingroup asyncgroup
   * Advance both the incoming and outgoing task queues
   * Note that advance() shouldn't be be called in any GASNet AM
   * handlers because it calls gasnet_AMPoll() and may result in
   * a deadlock.
   *
   * \param max_dispatched
   *
   * \return the total number of tasks that have been processed in the incoming
   * and outgoing task queues
   */
   int advance(int max_in = 0, int max_out = 0);

   int progress();

  /**
   * \ingroup asyncgroup
   *
   * Advance the incoming and outgoing tasks queues
   *
   * Poll the task queue (once) and process any asynchronous
   * tasks received until either the queue is empty or the optional
   * user-specified maximum number of tasks have been processed.
   * Like progress(), drain cannot be called in any GASNet AM
   * handlers because it calls gasnet_AMPoll().
   */
  int drain(int max_dispatched = 0);

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
