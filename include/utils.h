/**
 * utils.h - internal macro and template utilities
 */

#pragma once

#ifdef __COUNTER__
#define UPCXX_UNIQUIFY(prefix) UPCXX_CONCAT_(prefix, __COUNTER__)
#else
#define UPCXX_UNIQUIFY(prefix) UPCXX_CONCAT_(prefix, __LINE__)
#endif

#define UPCXX_UNIQUIFYN(prefix, name)           \
  UPCXX_UNIQUIFY(prefix ## name ## _)

#define UPCXX_CONCAT_(a, b) UPCXX_CONCAT__(a, b)
#define UPCXX_CONCAT__(a, b)  a ## b

#ifdef UPCXX_HAVE_CXX11
#include <tuple>
#endif

namespace upcxx {
#ifdef UPCXX_HAVE_CXX11
  // Apply a function to a list of arguments stored in a std::tuple
  template<size_t N>
  struct apply {
    template<typename Function, typename... Ts, typename... Ts2>
    static void call(Function k, std::tuple<Ts...> t, Ts2... as) {
      upcxx::apply<N-1>::call(k, t, std::get<N-1>(t), as...);
    }
  };

  template<>
  struct apply<0> {
    template<typename Function, typename... Ts, typename... Ts2>
    static void call(Function k, std::tuple<Ts...> t, Ts2... as) {
      k(as...);
    }
  };
#endif
} // namespace upcxx
