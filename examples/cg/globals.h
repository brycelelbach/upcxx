#pragma once

#ifndef USE_UPCXX
# define USE_UPCXX 1
#endif

#ifndef DISABLE_TEAMS
# define CTEAMS
#endif

/* PROFILING */

/*#define COUNTERS_ENABLED*/
/* the following is valid only if counters are enabled */
#define PAPI_MEASURE PAPICounter.PAPI_FP_INS

/* the following flag uses point-to-point communication to sync reductions
   in the SpMV, dot products, and L2-Norms.  If not enabled, barriers are
   used instead. This is disabled since it does not work properly in a
   relaxed consistency model. */
/* #define POINT_TO_POINT_COMM */

#if !defined(USE_STRIDED) && !defined(USE_UNSTRIDED)
# define USE_UNSTRIDED
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

#ifdef USE_FOREACH
# define FOREACH upcxx_foreach
#elif !defined(FOREACH)
# define FOREACH foreach1
# define foreach1(var, dom)                             \
  foreach1_(var, (dom), UPCXXA_CONCAT_(var, _upb),      \
            UPCXXA_CONCAT_(var, _stride),               \
            UPCXXA_CONCAT_(var, _done))
# define foreach1_(var, dom, u_, s_, d_)                \
  for (upcxx::cint_t u_ = (dom).upb()[1],               \
	 s_ = (dom).raw_stride()[1],                    \
	 var = (dom).lwb()[1]; var < u_; var += s_)
#endif

#if defined(CTEAMS) && !defined(VREDUCE)
# define VREDUCE
#endif

#if defined(VREDUCE) && !defined(TEAMS)
# define TEAMS
#endif

/* Enabling the following flag indicates pushing data for the diagonal swap
   in SparMat.ti.  If turned off, the data is pulled instead. */
#define PUSH_DATA

/* DO NOT CHANGE THE FOLLOWING */

#ifndef DISABLE_TIMERS
# define TIMERS_ENABLED
#endif

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

#ifdef TIMERS_ENABLED
# define TIMER_START(timername)  timername.start()
# define TIMER_STOP(timername)   timername.stop()
# define TIMER_STOP_READ(timername, dest) do {  \
    timername.stop();                           \
    dest = timername.secs();                    \
    timername.reset();                          \
  } while (false)
#else
# define TIMER_START(timername)           do {} while (false)
# define TIMER_STOP(timername)            do {} while (false)
# define TIMER_STOP_READ(timername, dest) do {} while (false)
#endif

#ifdef COUNTERS_ENABLED
# define COUNTER_START(countername)  countername.start()
# define COUNTER_STOP(countername)   countername.stop()
# define COUNTER_STOP_READ(countername, dest) do { \
    countername.stop();                            \
    dest = countername.getCounterValue();          \
    countername.clear();                           \
  } while (false)
#else
# define COUNTER_START(countername)           do {} while (false)
# define COUNTER_STOP(countername)            do {} while (false)
# define COUNTER_STOP_READ(countername, dest) do {} while (false)
#endif

#include <iostream>
#if USE_UPCXX
# include <upcxx.h>
# ifdef TEAMS
#  define USE_TEAMS
#  include <upcxx/team.h>
# endif
# include <upcxx/array.h>
#else
# include "../../include/upcxx-arrays/array.h"
#endif
#define print(s) std::cout << s
#define println(s) std::cout << s << std::endl

#define Point point
#define RectDomain rectdomain

using namespace upcxx;

#if !USE_UPCXX
static void barrier() {}
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
