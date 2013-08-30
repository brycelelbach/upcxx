/**
 * team.h - implements teams and collective operations for teams
 *
 */

#pragma once

#include <cassert>
#include <cstring>

#include "upcxx_types.h"
#include "upcxx_runtime.h"
#include "gasnet_api.h"
#include "machine.h"
#include "range.h"
#include "collective.h"

/// \cond SHOW_INTERNAL

#define UPCXX_GASNET_COLL_FLAG                                          \
  (GASNET_COLL_IN_MYSYNC | GASNET_COLL_OUT_MYSYNC | GASNET_COLL_LOCAL)

namespace upcxx
{
  struct team {
    team(uint32_t team_id, uint32_t size, uint32_t myrank, uint32_t *members,
         gasnet_team_handle_t gasnet_team = NULL)
      : _team_id(team_id), _size(size), _myrank(myrank), 
        _gasnet_team(gasnet_team)
    {
      assert(members != NULL);
      _members = (uint32_t *)malloc(sizeof(uint32_t) * size);
      assert(_members != NULL);
      memcpy(_members, members, sizeof(uint32_t) * size);
    }

    ~team()
    {
      if (_members) {
        free(_members);
      }
    }

    inline uint32_t myrank() const { return _myrank; }
    
    inline uint32_t size() const { return _size; }
    
    inline uint32_t team_id() const { return _team_id; }
    
    // YZ: We can use a compact format to represent team members and only store
    // log2 number of neighbors on each node
    inline uint32_t member(uint32_t i) const
    {
      assert(i < _size);
      return _members[i];
    }

    inline bool is_team_all()
    {
      return (_team_id == 0); // team_all has team_id 0
    }

    inline uint32_t team_get_global_rank(upcxx::team t, uint32_t team_rank)
    {
      if (_members != NULL) {
        assert(team_rank < _size);
        return _members[team_rank];
      } else if (_member_range != NULL) {
        // if the team members are represented by a range
        
      } else {
        // error: the team is not appropriately initialized
      }

      return -1; 
    }
    
    // void create_gasnet_team();
    int split(uint32_t color, uint32_t relrank, team *&new_team);

    inline int barrier()
    {
      int rv;
      assert(_gasnet_team != NULL);
      gasnet_coll_barrier_notify(_gasnet_team, 0,
                                 GASNET_BARRIERFLAG_ANONYMOUS);
      while ((rv=gasnet_coll_barrier_try(_gasnet_team, 0,
                                         GASNET_BARRIERFLAG_ANONYMOUS))
             == GASNET_ERR_NOT_READY) {
        if (progress() != UPCXX_SUCCESS) { // process the async task queue
          return UPCXX_ERROR;
        }
      }
      assert(rv == GASNET_OK);
      return UPCXX_SUCCESS;

    }

    inline int bcast(void *src, void *dst, size_t nbytes, uint32_t root)
    {
      assert(_gasnet_team != NULL);
      gasnet_coll_broadcast(_gasnet_team, dst, root, src, nbytes,
                            UPCXX_GASNET_COLL_FLAG);
      return UPCXX_SUCCESS;
    }
  
    inline int gather(void *src, void *dst, size_t nbytes, uint32_t root)
    {
      assert(_gasnet_team != NULL);
      gasnet_coll_gather(_gasnet_team, root, dst, src, nbytes, 
                         UPCXX_GASNET_COLL_FLAG);
      return UPCXX_SUCCESS;
    }

    inline int allgather(void *src, void *dst, size_t nbytes)
    {
      assert(_gasnet_team != NULL);
      gasnet_coll_gather_all(_gasnet_team, dst, src, nbytes, 
                             UPCXX_GASNET_COLL_FLAG);
      return UPCXX_SUCCESS;
    }

    template<class T>
    int reduce(T *src, T *dst, size_t count, uint32_t root,
               upcxx_op_t op, upcxx_datatype_t dt)
    {
      // YZ: check consistency of T and dt
      assert(_gasnet_team != NULL);
      gasnet_coll_reduce(_gasnet_team, root, dst, src, 0, 0, sizeof(T),
                         count, dt, op, UPCXX_GASNET_COLL_FLAG);
      return UPCXX_SUCCESS;
    }
    
  private:
    uint32_t _team_id;
    uint32_t _size;
    uint32_t _myrank;
    uint32_t *_members;
    range *_member_range;
    gasnet_team_handle_t _gasnet_team;
  }; // end of struct team
  
  int team_size(upcxx::team t, uint32_t &size);

  int team_rank(upcxx::team t, uint32_t &rank);

} // namespace upcxx

extern upcxx::team team_all;


