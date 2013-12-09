#pragma once

#include <iostream>
#include <vector>
#include <upcxx.h>
#include <array.h>

namespace upcxx {

  struct color_and_rank {
    int color, rank;
  };

  class Team {
   public:
    Team() : _my_child(NULL), _num_subteams(-1), _split(false) {
      Team *other = currentTeam();
      _members = other->_members;
      _parent = other->_parent;
      _size = other->_size;
      _rank = other->_rank;
      _num_siblings = other->_num_siblings;
      _team_rank = other->_team_rank;
      _gasnet_team = other->_gasnet_team;
    }
    Team(Team *other) : _my_child(NULL), _num_subteams(-1), _split(false) {
      _members = other->_members;
      _parent = other->_parent;
      _size = other->_size;
      _rank = other->_rank;
      _num_siblings = other->_num_siblings;
      _team_rank = other->_team_rank;
      _gasnet_team = other->_gasnet_team;
    }

    Team *splitTeamAll(int color, int rank);
    /* void splitTeam(int num_children); */
    Team *myChildTeam() { return _my_child; }
    int numChildren() { return _num_subteams; }
    int teamRank() { return _team_rank; }
    int size() { return _size; }
    int rank() { return _rank; }

    static void initialize() {
      _team_stack.push_back(new Team(false));
    }
    static Team *currentTeam() { return _team_stack.back(); }
    static Team *globalTeam() { return _team_stack[0]; }
    static void descend_team(Team *t) {
      if (t->_parent != currentTeam()) {
        std::cerr << "team is not a subteam of current team" << endl;
        abort();
      }
      _team_stack.push_back(t);
    }
    static void ascend_team() {
      _team_stack.pop_back();
    }
   private:
    Team(Team *parent, int size, int rank, int num_siblings, int team_rank)
      : _members(new int[size]), _parent(parent), _my_child(NULL),
        _size(size), _rank(rank), _num_subteams(-1), _num_siblings(num_siblings),
        _team_rank(team_rank), _split(false) {}
    Team(bool ignored) : _members(new int[THREADS]), _parent(NULL),
      _my_child(NULL), _size(THREADS), _rank(MYTHREAD), _num_subteams(-1),
      _num_siblings(1), _team_rank(0), _split(false),
      _gasnet_team(GASNET_TEAM_ALL) {
        for (int i = 0; i < THREADS; i++)
          _members[i] = i;
      }

    int *_members;
    Team *_parent, *_my_child;
    int _size, _rank, _num_subteams, _num_siblings, _team_rank;
    bool _split;
    gasnet_team_handle_t _gasnet_team;
    static vector<Team *> _team_stack;
  };

  struct ts_scope {
    int done;
    inline ts_scope(Team *t) : done(0) {
      Team::descend_team(t);
    }
    inline ~ts_scope() {
      Team::ascend_team();
    }
  };

} // namespace upcxx

#define teamsplit(t) teamsplit_(UNIQUIFY(fs_), t)
#define teamsplit_(name, t)                             \
  for (ts_scope name(t); name.done == 0; name.done = 1)
