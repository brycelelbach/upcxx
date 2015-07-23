#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>

#ifndef USE_UPCXX
# define USE_UPCXX 1
#endif

#if USE_UPCXX
# include <upcxx.h>
# include <upcxx/array.h>
# include <upcxx/reduce.h>
# include <upcxx/broadcast.h>
# include <upcxx/event.h>
#else
# include "../../include/upcxx-arrays/array.h"
static void barrier() {}
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

template<class T> T broadcast(T val, int root) {
  return val;
}

template<class T, int N, class F1, class F2>
void broadcast(upcxx::ndarray<T, N, F1, F2> &src,
               upcxx::ndarray<T, N, F1, F2> &dst,
               int root) {
  dst.copy(src);
}
#endif

#ifndef DISABLE_TIMERS
# define TIMERS_ENABLED
#endif

#if USE_UPCXX
# include <upcxx/timer.h>
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

#ifdef VAR_INDEXING
# define getTotal(total, i, j) total(i,j)
# define writeTotal(total, i, j, value) total(i,j) = value
#else
# define getTotal(total, i, j) total[PT(i,j)]
# define writeTotal(total, i, j, value) total[PT(i,j)] = value
#endif

using namespace std;
using namespace upcxx;

#define max(x, y) (x >= y ? x : y)
#define min(x, y) (x >= y ? y : x)
#define print(s) std::cout << s
#define println(s) std::cout << s << std::endl

#ifdef DEBUG_LEVEL
# define debug(x, y) if (DEBUG_LEVEL >= y) println(x)
#else
# define debug(x, y)
#endif
