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
#ifdef __CUDACC__
#include <gasnet_extended_gpu.h>
#endif

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
};

#endif // GASNET_API_H_
