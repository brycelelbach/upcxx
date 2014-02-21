#pragma once

#define HAVE_GASNET_TOOLS
#define MEMORY_DISTRIBUTED
#define COLL_GASNET
#define COMM_AM2
#include "allocate.h"
#include "array_bulk.h"
#include "interfaces.h"
#include "team.h"
#include "upcxx-arrays/array.h"
