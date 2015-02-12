/* Tests safe broadcast scheme defined in <broadcast.h>. */

#include <iostream>
#include <upcxx.h>
#include <global_ptr.h>
#include <collective.h>
#include <array.h>
#include <broadcast.h>

using namespace upcxx;

int main(int argc, char **argv) {
  init(&argc, &argv);

  {
    ndarray<int, 1> arr(RD(0, 5));
    broadcast(arr, 0);
  }
  {
    ndarray<int, 1, global> arr(RD(0, 5));
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
    broadcast(src, dst, 3, 0);
  }
  {
    ndarray<int, 1> src(RD(0, 3));
    ndarray<int, 1> dst(RD(0, 3));
    broadcast(src, dst, 0);
  }
  {
    ndarray<ndarray<int, 1, global>, 1> src(RD(0, 3));
    ndarray<ndarray<int, 1, global>, 1> dst(RD(0, 3));
    broadcast(src, dst, 0);
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
  {
    ndarray<int, 1, global> src(RD(0, 3));
    ndarray<int, 1, global> dst(RD(0, 3));
    broadcast(src, dst, 0);
  }
  {
    ndarray<int *, 1> src(RD(0, 3));
    ndarray<int *, 1> dst(RD(0, 3));
    broadcast(src, dst, 0);
  }
  {
    ndarray<ndarray<int, 1>, 1> src(RD(0, 3));
    ndarray<ndarray<int, 1>, 1> dst(RD(0, 3));
    broadcast(src, dst, 0);
  }
#endif

  finalize();
  return 0;
}
