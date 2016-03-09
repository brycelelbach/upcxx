/*
 * upcxx_internal.h -- UPC++ runtime internal interface, which should only be
 * used inside UPC++ runtime implementation but not in any UPC++ applications
 */

#pragma once

#define ONLY_MSPACES 1 // only use mspace from dl malloc
#include "dl_malloc.h"

#include <stdlib.h>
#include <assert.h>
#include "gasnet_api.h"

namespace upcxx 
{
  /// \cond SHOW_INTERNAL
  struct alloc_am_t
  {
    size_t nbytes;
    void **ptr_addr;
    event *cb_event;
  };

  struct alloc_reply_t
  {
    void **ptr_addr;
    void *ptr;
    event *cb_event;
  };

  struct free_am_t
  {
    void *ptr;
  };

  struct inc_am_t
  {
    void *ptr;
  };
  /// @endcond SHOW_INTERNAL

  /**
   * @ingroup internal
   * Advance the outgoing task queue by processing local tasks
   *
   * Note that some local tasks may take
   *
   * @param max_dispatched the maximum number of tasks to be processed
   *
   * @return the number of tasks that have been processed
   */
  int advance_out_task_queue(queue_t *outq, int max_dispatched);

  inline int advance_out_task(int max_dispatched = MAX_DISPATCHED_OUT)
  {
    return advance_out_task_queue(out_task_queue, max_dispatched);
  }

  /*
   * @ingroup internal
   * Advance the incoming task queue by sending out remote task requests
   *
   * Note that advance_out_task_queue() shouldn't be be called in
   * any GASNet AM handlers because it calls gasnet_AMPoll() and
   * may result in a deadlock.
   *
   * @param max_dispatched the maximum number of tasks to send
   *
   * @return the number of tasks that have been sent
   */
  int advance_in_task_queue(queue_t *inq, int max_dispatched);

  inline int advance_in_task(int max_dispatched = MAX_DISPATCHED_IN)
  {
    return advance_in_task_queue(in_task_queue, max_dispatched);
  }
  
  // AM handler functions
  void async_am_handler(gasnet_token_t token, void *am, size_t nbytes);
  void async_done_am_handler(gasnet_token_t token, void *am, size_t nbytes);
  void alloc_cpu_am_handler(gasnet_token_t token, void *am, size_t nbytes);
  void alloc_gpu_am_handler(gasnet_token_t token, void *am, size_t nbytes);
  void alloc_reply_handler(gasnet_token_t token, void *reply, size_t nbytes);
  void free_cpu_am_handler(gasnet_token_t token, void *am, size_t nbytes);
  void free_gpu_am_handler(gasnet_token_t token, void *am, size_t nbytes);
  void inc_am_handler(gasnet_token_t token, void *am, size_t nbytes);

  MEDIUM_HANDLER_DECL(copy_and_signal_request, 4, 8);
  SHORT_HANDLER_DECL(copy_and_signal_reply, 2, 4);
  MEDIUM_HANDLER_DECL(am_request2, 3, 4);
  MEDIUM_HANDLER_DECL(am_request4, 5, 6);
  MEDIUM_HANDLER_DECL(am_request2p2i, 5, 8);

  enum {
    gasneti_handleridx(copy_and_signal_request) = COPY_AND_SIGNAL_REQUEST, 
    gasneti_handleridx(copy_and_signal_reply) = COPY_AND_SIGNAL_REPLY, 
    gasneti_handleridx(am_request2) = GENERIC_AM_REQUEST2,
    gasneti_handleridx(am_request4) = GENERIC_AM_REQUEST4,
    gasneti_handleridx(am_request2p2i) = GENERIC_AM_REQUEST2P2I,
  };
  
  //typedef struct {
  //  void *addr;
  //  uintptr_t size;
  //} gasnet_seginfo_t;

  // int gasnet_getSegmentInfo (gasnet_seginfo_t *seginfo table, int numentries);

  // This may need to be a thread-private data if GASNet supports per-thread segment in the future
  extern gasnet_seginfo_t *all_gasnet_seginfo;
  extern gasnet_seginfo_t *my_gasnet_seginfo;
  extern mspace _gasnet_mspace;
  extern gasnet_nodeinfo_t *all_gasnet_nodeinfo;
  extern gasnet_nodeinfo_t *my_gasnet_nodeinfo;
  extern gasnet_node_t my_gasnet_supernode;


  extern int env_use_am_for_copy_and_set; // defined in upcxx_runtime.cpp
  extern int env_use_dmapp; // defined in upcxx_runtime.cpp

  void gasnet_seg_free(void *p);
  void *gasnet_seg_memalign(size_t nbytes, size_t alignment);
  void *gasnet_seg_alloc(size_t nbytes);

  // Return the supernode of node n in GASNet
  static inline gasnet_node_t gasnet_supernode_of(gasnet_node_t n)
  {
    assert(all_gasnet_seginfo != NULL);
    return all_gasnet_nodeinfo[n].supernode;
  }

#if GASNET_PSHM
  static inline uintptr_t gasnet_pshm_offset(gasnet_node_t n)
  {
    assert(all_gasnet_nodeinfo != NULL);
    return all_gasnet_nodeinfo[n].offset;
  }
#endif
  
  void init_pshm_teams(const gasnet_nodeinfo_t *nodeinfo_from_gasnet,
                       uint32_t num_nodes);
} // namespace upcxx
