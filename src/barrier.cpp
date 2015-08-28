#include "upcxx.h"

namespace upcxx {
  int barrier()
  {
    int rv;
    gasnet_coll_barrier_notify(current_gasnet_team(), 0,
                               GASNET_BARRIERFLAG_UNNAMED);
    while ((rv=gasnet_coll_barrier_try(current_gasnet_team(), 0,
                                       GASNET_BARRIERFLAG_UNNAMED))
           == GASNET_ERR_NOT_READY) {
      if (advance() < 0) { // process the async task queue
        return UPCXX_ERROR;
      }
    }
    assert(rv == GASNET_OK);
    return UPCXX_SUCCESS;
  }
} // namespace upcxx
