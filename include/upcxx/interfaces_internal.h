/**
 * interfaces_internal.h - contains internal logic for implementing
 * the UPC++ globalization and local array interfaces.
 * @see interfaces.h for user-level interfaces
 * @see broadcast.h for an example of how the interface is used
 */

#pragma once

#include <upcxx/global_ptr.h>

namespace upcxx {
  template<class T, int N> struct globalize_result {
    typedef T type;
  };

  template<class T, int N> struct globalize_result<T *, N> {
    typedef global_ptr<T> type;
  };

  template<class T> struct globalize_result<T, sizeof(char)> {
    typedef typename T::global_type::type type;
  };

  template<class T, int N> struct non_globalize_result {
    typedef T type;
  };

#ifdef UPCXX_HAVE_CXX11
  template<class T> struct assertion_trigger {
    enum { fail = 0 };
  };
# define UPCXXI_FAIL assertion_trigger<T>::fail
#endif

  template<class T, int N> struct non_globalize_result<T *, N> {
#ifdef UPCXX_HAVE_CXX11
    static_assert(UPCXXI_FAIL,
                  "Cannot communicate non-global pointer");
#endif
  };

  template<class T> struct non_globalize_result<T, sizeof(char)> {
#ifdef UPCXX_HAVE_CXX11
    static_assert(UPCXXI_FAIL,
                  "Type requires globalization before communication");
#endif
  };

#undef UPCXXI_FAIL

  template<class T> struct int_type {
    typedef int type;
  };

  struct many_chars {
    char chars[100];
  };

#define UPCXXI_SELECTOR_TYPE(T)                                 \
  typename int_type<typename U::global_type::type>::type

  template<class T> struct globalize_index {
    template<class U> static many_chars foo(...);
    template<class U> static char foo(UPCXXI_SELECTOR_TYPE(T) x);
    enum { size = sizeof(foo<T>(0)) };
  };
#undef UPCXXI_SELECTOR_TYPE

#define UPCXXI_GLOBALIZE_TYPE(T)                                \
  typename globalize_result<T, globalize_index<T>::size>::type
#define UPCXXI_NONGLOBALIZE_TYPE(T)                                     \
  typename non_globalize_result<T, globalize_index<T>::size>::type

#define UPCXXI_ARRAY_ELEM_TYPE(Array)           \
  typename Array::local_elem_type::type
#define UPCXXI_ARRAY_NG_TYPE(Array)                             \
  UPCXXI_NONGLOBALIZE_TYPE(UPCXXI_ARRAY_ELEM_TYPE(Array))
} // namespace upcxx
