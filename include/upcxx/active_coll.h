/**
 * active_coll.h - Active Message Broadcast
 *
 */

#pragma once

#include <iostream>
#include <cassert>
#include <string.h>
#include <stdlib.h>

#include "event.h"
#include "range.h"

// #define DEBUG

namespace upcxx 
{    
  typedef void* (*generic_fp)(void *);

#define MAX_AM_BCAST_PACKET_SIZE 4096

  /// \cond SHOW_INTERNAL
  /**
   * \ingroup internalgroup Internal API
   * This group of API is intended for internal use within the UPCXX lib.
   * @{
   */

  /**
   *
   * Initiate Active Message Broadcast
   *
   * \param [in] target the set of target nodes
   * \param [in] ack the acknowledgment event
   * \param [in] fp the pointer to the function to be executed
   * \param [in] arg_sz the size of the function arguments in bytes
   * \param [in] args the pointer to the function arguments
   * \param [in] after the dependent event
   * \param [in] root_index the node id of the AM bcast root
   */
  void am_bcast(range target,
                event *ack,
                generic_fp fp,
                size_t arg_sz,
                void * args,
                event *after,
                int root_index);

   /** @} */ // end of internalgroup
  /// \endcond
} // namespace upcxx

