#include <cstring>
#include <cstdlib>
#include <string>
#include <iostream>

#ifndef USE_UPCXX
# define USE_UPCXX 1
#endif

#ifndef USE_MPI
# define USE_MPI 0
#endif

#if USE_UPCXX
# include <upcxx.h>
# include <array.h>
# include <event.h>
#elif USE_MPI
# include <assert.h>
# include <mpi.h>
# ifdef USE_ARRAYS
#  include "../../include/upcxx-arrays/array.h"
# endif
static void barrier() { MPI_Barrier(MPI_COMM_WORLD); }
static void async_wait() {}
static void *allocate(size_t s) { return malloc(s); }
static int num_threads, thread_id;
static int ranks() { return num_threads; }
static int myrank() { return thread_id; }
static void init(int *argc, char ***argv) {
  MPI_Init(argc, argv);
  MPI_Comm_size(MPI_COMM_WORLD, &num_threads);
  MPI_Comm_rank(MPI_COMM_WORLD, &thread_id);
}
static void finalize() {
  MPI_Finalize();
}

struct reduce {
  template<class T> static T add(T val, int = 0) { return val; }
  template<class T> static T max(T val, int = 0) { return val; }
  template<class T> static T min(T val, int = 0) { return val; }
  static double add(double val, int root) {
    double res = 0;
    MPI_Reduce(&val, &res, 1, MPI_DOUBLE, MPI_SUM, root,
               MPI_COMM_WORLD);
    return res;
  }
  static double max(double val, int root) {
    double res = 0;
    MPI_Reduce(&val, &res, 1, MPI_DOUBLE, MPI_MAX, root,
               MPI_COMM_WORLD);
    return res;
  }
  static double min(double val, int root) {
    double res = 0;
    MPI_Reduce(&val, &res, 1, MPI_DOUBLE, MPI_MIN, root,
               MPI_COMM_WORLD);
    return res;
  }
};
#else
# include "../../include/upcxx-arrays/array.h"
static void barrier() {}
static void async_wait() {}
static int ranks() { return 1; }
static int myrank() { return 0; }
static void *allocate(size_t s) { return malloc(s); }
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
# define TIMER_START(t) t.start()
# define TIMER_STOP(t) t.stop()
# define TIMER_RESET(t) t.reset()
#else
# define TIMER_START(t)
# define TIMER_STOP(t)
# define TIMER_RESET(t)
#endif

#ifndef foreach3
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

#if defined(OPT_LOOP) || defined(SPEC_LOOP) || defined(OMP_LOOP) || defined(VAR_LOOP) || defined(RAW_LOOP) || defined(RAW_FOR_LOOP)
# define ALT_LOOP
#endif

#ifdef ALT_LOOP
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
#endif /* ALT_LOOP */

#if defined(RAW_LOOP) || defined(RAW_FOR_LOOP) || USE_MPI || defined(MPI_STYLE)
# ifdef USE_CMAJOR
#  define FIRST_DIM(i, j, k) k
#  define LAST_DIM(i, j, k) i
# else
#  define FIRST_DIM(i, j, k) i
#  define LAST_DIM(i, j, k) k
# endif
#endif

#if defined(USE_CMAJOR) && !defined(STRIDEDNESS)
# define STRIDEDNESS column
#elif defined(USE_UNSTRIDED) && !defined(STRIDEDNESS)
# define STRIDEDNESS row
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
#else
# define UNSTRIDED
#endif

#if !defined(USE_EXCHANGE) && !defined(SHARED_DIR) && USE_UPCXX
# define SHARED_DIR
#endif

using namespace std;
#if !USE_MPI || defined(USE_ARRAYS)
using namespace upcxx;
#endif

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
# define WEIGHT (-6.0)
#endif

template<class T> void SWAP(T &x, T &y) {
  T tmp = x;
  x = y;
  y = tmp;
}

#define COMMA ,
