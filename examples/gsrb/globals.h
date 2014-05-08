#pragma once

#ifndef USE_UPCXX
# define USE_UPCXX 1
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
using namespace std;

#ifndef DISABLE_TIMERS
# define TIMERS_ENABLED
#endif

#if USE_UPCXX
# include <timer.h>
# include <reduce.h>
#elif __cplusplus >= 201103L
# include <chrono>
namespace upcxx {
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
}
#else
# include <ctime>
namespace upcxx {
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
}
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

#if USE_UPCXX
# include <broadcast.h>
# include <event.h>
#else
# define broadcast(val, src) val
# define async_wait()
# define THREADS 1
# define MYTHREAD 0
namespace upcxx {
  static void barrier() {}
  static void init(int *argc, char ***argv) {}
  static void finalize() {}

  struct reduce {
    template<class T> static T min(T val) { return val; } 
    template<class T> static T max(T val) { return val; } 
    template<class T> static T add(T val) { return val; } 
  };
}
#endif
