#include <global_ptr.h>
#include <collective.h>

namespace upcxx {
  template<class T, int N> struct globalize_result {
    typedef T type;
  };

  template<class T, int N> struct globalize_result<T *, N> {
    typedef global_ptr<T> type;
  };

  template<class T> struct globalize_result<T, sizeof(char)> {
    typedef typename T::global_type type;
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

#define SELECTOR_TYPE(T)                                \
  typename int_type<typename U::global_type>::type

  template<class T> struct globalize_index {
    template<class U> static many_chars foo(...);
    template<class U> static char foo(SELECTOR_TYPE(T) x);
    enum { size = sizeof(foo<T>(0)) };
  };
#undef SELECTOR_TYPE

#define GLOBALIZE_TYPE(T)                                       \
  typename globalize_result<T, globalize_index<T>::size>::type

  template<class T> GLOBALIZE_TYPE(T) broadcast(T val, int root) {
    GLOBALIZE_TYPE(T) sval = (GLOBALIZE_TYPE(T)) val;
    GLOBALIZE_TYPE(T) result;
    upcxx_bcast(&sval, &result, sizeof(GLOBALIZE_TYPE(T)), root);
    return result;
  }
#undef GLOBALIZE_TYPE

#define NONGLOBALIZE_TYPE(T)                                            \
  typename non_globalize_result<T, globalize_index<T>::size>::type

  template<class T> void broadcast(NONGLOBALIZE_TYPE(T) * src,
                                   T *dst, size_t count, int root) {
    upcxx_bcast(src, dst, count*sizeof(T), root);
  }
#undef NONGLOBALIZE_TYPE

#define ARRAY_ELEM_TYPE(Array)                  \
  typename Array::local_elem_type

  template<class Array> void broadcast(Array src, Array dst, int root,
                                       ARRAY_ELEM_TYPE(Array) * = 0) {
    upcxx_bcast(src.storage_ptr(), dst.storage_ptr(),
                src.size()*sizeof(ARRAY_ELEM_TYPE(Array)), root);
  }
#undef ARRAY_ELEM_TYPE
}
