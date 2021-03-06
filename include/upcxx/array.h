#pragma once

#include "upcxx.h"
#define UPCXXA_HAVE_GASNET_TOOLS
#define UPCXXA_MEMORY_DISTRIBUTED
#define UPCXXA_COMM_AM2
#ifdef UPCXX_HAVE_CXX11
# define UPCXXA_USE_CXX11
#endif
#ifdef UPCXX_HAVE_TRIVIALLY_DESTRUCTIBLE
# define UPCXXA_USE_CXX11_TRIVIALLY_DESTRUCTIBLE
#endif
#ifdef UPCXX_HAVE_INITIALIZER_LIST
# define UPCXXA_USE_CXX11_INITIALIZER_LIST
#endif
#ifdef UPCXX_SHORT_MACROS
# define UPCXXA_SHORT_MACROS
#endif
#ifdef UPCXX_BOUNDS_CHECKING
# define UPCXXA_BOUNDS_CHECKING 1
#endif
#include "allocate.h"
#include "array_bulk.h"
#include "array_defs.h"
#include "interfaces.h"
#include "team.h"
#include "upcxx-arrays/array.h"
