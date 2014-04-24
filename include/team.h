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
#include "range.h"
#include "collective.h"

/// \cond SHOW_INTERNAL

#define UPCXX_GASNET_COLL_FLAG                                          \
  (GASNET_COLL_IN_MYSYNC | GASNET_COLL_OUT_MYSYNC | GASNET_COLL_LOCAL)

namespace upcxx
{
  struct team {
    team() {}
    
    team(uint32_t team_id, uint32_t size, uint32_t myrank, range &mbr,
         gasnet_team_handle_t gasnet_team = NULL)
      : _team_id(team_id), _size(size), _myrank(myrank), _mbr(mbr),
        _gasnet_team(gasnet_team)
    {}

    ~team()
    {
      //      if (_gasnet_team != NULL) {
      //        gasnete_coll_team_free(_gasnet_team);
      //      }
    }

    inline void init(uint32_t team_id, uint32_t size, uint32_t myrank,
                     range &mbr, gasnet_team_handle_t gasnet_team = NULL)
    {
      _team_id = team_id;
      _size = size;
      _myrank = myrank;
      _mbr = mbr;
      _gasnet_team = gasnet_team;
    }

    inline uint32_t myrank() const { return _myrank; }
    
    inline uint32_t size() const { return _size; }
    
    inline uint32_t team_id() const { return _team_id; }
    
    inline void set_team_id(uint32_t team_id)
    {
      _team_id = team_id;
    }
    
    // YZ: We can use a compact format to represent team members and only store
    // log2 number of neighbors on each node
    inline uint32_t member(uint32_t i) const
    {
      assert(i < _size);
      return _mbr[i];
    }

    inline bool is_team_all()
    {
      return (_team_id == 0); // team_all has team_id 0
    }

    inline int team_get_global_rank(upcxx::team t,
                                    uint32_t team_rank,
                                    uint32_t *global_rank)
    {
      if (team_rank < _mbr.count()) {
        *global_rank = _mbr[team_rank];
        return UPCXX_SUCCESS;
      }

      return UPCXX_ERROR;
    }
    
    // Initialize the underlying gasnet team if necessary
    int init_gasnet_team();
    
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
        if (advance() < 0) { // process the async task queue
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
    
    static gasnet_hsl_t _tid_lock;
    static uint32_t new_team_id();

  private:
    uint32_t _team_id;
    uint32_t _size;
    uint32_t _myrank;
    range _mbr;
    gasnet_team_handle_t _gasnet_team;

  }; // end of struct team
  
  inline
  std::ostream& operator<<(std::ostream& out, const team& t)
  {
    return out << "team: id " << t.team_id()
               << ", size " << t.size() << ", myrank " << t.myrank();
  }
  

} // namespace upcxx

extern upcxx::team team_all;



