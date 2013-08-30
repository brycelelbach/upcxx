#include "upcxx.h"

namespace upcxx {
  int barrier()
  {
    int rv;
    gasnet_barrier_notify(0, GASNET_BARRIERFLAG_ANONYMOUS);
    while ((rv=gasnet_barrier_try(0, GASNET_BARRIERFLAG_ANONYMOUS))
           == GASNET_ERR_NOT_READY) {
      if (progress() != UPCXX_SUCCESS) { // process the async task queue
        return UPCXX_ERROR;
      }
    }
    assert(rv == GASNET_OK);
    return UPCXX_SUCCESS;
  }
} // namespace upcxx
