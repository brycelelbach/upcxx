#include <upcxx.h>

#include "dmapp_helper.h"

dmapp_seg_desc_t upcxx_dmapp_myseg;
dmapp_seg_desc_t *upcxx_dmapp_segs;
dmapp_rma_attrs_ext_t upcxx_dmapp_rma_args = {0};
// dmapp_queue_handle_t upcxx_dmapp_queue;

// Init Cray DMAPP for UPC++ w. GASNet
// Should be called after gasnet_attach but before using any DMAPP features
void upcxx::init_dmapp()
{
  dmapp_rma_attrs_ext_t rma_args = {0};

  /* Set DMAPP RMA parameters. */
  rma_args.put_relaxed_ordering = DMAPP_ROUTING_ADAPTIVE;
  rma_args.get_relaxed_ordering = DMAPP_ROUTING_ADAPTIVE;
  rma_args.max_outstanding_nb = DMAPP_MAX_OUTSTANDING_NB; // DMAPP_DEF_OUTSTANDING_NB;
  rma_args.offload_threshold = 2048; // DMAPP_OFFLOAD_THRESHOLD;

#if defined(GASNET_SEQ)
  rma_args.max_concurrency = 1; // DMAPP_MAX_THREADS_UNLIMITED;
  #else
  rma_args.max_concurrency = DMAPP_MAX_THREADS_UNLIMITED;
  #endif

  rma_args.PI_ordering = DMAPP_PI_ORDERING_RELAXED;
  rma_args.queue_depth = DMAPP_QUEUE_DEFAULT_DEPTH; // might be smaller
  rma_args.queue_nelems = 1024; // 512 doesn't work??
  // rma_args.queue_flags = DMAPP_QUEUE_ASYNC_PROGRESS;

  DMAPP_SAFE(dmapp_init_ext(&rma_args, &upcxx_dmapp_rma_args));

  // \Todo Check the actual attributes!!
  // printf("gasnetc_dmapp_rma_args.queue_nelems: %u\n", gasnetc_dmapp_rma_args.queue_nelems);

  void *segbase = upcxx::my_gasnet_seginfo->addr;
  uintptr_t segsize = upcxx::my_gasnet_seginfo->size;

  upcxx_dmapp_segs =
    (dmapp_seg_desc_t *)malloc(sizeof(dmapp_seg_desc_t) * gasneti_nodes);
  DMAPP_SAFE(dmapp_mem_register(segbase, segsize, &upcxx_dmapp_myseg));

  upcxx::upcxx_allgather(&upcxx_dmapp_myseg,
                         upcxx_dmapp_segs,
                         sizeof(dmapp_seg_desc_t));
}
