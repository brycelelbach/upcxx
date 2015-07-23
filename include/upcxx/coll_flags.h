/**
 * coll_flags.h - GASNet flags used by all collective operations
 */

#pragma once

#define UPCXX_GASNET_COLL_FLAG \
  (GASNET_COLL_IN_MYSYNC | GASNET_COLL_OUT_MYSYNC | GASNET_COLL_LOCAL)
