/**
 * team.h - implements teams and collective operations for teams
 *
 */

#pragma once

#include <cassert>
#include <cstring>
#include <vector>
#include <iostream>

#include "upcxx_types.h"
#include "upcxx_runtime.h"
#include "gasnet_api.h"
#include "range.h"
#include "coll_flags.h"
#include "utils.h"

/// \cond SHOW_INTERNAL

namespace upcxx
{

  struct ts_scope;

  struct team {
    team() : _mychild(NULL) {
      if (_team_stack.size() > 0) {
        const team *other = current_team();
        _parent = other->_parent;
        _size = other->_size;
        _myrank = other->_myrank;
        _color = other->_color;
        _team_id = other->_team_id;
        _gasnet_team = other->_gasnet_team;
      }
    }

    team(const team *other) : _mychild(NULL) {
      _parent = other->_parent;
      _size = other->_size;
      _myrank = other->_myrank;
      _color = other->_color;
      _team_id = other->_team_id;
      _gasnet_team = other->_gasnet_team;
    }
    
    ~team()
    {
      // YZ: free the underlying gasnet team?
      //      if (_gasnet_team != NULL) {
      //        gasnete_coll_team_free(_gasnet_team);
      //      }
    }

    inline uint32_t myrank() const { return _myrank; }
    
    inline uint32_t size() const { return _size; }
    
    inline uint32_t team_id() const { return _team_id; }
    
    inline team *parent() const { return _parent; }

    inline team *mychild() const { return _mychild; }

    inline uint32_t color() const { return _color; }

    inline gasnet_team_handle_t gasnet_team() const {
      return _gasnet_team;
    }

    inline bool is_team_all() const
    {
      return (_team_id == 0); // team_all has team_id 0
    }

    // void create_gasnet_team();
    int split(uint32_t color, uint32_t relrank, team *&new_team);
    int split(uint32_t color, uint32_t relrank);
    
    inline int barrier() const
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

    inline int bcast(void *src, void *dst, size_t nbytes, uint32_t root) const
    {
      assert(_gasnet_team != NULL);
      gasnet_coll_broadcast(_gasnet_team, dst, root, src, nbytes,
                            UPCXX_GASNET_COLL_FLAG);
      return UPCXX_SUCCESS;
    }
  
    inline int gather(void *src, void *dst, size_t nbytes, uint32_t root) const
    {
      assert(_gasnet_team != NULL);
      gasnet_coll_gather(_gasnet_team, root, dst, src, nbytes, 
                         UPCXX_GASNET_COLL_FLAG);
      return UPCXX_SUCCESS;
    }

    inline int allgather(void *src, void *dst, size_t nbytes) const
    {
      assert(_gasnet_team != NULL);
      gasnet_coll_gather_all(_gasnet_team, dst, src, nbytes, 
                             UPCXX_GASNET_COLL_FLAG);
      return UPCXX_SUCCESS;
    }

    template<class T>
    int reduce(T *src, T *dst, size_t count, uint32_t root,
               upcxx_op_t op, upcxx_datatype_t dt) const
    {
      // YZ: check consistency of T and dt
      assert(_gasnet_team != NULL);
      gasnet_coll_reduce(_gasnet_team, root, dst, src, 0, 0, sizeof(T),
                         count, dt, op, UPCXX_GASNET_COLL_FLAG);
      return UPCXX_SUCCESS;
    }
    
    /**
     * Translate a rank in a team to its global rank
     */
    inline uint32_t team_rank_to_global(uint32_t team_rank) const
    {
      return gasnete_coll_team_rank2node(_gasnet_team, team_rank);
    }
    
    inline uint32_t global_rank_to_team(uint32_t global_rank) const
    {
      return gasnete_coll_team_node2rank(_gasnet_team, global_rank);
    }

    static uint32_t new_team_id();

    static team *current_team() { return _team_stack.back(); }

