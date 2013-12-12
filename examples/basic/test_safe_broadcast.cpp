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
#include "../mg/broadcast.h"

using namespace upcxx;

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
  {
    int *src = new int[3];
    int *dst = new int[3];
    for (int i = 0; i < 3; i++) src[i] = MYTHREAD+1;
    broadcast(src, dst, 3, 0);
  }
#if 0 // these should fail
  {
    int **src = new int*[3];
    int **dst = new int*[3];
    broadcast(src, dst, 3, 0);
  }
  {
    ndarray<int, 1> *src = new ndarray<int, 1>[3];
    ndarray<int, 1> *dst = new ndarray<int, 1>[3];
    broadcast(src, dst, 3, 0);
  }
#endif

  finalize();
  return 0;
}
