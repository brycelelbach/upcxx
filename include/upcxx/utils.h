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
  struct applier {
    template<typename Function, typename... Ts, typename... Ts2>
    static auto call(Function k, const std::tuple<Ts...>& t, Ts2... as) ->
      decltype(applier<N-1>::call(k, t, std::get<N-1>(t), as...)) {
      return applier<N-1>::call(k, t, std::get<N-1>(t), as...);
    }
  };

  template<>
  struct applier<0> {
    template<typename Function, typename... Ts, typename... Ts2>
    static auto call(Function k, const std::tuple<Ts...>& t, Ts2... as) ->
      decltype(k(as...)) {
      return k(as...);
    }
  };

  template<typename Function, typename... Ts>
  auto apply(Function k, const std::tuple<Ts...>& t) ->
    decltype(applier<sizeof...(Ts)>::call(k, t)) {
    return applier<sizeof...(Ts)>::call(k, t);
  }

  namespace util {
    template<int ...>
    struct seq { };

    template<int N, int ...S>
    struct gens : gens<N-1, N-1, S...> { };

    template<int ...S>
    struct gens<0, S...> {
      typedef seq<S...> type;
    };
  } // end of helper namespace

#endif
} // namespace upcxx
