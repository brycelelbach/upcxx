#include <cstring>

#ifndef USE_UPCXX
# define USE_UPCXX 1
#endif

#if USE_UPCXX
# include <upcxx.h>
# include <array.h>
#else
# include "../../include/upcxx-arrays/array.h"
# define barrier()
# define async_copy_fence()
# define THREADS 1
# define MYTHREAD 0
static void init(int *argc, char ***argv) {}
static void finalize() {}

struct reduce {
  template<class T> static T add(T val, int = 0) { return val; } 
  template<class T> static T max(T val, int = 0) { return val; } 
  template<class T> static T min(T val, int = 0) { return val; } 
};
#endif

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

#ifdef TIMERS_ENABLED
# define TIMER_START(t) t.start()
# define TIMER_STOP(t) t.stop()
# define TIMER_RESET(t) t.reset()
#else
# define TIMER_START(t)
# define TIMER_STOP(t)
# define TIMER_RESET(t)
#endif

#if defined(OPT_LOOP) || defined(SPLIT_LOOP) || defined(OMP_SPLIT_LOOP)
# define USE_UNSTRIDED
#  define foreachh(N, dom, l_, u_, s_, d_)                              \
  for (point<N> l_ = dom.lwb(), u_ = dom.upb(), s_ = dom.stride(),      \
         d_ = point<N>::all(0); !d_.x[0]; d_.x[0] = 1)
#  define foreachhd(var, dim, lp_, up_, sp_)                            \
  for (int var = lp_.x[dim]; var < up_.x[dim]; var += sp_.x[dim])
# ifdef ALT_FOREACH
#  undef foreach1
#  undef foreach2
#  undef foreach3
#  define foreach1(v1, dom)                                             \
  foreach1_(v1, dom, CONCAT_(v1, _plwb),  CONCAT_(v1, _pupb),           \
            CONCAT_(v1, _pstr), CONCAT_(v1, _done))
#  define foreach1_(v1, dom, lp_, up_, sp_, d_) \
  foreachh(1, dom, lp_, up_, sp_, d_)           \
    foreachhd(v1, 0, lp_, up_, sp_)
#  define foreach2(v1, v2, dom)                                         \
  foreach2_(v1, v2, dom, CONCAT_(v1, _plwb),  CONCAT_(v1, _pupb),       \
            CONCAT_(v1, _pstr), CONCAT_(v1, _done))
#  define foreach2_(v1, v2, dom, lp_, up_, sp_, d_)     \
  foreachh(2, dom, lp_, up_, sp_, d_)                   \
    foreachhd(v1, 0, lp_, up_, sp_)                     \
      foreachhd(v2, 1, lp_, up_, sp_)
#  define foreach3(v1, v2, v3, dom)                                     \
  foreach3_(v1, v2, v3, dom, CONCAT_(v1, _plwb),  CONCAT_(v1, _pupb),   \
            CONCAT_(v1, _pstr), CONCAT_(v1, _done))
#  define foreach3_(v1, v2, v3, dom, lp_, up_, sp_, d_) \
  foreachh(3, dom, lp_, up_, sp_, d_)                   \
    foreachhd(v1, 0, lp_, up_, sp_)                     \
      foreachhd(v2, 1, lp_, up_, sp_)                   \
        foreachhd(v3, 2, lp_, up_, sp_)
# endif /* ALT_FOREACH */
#endif /* OPT_LOOP || SPLIT_LOOP || OMP_SPLIT_LOOP */

#if defined(USE_UNSTRIDED) && !defined(STRIDEDNESS)
# ifdef USE_CMAJOR
#  define STRIDEDNESS simple_column
# else
#  define STRIDEDNESS simple
# endif
#endif

#ifdef USE_CMAJOR
# define CMAJOR , true
# define cforeach3(i, j, k, dom) foreach3(k, j, i, dom)
#else
# define CMAJOR
# define cforeach3(i, j, k, dom) foreach3(i, j, k, dom)
#endif

#ifdef STRIDEDNESS
# define UNSTRIDED , STRIDEDNESS
# define GUNSTRIDED , STRIDEDNESS
#else
# define UNSTRIDED
# define GUNSTRIDED
#endif

using namespace std;
using namespace upcxx;

#ifndef DEBUG
# define DEBUG 0
#endif

#ifdef SECOND_ORDER
# define GHOST_WIDTH 2
#else
# define GHOST_WIDTH 1
#endif

#ifndef CONSTANT_VALUE
# define CONSTANT_VALUE 1.0
#endif

#ifndef WEIGHT
# define WEIGHT 6.0
#endif

#define SWAP(A, B, TYPE) do {                   \
    TYPE swap_tmp##A = A;                       \
    A = B;                                      \
    B = swap_tmp##A;                            \
  } while (false)

#define COMMA ,