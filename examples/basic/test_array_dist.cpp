// Test distributed copy operations.

#include <iostream>
#include <upcxx.h>
#include <array.h>

using namespace std;
using namespace upcxx;

#define println(str) cout << MYTHREAD << ": " << str << endl

void test_copy() {
  rectdomain<1> rd1 = RECTDOMAIN((0), (10));
  rectdomain<1> rd2 = RECTDOMAIN((0), (10), (2));
  domain<1> d = rd2 + RECTDOMAIN((0), (5));
  ndarray<int, 1, global> arrA1(rd1);
  ndarray<int, 1, global> arrB1;
  ndarray<ndarray<int, 1, global>, 1> allArrA1s(rectdomain<1>((int) THREADS));

  foreach (p, rd1) {
    arrA1[p] = 100 * MYTHREAD + p[1];
  };

  allArrA1s.exchange(arrA1);
  arrB1 = allArrA1s[THREADS-1];

  // test copy operations
  if (MYTHREAD == 0) println("testing copy operations...");
  barrier();
  {
    ndarray<int, 1, global> arrA2(rd1);
    arrA2.copy(arrB1);
    foreach (p, rd1) {
      if (arrA2[p] != arrB1[p]) {
        println("error: mismatch at " << p << ", " << arrA2[p] <<
                " != " << arrB1[p]);
      }
    };
  }
  barrier();
  {
    ndarray<int, 1, global> arrA2(rd2);
    arrA2.copy(arrB1);
    foreach (p, rd2) {
      if (arrA2[p] != arrB1[p]) {
        println("error: mismatch at " << p << ", " << arrA2[p] <<
                " != " << arrB1[p]);
      }
    };
  }
  barrier();
  {
    ndarray<int, 1, global> arrA2(rd1);
    arrA2.copy(arrB1, rd2);
    foreach (p, rd1) {
      if (rd2.contains(p) && arrA2[p] != arrB1[p]) {
        println("error: mismatch at " << p << ", " << arrA2[p] <<
                " != " << arrB1[p]);
      }
      if (!rd2.contains(p) && arrA2[p] == arrB1[p]) {
        println("error: unexpected match at " << p << ", " <<
                arrA2[p] << " == " << arrB1[p]);
      }
    };
  }
  barrier();
  {
    ndarray<int, 1, global> arrA2(rd1);
    arrA2.copy(arrB1, d);
    foreach (p, rd1) {
      if (d.contains(p) && arrA2[p] != arrB1[p]) {
        println("error: mismatch at " << p << ", " << arrA2[p] <<
                " != " << arrB1[p]);
      }
      if (!d.contains(p) && arrA2[p] == arrB1[p]) {
        println("error: unexpected match at " << p << ", " <<
                arrA2[p] << " == " << arrB1[p]);
      }
    };
  }
  barrier();
  {
    ndarray<int, 1, global> arrA2(rd1);
    ndarray<point<1>, 1> parr(RECTDOMAIN((0), ((int) d.size())));
    int idx = 0;
    foreach (p, d) {
      parr[idx++] = p;
    };
    arrA2.copy(arrB1, parr);
    foreach (p, rd1) {
      if (d.contains(p) && arrA2[p] != arrB1[p]) {
        println("error: mismatch at " << p << ", " << arrA2[p] <<
                " != " << arrB1[p]);
      }
      if (!d.contains(p) && arrA2[p] == arrB1[p]) {
        println("error: unexpected match at " << p << ", " <<
                arrA2[p] << " == " << arrB1[p]);
      }
    };
  }
}

int main(int argc, char **argv) {
  init(&argc, &argv);
  test_copy();
  barrier();
  if (MYTHREAD == 0) println("done.");
  finalize();
  return 0;
}
