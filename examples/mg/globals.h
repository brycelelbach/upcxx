#pragma once

#ifndef USE_UPCXX
# define USE_UPCXX 1
#endif

#ifndef DISABLE_TIMERS
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
#define COPY async_copy
#else
#define COPY copy
#endif

#include <iostream>
#if USE_UPCXX
# include <upcxx.h>
# include <upcxx/array.h>
# include <upcxx/broadcast.h>
# include <upcxx/reduce.h>
# include <upcxx/event.h>
#else
# include "../../include/upcxx-arrays/array.h"
static void barrier() {}
# define broadcast(val, src) val
static void async_wait() {}
static int ranks() { return 1; }
static int myrank() { return 0; }
static void init(int *argc, char ***argv) {}
static void finalize() {}

struct reduce {
  template<class T> static T add(T val, int = 0) { return val; }
  template<class T> static T max(T val, int = 0) { return val; }
  template<class T> static T min(T val, int = 0) { return val; }
};
#endif

#define print(s) std::cout << s
#define println(s) std::cout << s << std::endl

#define Point point
#define RectDomain rectdomain

using namespace upcxx;

#if USE_UPCXX
# include <upcxx/timer.h>
# include <upcxx/reduce.h>
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
    elap += std::chrono::system_clock::now() - cur;
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
    elap += clock() - cur;
  }
  inline void reset() {
    elap = 0;
  }
  inline double secs() {
    return ((double)elap) / CLOCKS_PER_SEC;
  }
};
#endif

#ifndef foreach
# define foreach upcxx_foreach
#endif

#ifndef foreach1
# define foreach1(i, dom)                       \
  foreachd(i, dom, 1)
# define foreach2(i, j, dom)                    \
  foreach1(i, dom) foreachd(j, dom, 2)
# define foreach3(i, j, k, dom)                 \
  foreach2(i, j, dom) foreachd(k, dom, 3)
# define foreachd(var, dom, dim)                        \
  foreachd_(var, (dom), dim, UPCXXA_CONCAT_(var, _upb), \
            UPCXXA_CONCAT_(var, _stride),               \
            UPCXXA_CONCAT_(var, _done))
# define foreachd_(var, dom, dim, u_, s_, d_)           \
  for (upcxx::cint_t u_ = (dom).upb()[dim],             \
	 s_ = (dom).raw_stride()[dim],                  \
	 var = (dom).lwb()[dim]; var < u_; var += s_)
#endif

#if defined(USE_UNSTRIDED) && !defined(STRIDEDNESS)
# define STRIDEDNESS row
#endif

#ifdef STRIDEDNESS
# define UNSTRIDED , STRIDEDNESS
# define GUNSTRIDED , STRIDEDNESS
#else
# define UNSTRIDED
# define GUNSTRIDED
#endif
