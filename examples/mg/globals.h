#pragma once

#ifndef USE_UPCXX
# define USE_UPCXX 1
#endif

#if USE_UPCXX
# define TIMERS_ENABLED
#endif
#include "profiling.h"

/* SPECIFY PROBLEM CLASS */

/* Turn on for problem classes A, S, or W ONLY */
/* #define CLASS_A_S_OR_W */


/* POTENTIAL OPTIMIZATIONS */

/* Threshold level (the level below which everything is on proc 0 instead of distributed) */
/* NOTE: this can be changed to see if we get better performance */
#define THRESHOLD_LEVEL 4

/* The following optimization communicates with either 6 nearest neighbors (with barriers) or 26 nearest
   neighbors (without barriers) */
#define UPDATE_BORDER_DIRECTIONS 6

/* If the following flag is turned on, data is pushed- otherwise, it is pulled */
#define PUSH_DATA

/* The following optimization uses buffers to communicate with each block's nearest neighbors so that
   copied arrays are stored contiguously in memory both when sending and receiving.  
   THIS FLAG IS CURRENTLY REQUIRED FOR NONBLOCKING ARRAYCOPY TO OCCUR.  */
#define CONTIGUOUS_ARRAY_BUFFERS

/* The following flag enables nonblocking arraycopy- do NOT modify the statements following it */
#define NONBLOCKING_ARRAYCOPY

#ifdef NONBLOCKING_ARRAYCOPY
#define COPY copy_nbi
#else
#define COPY copy
#endif

#include <iostream>
#if USE_UPCXX
# include <upcxx.h>
# include <array.h>
#else
# include "../../include/upcxx-arrays/array.h"
#endif
#define print(s) std::cout << s
#define println(s) std::cout << s << std::endl

#define Point point
#define RectDomain rectdomain

using namespace upcxx;

#ifdef TIMERS_ENABLED
# include <timer.h>
#endif

#if USE_UPCXX
# include <broadcast.h>
# include <reduce.h>
#else
# define barrier()
# define broadcast(val, src) val
# define async_copy_fence()
# define THREADS 1
# define MYTHREAD 0
static void init(int *argc, char ***argv) {}
static void finalize() {}

struct Reduce {
  template<class T> static T add(T val) { return val; } 
};
#endif
