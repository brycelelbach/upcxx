#pragma once

/**
 * gasnet_api.h - export the GASNet C API to C++ programs
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include <gasnet.h>
#include <gasnet_tools.h>
#include <gasnet_coll.h>
#include <gasnet_handler.h> // for GASNET macros

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

#ifdef __cplusplus
} // end of extern "C"
#endif

// In gasnet_team_coll.h
void gasnete_coll_team_free(gasnet_team_handle_t team);

// Return true if process n is in the same supernode as the calling process
bool upcxx_gasnet_pshm_in_supernode(gasnet_node_t n);

// Return local version of remote in-supernode address if the data
// pointed to is on the same supernode (shared-memory node)
void *upcxx_gasnet_pshm_addr2local(gasnet_node_t n, void *addr);

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
  INC_AM,           // remote increment
  FETCH_ADD_U64_AM,  // fetch and add for uint64_t
  FETCH_ADD_U64_REPLY, // reply message for FETCH_ADD_U64_AM
  COPY_AND_SIGNAL_REQUEST, // transfer data and signal a remote event
  COPY_AND_SIGNAL_REPLY,   // reply a COPY_AND_SIGNAL_REQUEST
  
  /* array_bulk.c */
  ARRAY_MISC_DELETE_REQUEST,
  ARRAY_MISC_ALLOC_REQUEST,
  ARRAY_MISC_ALLOC_REPLY,
  ARRAY_ARRAY_ALLOC_REQUEST,
  ARRAY_STRIDED_PACK_REQUEST,
  ARRAY_STRIDED_PACK_REPLY,
  ARRAY_STRIDED_UNPACKALL_REQUEST,
  ARRAY_STRIDED_UNPACK_REPLY,
  ARRAY_STRIDED_UNPACKDATA_REQUEST,
  ARRAY_STRIDED_UNPACKONLY_REQUEST,
  ARRAY_SPARSE_SIMPLESCATTER_REQUEST,
  ARRAY_SPARSE_DONE_REPLY,
  ARRAY_SPARSE_GENERALSCATTER_REQUEST,
  ARRAY_SPARSE_SIMPLEGATHER_REQUEST,
  ARRAY_SPARSE_SIMPLEGATHER_REPLY,
  ARRAY_SPARSE_GENERALGATHER_REQUEST,
};
