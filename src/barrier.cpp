#include "upcxx/upcxx.h"
#include "upcxx/upcxx_internal.h"

namespace upcxx {
  int barrier()
  {
    int rv;
    UPCXX_CALL_GASNET(gasnet_coll_barrier_notify(current_gasnet_team(), 0,
                                                 GASNET_BARRIERFLAG_ANONYMOUS));
    do {
      UPCXX_CALL_GASNET(rv=gasnet_coll_barrier_try(current_gasnet_team(), 0,
                                                   GASNET_BARRIERFLAG_ANONYMOUS));
      if (rv == GASNET_ERR_NOT_READY) {
        if (advance() < 0) { // process the async task queue
          return UPCXX_ERROR;
        }
      }
    } while (rv != GASNET_OK);

    return UPCXX_SUCCESS;
  }
} // namespace upcxx
