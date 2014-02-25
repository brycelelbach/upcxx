#include "upcxx.h"
#include "team.h"

namespace upcxx {
  int barrier()
  {
    int rv;
    gasnet_coll_barrier_notify(CURRENT_GASNET_TEAM, 0,
                               GASNET_BARRIERFLAG_ANONYMOUS);
    while ((rv=gasnet_coll_barrier_try(CURRENT_GASNET_TEAM, 0,
                                       GASNET_BARRIERFLAG_ANONYMOUS))
           == GASNET_ERR_NOT_READY) {
      if (advance() < 0) { // process the async task queue
        return UPCXX_ERROR;
      }
    }
    assert(rv == GASNET_OK);
    return UPCXX_SUCCESS;
  }
} // namespace upcxx
