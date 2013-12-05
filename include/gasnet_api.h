/**
 * gasnet_api.h - export the GASNet C API to C++ programs
 */

#ifndef GASNET_API_H_
#define GASNET_API_H_

#ifdef _cpluscplus
extern "C"
{
#endif

#include <gasnet.h>
#include <gasnet_tools.h>
#include <gasnet_coll.h>
#include <gasnet_handler.h>

#define THREADS gasnet_nodes()
#define MYTHREAD gasnet_mynode()

#define GASNET_SAFE(fncall) do {                                      \
    int _retval;                                                      \
    if ((_retval = fncall) != GASNET_OK) {                            \
      fprintf(stderr, "ERROR calling: %s\n"                           \
              " at: %s:%i\n"                                          \
              " error: %s (%s)\n",                                    \
              #fncall, __FILE__, __LINE__,                            \
              gasnet_ErrorName(_retval), gasnet_ErrorDesc(_retval));  \
      fflush(stderr);                                                 \
      gasnet_exit(_retval);                                           \
    }                                                                 \
  } while(0)

#define GASNET_BARRIER do {                                 \
    gasnet_barrier_notify(0, GASNET_BARRIERFLAG_ANONYMOUS); \
    /* gasnet:Poll(); // process */                         \
    gasnet_barrier_wait(0, GASNET_BARRIERFLAG_ANONYMOUS);   \
  } while (0)

#ifdef _cpluscplus
} // end of extern "C"
#endif

// In gasnet_team_coll.h
void gasnete_coll_team_free(gasnet_team_handle_t team);

/* GASNET AM functions */
enum am_index_t {
  // 128 is the beginning AM number for GASNet client
  ASYNC_AM=128,     // asynchronous task 
  ASYNC_DONE_AM,    // acknowledge the completion of an async task
  ALLOC_CPU_AM,     // CPU global memory allocation 
  ALLOC_REPLY,      // reply message for ALLOC_CPU_AM and ALLOC_GPU_AM
  FREE_CPU_AM,      // CPU global memory deallocation
  LOCK_AM,          // inter-node lock
  LOCK_REPLY,       // reply message for LOCK_AM
  UNLOCK_AM,        // inter-node unlock
  AM_BCAST,         // active broadcast
  AM_BCAST_REPLY,   // active broadcast reply
  INC_AM,   // active broadcast reply

  /* array_bulk.c */
  gasneti_handleridx(misc_null_reply),
  gasneti_handleridx(misc_delete_request),
  gasneti_handleridx(misc_alloc_request),
  gasneti_handleridx(misc_alloc_reply),
  gasneti_handleridx(strided_pack_request),
  gasneti_handleridx(strided_pack_reply),
  gasneti_handleridx(strided_unpackAll_request),
  gasneti_handleridx(strided_unpack_reply),
  gasneti_handleridx(strided_unpackOnly_request),
  gasneti_handleridx(sparse_simpleScatter_request),
  gasneti_handleridx(sparse_done_reply),
  gasneti_handleridx(sparse_generalScatter_request),
  gasneti_handleridx(sparse_largeScatterNoDelete_request),
  gasneti_handleridx(sparse_simpleGather_request),
  gasneti_handleridx(sparse_simpleGather_reply),
  gasneti_handleridx(sparse_generalGather_request),
  gasneti_handleridx(sparse_largeGather_request),
  gasneti_handleridx(sparse_largeGather_reply),
};

#endif // GASNET_API_H_
