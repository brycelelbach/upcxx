#include <global_ptr.h>
#include <collective.h>

template<class T, int N> struct broadcast_result {
  typedef T type;
};

template<class T, int N> struct broadcast_result<T *, N> {
  typedef global_ptr<T> type;
};

template<class T> struct broadcast_result<T, sizeof(char)> {
  typedef typename T::global_type type;
};

template<class T> struct int_type {
  typedef int type;
};

struct many_chars {
  char chars[100];
};

#define BROADCAST_TYPE(T)                                       \
  typename broadcast_result<T, broadcast_index<T>::size>::type

#define SELECTOR_TYPE(T)                                \
  typename int_type<typename U::global_type>::type

template<class T> struct broadcast_index {
  template<class U> static many_chars foo(...);
  template<class U> static char foo(SELECTOR_TYPE(T) x);
  enum { size = sizeof(foo<T>(0)) };
};

template<class T> BROADCAST_TYPE(T) broadcast(T val, int src) {
  BROADCAST_TYPE(T) sval = (BROADCAST_TYPE(T)) val;
  BROADCAST_TYPE(T) result;
  upcxx_bcast(&sval, &result, sizeof(BROADCAST_TYPE(T)), src);
  return result;
}

#undef BROADCAST_TYPE
#undef SELECTOR_TYPE

