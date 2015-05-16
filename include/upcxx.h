/**
 *\mainpage UPCXX 
 * UPCXX is a global address space programming system implemented as a C++ library.
 *
 * UPCXX clients should include \c upcxx.h, which includes all other
 * necessary headers.
 */


/**
 * \section intro Introduction
 */

#ifndef UPCXX_H_
#define UPCXX_H_

#include <cstring>
#include <cassert>
#include <ios>

#include "upcxx_config.h"
#include "gasnet_api.h"
#include "upcxx_types.h"
#include "upcxx_runtime.h"
#include "team.h"
#include "allocate.h"
#include "event.h"
#include "global_ptr.h"
#include "async_copy.h"
#include "queue.h"
#include "lock.h"
#include "shared_var.h"
#include "async.h"
#include "active_coll.h"
#include "group.h"
#include "collective.h"
#include "shared_array.h"
#include "atomic.h"

namespace upcxx
{
  struct upcxx_runtime
  {
    bool _owner;

    upcxx_runtime()
    {
      if (!is_init()) {
#if UPCXX_DEBUG
        printf("Rank %u: Initializing upcxx runtime.\n", myrank());
#endif
        init(NULL, NULL);
        _owner = true;
      } else {
        _owner = false;
      }
    }

    ~upcxx_runtime()
    {
      assert(is_init());

      if (_owner) {
#if UPCXX_DEBUG
        printf("Rank %u; Destructing upcxx runtime\n", myrank());
#endif
        finalize();
      }
    }
  };
  static upcxx_runtime __upcxx_runtime_obj; // this object is not expected to be accessed directly by the user
}

#endif /* UPCXX_H_ */
