/**
 * collective.h - provide global collective operations
 * \see team.h for team collective operations
 */

#pragma once

#include <assert.h>

#include "gasnet_api.h"
#include "upcxx_types.h"
#include "coll_flags.h"
#include "allocate.h"

namespace upcxx
{
  // Defined in team.cpp but we don't want to include team.h here due to
  // cyclic include dependencies
  extern gasnet_team_handle_t current_gasnet_team();

  template<class T>
  void reduce(T *src, T *dst, size_t count, uint32_t root,
              upcxx_op_t op, upcxx_datatype_t dt)
  {
    // YZ: check consistency of T and dt
    gasnet_coll_reduce(current_gasnet_team(), root, dst, src, 0, 0,
                       sizeof(T), count, dt, op,
                       UPCXX_GASNET_COLL_FLAG);
  }
  #define upcxx_reduce upcxx::reduce
  
  void init_collectives();
  
  static inline void bcast(void *src, void *dst, size_t nbytes, uint32_t root)
  {
    gasnet_coll_broadcast(current_gasnet_team(), dst, root, src,
                          nbytes, UPCXX_GASNET_COLL_FLAG);
  }
  #define upcxx_bcast upcxx::bcast
  
  static inline void gather(void *src, void *dst, size_t nbytes, uint32_t root)
  {
    gasnet_coll_gather(current_gasnet_team(), root, dst, src, nbytes,
                       UPCXX_GASNET_COLL_FLAG);
  }
  #define upcxx_gather upcxx::gather
  
  static inline void allgather(void *src, void *dst, size_t nbytes)
  {
    // gasnet_coll_gather_all(current_gasnet_team(), dst, src, nbytes,
    //                        UPCXX_GASNET_COLL_FLAG);
    void *temp = (void *)allocate(nbytes * gasnete_coll_team_size(current_gasnet_team()));
    assert(temp != NULL);
    upcxx::gather(src, temp, nbytes, 0);
    upcxx::bcast(temp, dst, nbytes * gasnete_coll_team_size(current_gasnet_team()), 0);
    deallocate(temp);
  }
  #define upcxx_allgather upcxx::allgather
  
  static inline void alltoall(void *src, void *dst, size_t nbytes)
  {
    gasnet_coll_exchange(current_gasnet_team(), dst, src, nbytes,
                         UPCXX_GASNET_COLL_FLAG);
  }
  #define upcxx_alltoall upcxx::alltoall
} // end of namespace upcxx
