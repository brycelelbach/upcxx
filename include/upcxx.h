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

#include "upcxx_types.h"
#include "upcxx_runtime.h"
#include "allocate.h"
#include "gasnet_api.h"
#include "event.h"
#include "machine.h"
#include "ptr_to_shared.h"
#include "queue.h"
#include "lock.h"
#include "shared_var.h"
#include "async.h"
#include "active_coll.h"
#include "group.h"
#include "team.h"
#include "collective.h"
#include "shared_array.h"

#define THREADS gasnet_nodes()
#define MYTHREAD gasnet_mynode()

#endif /* UPCXX_H_ */
