/**
 * allocate.h - provide memory allocation service for the GASNet segment
 */

#pragma once

#define ONLY_MSPACES 1 // only use mspace from dl malloc
#include "dl_malloc.h"

#include <stdlib.h>
#include <assert.h>
#include "gasnet_api.h"

//typedef struct {
//  void *addr;
//  uintptr_t size;
//} gasnet_seginfo_t;

// int gasnet_getSegmentInfo (gasnet_seginfo_t *seginfo table, int numentries);

// This may need to be a thread-private data if GASNet supports per-thread segment in the future
extern gasnet_seginfo_t *all_gasnet_seginfo;
extern gasnet_seginfo_t *my_gasnet_seginfo;
extern mspace _gasnet_mspace;

static inline void init_gasnet_seg_mspace()
{
  all_gasnet_seginfo =
      (gasnet_seginfo_t *)malloc(sizeof(gasnet_seginfo_t) * gasnet_nodes());
  assert(all_gasnet_seginfo != NULL);

  int rv = gasnet_getSegmentInfo(all_gasnet_seginfo, gasnet_nodes());
  assert(rv == GASNET_OK);

  my_gasnet_seginfo = &all_gasnet_seginfo[gasnet_mynode()];

  _gasnet_mspace = create_mspace_with_base(my_gasnet_seginfo->addr,
                                           my_gasnet_seginfo->size, 1);
  assert(_gasnet_mspace != 0);

  // Set the mspace limit to the gasnet segment size so it won't go outside.
  mspace_set_footprint_limit(_gasnet_mspace, my_gasnet_seginfo->size);
}

// allocate memory from the GASNet network-addressable segment
// static inline void *gasnet_seg_alloc(size_t nbytes)
// {
//   if (_gasnet_mspace == 0) {
//     init_gasnet_seg_mspace();
//   }
//   return mspace_malloc(_gasnet_mspace, nbytes);
// }

// free memory
static inline void gasnet_seg_free(void *p)
{
  if (_gasnet_mspace == 0) {
    fprintf(stderr, "Error: the gasnet memory space is not initialized.\n");
    fprintf(stderr, "It is likely due to the pointer (%p) was not from hp_malloc().\n",
            p);
     exit(1);
  }
  assert(p != 0);
  mspace_free(_gasnet_mspace, p);
}

static inline void *gasnet_seg_memalign(size_t nbytes, size_t alignment)
{
  if (_gasnet_mspace== 0) {
    init_gasnet_seg_mspace();
  }
  return mspace_memalign(_gasnet_mspace, alignment, nbytes);
}

static inline void *gasnet_seg_alloc(size_t nbytes)
{
  return gasnet_seg_memalign(nbytes, 64);
}
