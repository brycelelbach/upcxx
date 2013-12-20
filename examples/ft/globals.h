#pragma once

#ifndef USE_UPCXX
# define USE_UPCXX 1
#endif

#if USE_UPCXX
# define TIMERS_ENABLED
#endif

/* POTENTIAL OPTIMIZATIONS */

/* The following flag enables communication of slabs.  If it is turned off, pencils
   are used instead (for finer-grained communication) */
#define SLABS

/* This indicates how many doubles are added to the unit-stride
   dimension of each array so as to avoid cache-thrashing */
#define PADDING 1

/* The following flag enables nonblocking arraycopy.  If it is turned off, blocking 
   communication will be used instead.  Do NOT modify the statements following it */
#define NONBLOCKING_ARRAYCOPY

#ifdef NONBLOCKING_ARRAYCOPY
#define COPY copy_nbi
#else
#define COPY copy
#endif

#include <complex>
typedef std::complex<double> Complex;

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

#include "profiling.h"

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

struct reduce {
  template<class T> static T min(T val) { return val; } 
  template<class T> static T max(T val) { return val; } 
  template<class T> static T add(T val) { return val; } 
};
#endif
