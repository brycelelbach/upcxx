/**
 * interfaces_internal.h - contains internal logic for implementing
 * the UPC++ globalization and local array interfaces.
 * \see interfaces.h for user-level interfaces
 * \see broadcast.h for an example of how the interface is used
 */

#pragma once

#include <global_ptr.h>

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

#if __cplusplus >= 201103L
  template<class T> struct assertion_trigger {
    enum { fail = 0 };
  };
# define FAIL assertion_trigger<T>::fail
#endif

  template<class T, int N> struct non_globalize_result<T *, N> {
#if __cplusplus >= 201103L
    static_assert(FAIL, "Cannot communicate non-global pointer");
#endif
  };

  template<class T> struct non_globalize_result<T, sizeof(char)> {
#if __cplusplus >= 201103L
    static_assert(FAIL, "Type requires globalization before communication");
#endif
  };

#if __cplusplus >= 201103L
# undef FAIL
#endif

  template<class T> struct int_type {
    typedef int type;
  };

  struct many_chars {
    char chars[100];
  };

#define SELECTOR_TYPE(T)                                                \
  typename int_type<typename U::global_type::type>::type

  template<class T> struct globalize_index {
    template<class U> static many_chars foo(...);
    template<class U> static char foo(SELECTOR_TYPE(T) x);
    enum { size = sizeof(foo<T>(0)) };
  };
#undef SELECTOR_TYPE

#define GLOBALIZE_TYPE(T)                                       \
  typename globalize_result<T, globalize_index<T>::size>::type
#define NONGLOBALIZE_TYPE(T)                                            \
  typename non_globalize_result<T, globalize_index<T>::size>::type

#define ARRAY_ELEM_TYPE(Array)                                  \
  typename Array::local_elem_type::type
#define ARRAY_NG_TYPE(Array)                            \
  NONGLOBALIZE_TYPE(ARRAY_ELEM_TYPE(Array))
} // namespace upcxx
