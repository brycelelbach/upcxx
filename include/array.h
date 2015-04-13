#pragma once

#define UPCXXA_HAVE_GASNET_TOOLS
#define UPCXXA_MEMORY_DISTRIBUTED
#define UPCXXA_COMM_AM2
#ifdef UPCXX_USE_CXX11
# define UPCXXA_USE_CXX11
#endif
#ifdef UPCXX_USE_TRIVIALLY_DESTRUCTIBLE
# define UPCXXA_USE_CXX11_TRIVIALLY_DESTRUCTIBLE
#endif
#ifdef UPCXX_USE_INITIALIZER_LIST
# define UPCXXA_USE_CXX11_INITIALIZER_LIST
#endif
#ifdef UPCXX_SHORT_MACROS
# define UPCXXA_SHORT_MACROS
#endif
#include "allocate.h"
#include "array_bulk.h"
#include "array_defs.h"
#include "interfaces.h"
#include "team.h"
#include "upcxx-arrays/array.h"
