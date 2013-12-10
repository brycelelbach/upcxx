#pragma once

#ifndef USE_UPCXX
# define USE_UPCXX 1
#endif

/* PROFILING */

#if USE_UPCXX
# define TIMERS_ENABLED
#endif

/*#define COUNTERS_ENABLED*/
/* the following is valid only if counters are enabled */
#define PAPI_MEASURE PAPICounter.PAPI_FP_INS

/* the following flag uses point-to-point communication to sync reductions
   in the SpMV, dot products, and L2-Norms.  If not enabled, barriers are
   used instead. This is disabled since it does not work properly in a
   relaxed consistency model. */
/* #define POINT_TO_POINT_COMM */


#if defined(RTCOPY) && !defined(CTEAMS)
# define CTEAMS
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
#  include "Team.h"
# endif
# include <array.h>
#else
# include "../../include/upcxx-arrays/array.h"
#endif
#define print(s) std::cout << s
#define println(s) std::cout << s << std::endl

#define Point point
#define RectDomain rectdomain

#if defined(TIMERS_ENABLED) || defined(COUNTERS_ENABLED)
# include "Reduce.h"
#endif

using namespace upcxx;

#if !USE_UPCXX
# define barrier()
# define THREADS 1
# define MYTHREAD 0
static void init(int *argc, char ***argv) {}
static void finalize() {}
#endif
