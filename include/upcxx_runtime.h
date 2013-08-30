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
