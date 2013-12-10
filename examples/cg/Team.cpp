#include "Team.h"
#define USE_TEAMS
#include <array.h>

#ifndef GASNET_COLL_SCRATCH_SEG_SIZE
# define GASNET_COLL_SCRATCH_SEG_SIZE (2048*1024)
#endif

namespace upcxx {

  vector<Team *> Team::_team_stack;
  int Team::_next_id = 1;

  Team *Team::splitTeamAll(int color, int rank) {
    if (_split) {
      std::cerr << "team has already been split; "
                << "split a copy instead" << endl;
      abort();
    }
    ndarray<color_and_rank, 1> arr(RECTDOMAIN((0), (THREADS)));
    color_and_rank car = {color, rank};
    arr.exchange(car);
    int max_color = color;
    int num_my_color = 0;
    foreach (p, arr.domain()) {
      car = arr[p];
      if (car.color > max_color) max_color = car.color;
      if (car.color == color) num_my_color++;
    }
    _my_child = new Team(this, num_my_color, rank, max_color+1, color);
    _num_subteams = max_color+1;
    _split = true;
    int i = 0;
    foreach (p, arr.domain()) {
      car = arr[p];
      if (car.color == color) _my_child->_members[i++] = p[1];
    }

    // Perform backend team split operation
    gasnet_seginfo_t scratch_seg;
    scratch_seg.size = GASNET_COLL_SCRATCH_SEG_SIZE;
#ifdef USE_GASNET_FAST_SEGMENT
    scratch_seg.addr = gasnet_seg_alloc(scratch_seg.size);
#else
    scratch_seg.addr = malloc(scratch_seg.size);
#endif
    assert(scratch_seg.addr != NULL); // change to user-level check

    gasnet_team_handle_t new_gasnet_team
      = gasnete_coll_team_split(_gasnet_team, color, rank, &scratch_seg);
    assert(new_gasnet_team != NULL); // change to user-level check
    _my_child->_gasnet_team = new_gasnet_team;
  
    return _my_child;
  }

} // namespace upcxx
