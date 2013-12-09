/* This test implements a safe broadcast scheme that converts local
   pointers and Titanium-style arrays to global pointers and arrays
   when broadcasting. Any type can be added to this scheme by defining
   a global_type typedef within the type and an explicit conversion to
   that global_type. */

#include <iostream>
#include <upcxx.h>
#include <global_ptr.h>
#include <collective.h>
#include <array.h>

using namespace upcxx;

template<class T, int N> struct broadcast_result {
  typedef T type;
  enum { index = 0 };
};

template<class T, int N> struct broadcast_result<T *, N> {
  typedef global_ptr<T> type;
  enum { index = 1 };
};

template<class T> struct broadcast_result<T, sizeof(char)> {
  typedef typename T::global_type type;
  enum { index = 2 };
};

template<class T> struct int_type {
  typedef int type;
};

struct many_chars {
  char chars[100];
};

template<class T> struct broadcast_index {
  template<class U> static many_chars foo(...);
  template<class U> static char foo(typename int_type<typename U::global_type>::type x);
  enum { size = sizeof(foo<T>(0)) };
};

#define BROADCAST_TYPE(T) typename broadcast_result<T, broadcast_index<T>::size>::type
#define BROADCAST_INDEX(T) broadcast_result<T, broadcast_index<T>::size>::index

template<class T> BROADCAST_TYPE(T) broadcast(T val, int src) {
  BROADCAST_TYPE(T) sval = (BROADCAST_TYPE(T)) val;
  BROADCAST_TYPE(T) result;
  std::cout << broadcast_index<T>::size << ", " << BROADCAST_INDEX(T) << endl;
  upcxx_bcast(&sval, &result, sizeof(BROADCAST_TYPE(T)), src);
  return result;
}

int main(int argc, char **argv) {
  init(&argc, &argv);

  {
    ndarray<int, 1> arr(RECTDOMAIN((0), (5)));
    broadcast(arr, 0);
  }
  {
    global_ndarray<int, 1> arr(RECTDOMAIN((0), (5)));
    broadcast(arr, 0);
  }
  {
    int *arr = new int[3];
    broadcast(arr, 0);
  }
  {
    int arr = 3;
    broadcast(arr, 0);
  }

  finalize();
  return 0;
}