    static team *global_team() { return _team_stack[0]; }

    friend int init(int *pargc, char ***pargv);
    friend struct ts_scope;

  private:
    team *_parent, *_mychild;
    uint32_t _size, _myrank, _color;
    uint32_t _team_id;
    gasnet_team_handle_t _gasnet_team;

    team(team *parent, int size, int rank, int color,
         gasnet_team_handle_t handle) : _parent(parent),
      _mychild(NULL), _size(size), _myrank(rank), _color(color),
      _team_id(new_team_id()), _gasnet_team(handle) {}

    void init_global_team() {
      _parent = NULL;
      _size = gasnet_nodes();
      _myrank = gasnet_mynode();
      _color = 0;
      _team_id = 0;
      _gasnet_team = GASNET_TEAM_ALL;
      // add to top of team stack
      if (_team_stack.size() == 0)
        _team_stack.push_back(this);
    }

    static gasnet_hsl_t _tid_lock;
    static std::vector<team *> _team_stack;

    static void descend_team(team *t) {
      if (t->_parent->_team_id != current_team()->_team_id) {
        std::cerr << "team is not a subteam of current team\n";
        abort();
      }
      _team_stack.push_back(t);
    }

    static void ascend_team() {
      _team_stack.pop_back();
    }
  }; // end of struct team
  
  inline
  std::ostream& operator<<(std::ostream& out, const team& t)
  {
    return out << "team: id " << t.team_id() << ", color " << t.color()
               << ", size " << t.size() << ", myrank " << t.myrank();
  }
  
  struct ts_scope {
    int done;
    inline ts_scope(team *t) : done(0) {
      team::descend_team(t->mychild());
    }
    inline ts_scope(team &t) : done(0) {
      team::descend_team(t.mychild());
    }
    inline ~ts_scope() {
      team::ascend_team();
    }
  };

  static inline gasnet_team_handle_t current_gasnet_team() {
    return team::current_team()->gasnet_team();
  }

  static inline uint32_t ranks() {
    return team::current_team()->size();
  }

  static inline uint32_t myrank() {
    return team::current_team()->myrank();
  }

  static inline void _threads_deprecated_warn() {
    extern bool _threads_deprecated_warned;
    _threads_deprecated_warned = true;
    /* Postpone the warning to the end in finalize() so it wouldn't
       get into the program output */
    // if (!_threads_deprecated_warned) {
    //   _threads_deprecated_warned = true;
    //   std::cerr << "WARNING: THREADS and MYTHREADS are deprecated;\n"
    //             << "         use upcxx::ranks() and upcxx::myranks() instead"
    //             << std::endl;
    // }
  }

  static inline uint32_t _threads_deprecated() {
    _threads_deprecated_warn();
    return team::current_team()->size();
  }

  static inline uint32_t _mythread_deprecated() {
    _threads_deprecated_warn();
    return team::current_team()->myrank();
  }

  /*
  static inline uint32_t global_ranks() {
    return team::global_team()->size();
  }

  static inline uint32_t global_myrank() {
    return team::global_team()->myrank();
  }
  */

  extern team team_all;
  extern std::vector< std::vector<rank_t> > pshm_teams;

} // namespace upcxx

// Dynamically scoped hierarchical team construct
#define upcxx_teamsplit(t) UPCXX_teamsplit_(UPCXX_UNIQUIFY(fs_), t)
#define UPCXX_teamsplit_(name, t)                               \
  for (upcxx::ts_scope name(t); name.done == 0; name.done = 1)

#ifdef UPCXX_SHORT_MACROS
# define teamsplit upcxx_teamsplit
#endif

// These are no longer supported.
// #define THREADS upcxx::ranks()
// #define MYTHREAD upcxx::myrank()

// Keep the warned version for a while for user code migration
#define THREADS upcxx::_threads_deprecated()
#define MYTHREAD upcxx::_mythread_deprecated()
