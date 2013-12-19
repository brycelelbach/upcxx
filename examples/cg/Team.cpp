#include "Team.h"
#define USE_TEAMS
#include <array.h>

#ifndef GASNET_COLL_SCRATCH_SEG_SIZE
# define GASNET_COLL_SCRATCH_SEG_SIZE (2048*1024)
#endif

namespace upcxx {

  vector<Team *> Team::_team_stack;
  int Team::_next_id = 1;

  Team *Team::split(int color, int rank) {
    if (_split) {
      std::cerr << "team has already been split; "
                << "split a copy instead" << endl;
      abort();
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

    _mychild = new Team(this, gasnet_coll_team_size(new_gasnet_team),
                        rank, color, new_gasnet_team);
    return _mychild;
  }

} // namespace upcxx
