#pragma once

#define UPCXXA_HAVE_GASNET_TOOLS
#define UPCXXA_MEMORY_DISTRIBUTED
#define UPCXXA_COMM_AM2
#ifdef USE_CXX11
# define UPCXXA_USE_CXX11
#endif
#include "allocate.h"
#include "array_bulk.h"
#include "array_defs.h"
#include "interfaces.h"
#include "team.h"
#include "upcxx-arrays/array.h"
