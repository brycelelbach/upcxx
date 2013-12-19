/**
 * team.cpp
 */

#include "upcxx.h"

#ifndef GASNET_COLL_SCRATCH_SEG_SIZE
#define GASNET_COLL_SCRATCH_SEG_SIZE (2048*1024)
#endif

upcxx::team team_all;

namespace upcxx
{
  uint32_t my_team_seq = 1;
  gasnet_hsl_t team::_tid_lock = GASNET_HSL_INITIALIZER;
  vector<team *> team::_team_stack;

#define TEAM_ID_BITS 8
#define TEAM_ID_SEQ_MASK 0xFF

  int team::split(uint32_t color,
                  uint32_t key,
                  team *&new_team)
  {
    // Performs the team split operation
    gasnet_seginfo_t scratch_seg;
    scratch_seg.size = GASNET_COLL_SCRATCH_SEG_SIZE;
#ifdef USE_GASNET_FAST_SEGMENT
    scratch_seg.addr = gasnet_seg_alloc(scratch_seg.size);
#else
    scratch_seg.addr = malloc(scratch_seg.size);
#endif
    assert(scratch_seg.addr != NULL);

    gasnet_team_handle_t new_gasnet_team
      = gasnete_coll_team_split(_gasnet_team, color, key, &scratch_seg);
    assert(new_gasnet_team != NULL);
  
    uint32_t team_sz = gasnet_coll_team_size(new_gasnet_team);
    // range r_tmp = range(0,0,0);
    new_team = new team(this, team_sz, key, color, new_gasnet_team);
    
    assert(new_team != NULL);

    if (_mychild == NULL) _mychild = new_team;
    
    return UPCXX_SUCCESS;
  } // team::split
  
  int team::split(uint32_t color,
                  uint32_t key)
  {
    if (_mychild) {
      std::cerr << "team has already been split; "
                << "split a copy instead" << endl;
      abort();
    }
    team *new_team;
    return split(color, key, new_team);
  }

  uint32_t team::new_team_id()
  {
    gasnet_hsl_lock(&team::_tid_lock);
    assert(my_team_seq < TEAM_ID_SEQ_MASK);
    uint32_t new_tid = (GLOBAL_MYTHREAD << TEAM_ID_BITS) + my_team_seq++;
    gasnet_hsl_unlock(&team::_tid_lock);
    return new_tid;
  }
  
} // namespace upcxx
