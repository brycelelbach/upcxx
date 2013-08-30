/**
 * team.cpp
 */

#include "upcxx.h"

#ifndef GASNET_COLL_SCRATCH_SEG_SIZE
#define GASNET_COLL_SCRATCH_SEG_SIZE (2048*1024)
#endif

namespace upcxx
{
  team *team_all;

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
  
    uint32_t *members;
    uint32_t team_sz = gasnet_coll_team_size(new_gasnet_team);
    members = (uint32_t *)malloc(sizeof(uint32_t) * team_sz);
    assert(members != NULL);
    for (uint32_t i = 0; i < team_sz; i++) {
      members[i] =  gasnete_coll_team_rank2node(new_gasnet_team, i);
    }
    new_team = new upcxx::team(0, // \Todo fix the team id!!
                               team_sz, // size
                               key, // rank
                               members, // need to have a public interface in GASNet to to expose this
                               new_gasnet_team);
    assert(new_team != NULL);
    free(members);
    
    return UPCXX_SUCCESS;
  } // team::split
} // namespace upcxx
