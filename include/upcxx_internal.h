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
  /// \endcond SHOW_INTERNAL

  /**
   * \ingroup internal
   * Advance the incoming task queue by processing local tasks
   *
   * Note that some local tasks may take
   *
   * \param max_dispatched the maximum number of tasks to be processed
   *
   * \return the number of tasks that have been processed
   */
  int advance_out_task_queue(queue_t *outq, int max_dispatched);

  inline int advance_out_task(int max_dispatched = MAX_DISPATCHED_OUT)
  {
    return advance_out_task_queue(out_task_queue, max_dispatched);
  }

  /*
   * \ingroup internal
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


  //typedef struct {
  //  void *addr;
  //  uintptr_t size;
  //} gasnet_seginfo_t;

  // int gasnet_getSegmentInfo (gasnet_seginfo_t *seginfo table, int numentries);

  // This may need to be a thread-private data if GASNet supports per-thread segment in the future
  extern gasnet_seginfo_t *all_gasnet_seginfo;
  extern gasnet_seginfo_t *my_gasnet_seginfo;
  extern mspace _gasnet_mspace;

  static inline void init_gasnet_seg_mspace()
  {
    all_gasnet_seginfo =
        (gasnet_seginfo_t *)malloc(sizeof(gasnet_seginfo_t) * gasnet_nodes());
    assert(all_gasnet_seginfo != NULL);

    int rv = gasnet_getSegmentInfo(all_gasnet_seginfo, gasnet_nodes());
    assert(rv == GASNET_OK);

    my_gasnet_seginfo = &all_gasnet_seginfo[gasnet_mynode()];

    _gasnet_mspace = create_mspace_with_base(my_gasnet_seginfo->addr,
                                             my_gasnet_seginfo->size, 1);
    assert(_gasnet_mspace != 0);

    // Set the mspace limit to the gasnet segment size so it won't go outside.
    mspace_set_footprint_limit(_gasnet_mspace, my_gasnet_seginfo->size);
  }

  // allocate memory from the GASNet network-addressable segment
  // static inline void *gasnet_seg_alloc(size_t nbytes)
  // {
  //   if (_gasnet_mspace == 0) {
  //     init_gasnet_seg_mspace();
  //   }
  //   return mspace_malloc(_gasnet_mspace, nbytes);
  // }

  // free memory
  static inline void gasnet_seg_free(void *p)
  {
    if (_gasnet_mspace == 0) {
      fprintf(stderr, "Error: the gasnet memory space is not initialized.\n");
      fprintf(stderr, "It is likely due to the pointer (%p) was not from hp_malloc().\n",
              p);
      exit(1);
    }
    assert(p != 0);
    mspace_free(_gasnet_mspace, p);
  }

  static inline void *gasnet_seg_memalign(size_t nbytes, size_t alignment)
  {
    if (_gasnet_mspace== 0) {
      init_gasnet_seg_mspace();
    }
    return mspace_memalign(_gasnet_mspace, alignment, nbytes);
  }

  static inline void *gasnet_seg_alloc(size_t nbytes)
  {
    return gasnet_seg_memalign(nbytes, 64);
  }

} // namespace upcxx
