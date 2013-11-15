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
   * \ingroup asyncgroup
   * Poll the task queue and process any asynchronous
   * tasks received. progress() cannot be called in any GASNet AM
   * handlers because it may call gasnet_AMPoll().
   */
  int progress();

  /**
   * \ingroup asyncgroup
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
