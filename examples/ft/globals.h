#pragma once

#ifndef USE_UPCXX
# define USE_UPCXX 1
#endif

#ifndef USE_FFTW3
# define USE_FFTW3 1
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
using namespace std;

#ifndef DISABLE_TIMERS
# define TIMERS_ENABLED
#endif

#if USE_UPCXX
# include <timer.h>
# include <reduce.h>
#elif __cplusplus >= 201103L
# include <chrono>
struct timer {
  std::chrono::time_point<std::chrono::system_clock> cur;
  std::chrono::duration<double> elap;
  inline timer() : cur(), elap(std::chrono::duration<double>::zero()) {}
  inline void start() {
    cur = std::chrono::system_clock::now();
  }
  inline void stop() {
    elap = std::chrono::system_clock::now() - cur;
  }
  inline void reset() {
    elap = std::chrono::duration<double>::zero();
  }
  inline double secs() {
    return elap.count();
  }
};
#else
# include <ctime>
struct timer {
  clock_t cur, elap;
  inline timer() : cur(0), elap(0) {}
  inline void start() {
    cur = clock();
  }
  inline void stop() {
    elap = clock() - cur;
  }
  inline void reset() {
    elap = 0;
  }
  inline double secs() {
    return ((double)elap) / CLOCKS_PER_SEC;
  }
};
#endif

#ifdef SPLIT_LOOP
# define USE_UNSTRIDED
#endif

#if defined(USE_UNSTRIDED) && !defined(STRIDEDNESS)
# define STRIDEDNESS simple
#endif

#ifdef STRIDEDNESS
# define UNSTRIDED , STRIDEDNESS
# define GUNSTRIDED , STRIDEDNESS
#else
# define UNSTRIDED
# define GUNSTRIDED
#endif

#include "profiling.h"

#if USE_UPCXX
# include <broadcast.h>
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
