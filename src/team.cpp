/**
 * team.cpp
 */

#include "upcxx.h"
#include "upcxx_internal.h"

#ifndef GASNET_COLL_SCRATCH_SEG_SIZE
#define GASNET_COLL_SCRATCH_SEG_SIZE (2048*1024)
#endif

using namespace std;

namespace upcxx
{
  team team_all;

  uint32_t my_team_seq = 1;
  gasnet_hsl_t team::_tid_lock = GASNET_HSL_INITIALIZER;
  vector<team *> team::_team_stack;

  bool _threads_deprecated_warned = false;

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
      = gasnete_coll_team_split(_gasnet_team, color, key, &scratch_seg GASNETE_THREAD_GET);
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
    uint32_t new_tid =
      (global_myrank() << TEAM_ID_BITS) + my_team_seq++;
    gasnet_hsl_unlock(&team::_tid_lock);
    return new_tid;
  }
  
  std::vector< std::vector<rank_t> > pshm_teams;

  void init_pshm_teams(const gasnet_nodeinfo_t *nodeinfo_from_gasnet,
                       uint32_t num_nodes)
  {
    if (nodeinfo_from_gasnet != NULL) {
      // Figure out the total number of supernodes
      gasnet_node_t max_supernode=0;
      for (uint32_t i=0; i<num_nodes; i++) {
        if (max_supernode < nodeinfo_from_gasnet[i].supernode) {
          max_supernode = nodeinfo_from_gasnet[i].supernode;
        }
      }
      // printf("max_supernode %u\n", max_supernode);
      pshm_teams.resize(max_supernode+1);

      for (uint32_t i=0; i<num_nodes; i++) {
        pshm_teams[nodeinfo_from_gasnet[i].supernode].push_back(i);
      }
    } else {
      pshm_teams.resize(global_ranks());
      for (uint32_t i=0; i<global_ranks(); i++) {
        pshm_teams[i].push_back(i);
      }
    }
  }
} // namespace upcxx
