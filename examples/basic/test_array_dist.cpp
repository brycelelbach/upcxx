// Test distributed copy operations.

#include <iostream>
#include <upcxx.h>
#include <upcxx/array.h>

using namespace std;
using namespace upcxx;

#define println(str) cout << myrank() << ": " << str << endl

void test_copy() {
  rectdomain<1> rd1(0, 10);
  rectdomain<1> rd2(0, 10, 2);
  domain<1> d = rd2 + RD(0, 5);
  ndarray<int, 1, global> arrA1(rd1);
  ndarray<int, 1, global> arrB1;
  ndarray<ndarray<int, 1, global>, 1> allArrA1s(RD((int) ranks()));

  upcxx_foreach (p, rd1) {
    arrA1[p] = 100 * myrank() + p[1];
  };

  allArrA1s.exchange(arrA1);
  arrB1 = allArrA1s[(int) ranks()-1];

  // test copy operations
  if (myrank() == 0) println("testing copy operations...");
  barrier();
  {
    ndarray<int, 1, global> arrA2(rd1);
    arrA2.copy(arrB1);
    upcxx_foreach (p, rd1) {
      if (arrA2[p] != arrB1[p]) {
        println("error: mismatch at " << p << ", " << arrA2[p] <<
                " != " << arrB1[p]);
      }
    };
  }
  barrier();
  {
    ndarray<int, 1, strided, global> arrA2(rd2);
    arrA2.copy(arrB1);
    upcxx_foreach (p, rd2) {
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
    upcxx_foreach (p, rd1) {
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
    upcxx_foreach (p, rd1) {
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
    ndarray<point<1>, 1> parr(RD(0, (int) d.size()));
    int idx = 0;
    upcxx_foreach (p, d) {
      parr[idx++] = p;
    };
    arrA2.copy(arrB1, parr);
    upcxx_foreach (p, rd1) {
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

  test_copy();
  barrier();
  if (myrank() == 0) println("done.");

  return 0;
}
