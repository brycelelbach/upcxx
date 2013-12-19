#pragma once

#include <iostream>
#include <vector>
#include <upcxx.h>

namespace upcxx {

  class Team {
   public:
    Team() : _mychild(NULL), _split(false) {
      Team *other = currentTeam();
      _parent = other->_parent;
      _size = other->_size;
      _myrank = other->_myrank;
      _color = other->_color;
      _id = other->_id;
      _gasnet_team = other->_gasnet_team;
    }
    Team(Team *other) : _mychild(NULL), _split(false) {
      _parent = other->_parent;
      _size = other->_size;
      _myrank = other->_myrank;
      _color = other->_color;
      _id = other->_id;
      _gasnet_team = other->_gasnet_team;
    }

    Team *split(int color, int rank);
    /* void splitTeam(int num_children); */
    Team *parent() { return _parent; }
    Team *mychild() { return _mychild; }
    int color() { return _color; }
    int size() { return _size; }
    int myrank() { return _myrank; }
    gasnet_team_handle_t gasnet_team() { return _gasnet_team; }

    static void initialize() {
      _team_stack.push_back(new Team(false));
    }
    static Team *currentTeam() { return _team_stack.back(); }
    static Team *globalTeam() { return _team_stack[0]; }
    static void descend_team(Team *t) {
      if (t->_parent->_id != currentTeam()->_id) {
        std::cerr << "team is not a subteam of current team" << endl;
        abort();
      }
      _team_stack.push_back(t);
    }
    static void ascend_team() {
      _team_stack.pop_back();
    }
   private:
    Team(Team *parent, int size, int rank, int color,
         gasnet_team_handle_t handle)
      : _parent(parent), _mychild(NULL), _size(size), _myrank(rank),
        _color(color), _split(false), _id(_next_id++),
        _gasnet_team(handle) {}
    Team(bool ignored) : _parent(NULL),
      _mychild(NULL), _size(THREADS), _myrank(MYTHREAD), _color(0),
      _split(false), _id(0), _gasnet_team(GASNET_TEAM_ALL) {}

    Team *_parent, *_mychild;
    int _size, _myrank, _color;
    bool _split;
    int _id;
    gasnet_team_handle_t _gasnet_team;
    static vector<Team *> _team_stack;
    static gasnet_team_handle_t _current_gasnet_team;
    static int _next_id;
  };

  struct ts_scope {
    int done;
    inline ts_scope(Team *t) : done(0) {
      Team::descend_team(t->mychild());
    }
    inline ~ts_scope() {
      Team::ascend_team();
    }
  };

} // namespace upcxx

#define teamsplit(t) teamsplit_(UNIQUIFY(fs_), t)
#define teamsplit_(name, t)                             \
  for (ts_scope name(t); name.done == 0; name.done = 1)

#define barrier() do {                                                  \
    int rv;                                                             \
    gasnet_team_handle_t _gasnet_team =                                 \
      Team::currentTeam()->gasnet_team();                               \
    gasnet_coll_barrier_notify(_gasnet_team, 0,                         \
                               GASNET_BARRIERFLAG_ANONYMOUS);           \
    while ((rv=gasnet_coll_barrier_try(_gasnet_team, 0,                 \
                                       GASNET_BARRIERFLAG_ANONYMOUS))   \
           == GASNET_ERR_NOT_READY) {                                   \
      if (progress() != UPCXX_SUCCESS) {                                \
        break;                                                          \
      }                                                                 \
    }                                                                   \
    assert(rv == GASNET_OK);                                            \
  } while (0)

