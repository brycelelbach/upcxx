/**
 * collective.h - provide global collective operations
 * \see team.h for team collective operations
 */

#pragma once

#include <assert.h>

#include "gasnet_api.h"
#include "upcxx_types.h"

#define UPCXX_GASNET_COLL_FLAG \
  (GASNET_COLL_IN_MYSYNC | GASNET_COLL_OUT_MYSYNC | GASNET_COLL_LOCAL)

namespace upcxx
{
  template<class T>
  void upcxx_reduce(T *src, T *dst, size_t count, uint32_t root,
                    upcxx_op_t op, upcxx_datatype_t dt)
  {
    // YZ: check consistency of T and dt
    gasnet_coll_reduce(GASNET_TEAM_ALL, root, dst, src, 0, 0, sizeof(T),
                       count, dt, op, UPCXX_GASNET_COLL_FLAG);
  }

  void init_collectives();
  
  static inline void upcxx_bcast(void *src, void *dst, size_t nbytes, uint32_t root)
  {
    gasnet_coll_broadcast(GASNET_TEAM_ALL, dst, root, src, nbytes, 
                          UPCXX_GASNET_COLL_FLAG);
  }
  
  static inline void upcxx_gather(void *src, void *dst, size_t nbytes, uint32_t root)
  {
    gasnet_coll_gather(GASNET_TEAM_ALL, root, dst, src, nbytes,
                       UPCXX_GASNET_COLL_FLAG);
  }
  
  static inline void upcxx_allgather(void *src, void *dst, size_t nbytes)
  {
    gasnet_coll_gather_all(GASNET_TEAM_ALL, dst, src, nbytes,
                           UPCXX_GASNET_COLL_FLAG);
  }

  static inline void upcxx_alltoall(void *src, void *dst, size_t nbytes)
  {
    gasnet_coll_exchange(GASNET_TEAM_ALL, dst, src, nbytes,
                         UPCXX_GASNET_COLL_FLAG);
  }
  
} // end of namespace upcxx
