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

#endif /* UPCXX_H_ */
