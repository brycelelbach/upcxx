/* Dan's tester for Ti-array operations */
#include <iostream>
#include <upcxx.h>
#include <array.h>

using namespace std;
using namespace upcxx;

static int value(int id, point<3> p) {
  return id * 1000 + p[1] * 100 + p[2] * 10 + p[3];
}

static void set(ndarray<int, 3, global> arr, int id) {
  foreach (p, arr.domain()) {
    arr[p] = value(id, p);
    // cout << id << ", " << p << " -> " << arr[p] << endl;
  }
}

static void check(ndarray<int, 3, global> src,
                  ndarray<int, 3, global> dst,
                  int sid, int did) {
  foreach (p, src.domain() * dst.domain()) {
    if (dst[p] != value(sid, p))
      cout << "incorrect result in " << did << " at " << p << ": "
           << dst[p] << endl;
  }
  foreach (p, dst.domain() - src.domain()) {
    if (dst[p] != value(did, p))
      cout << "incorrect result in " << did << " at " << p << ": "
           << dst[p] << endl;
  }
}

int main(int argc, char **argv) {
  init(&argc, &argv);
  if (MYTHREAD == 0) {
    cout << "Testing planned array copies..." << endl;
  }
  ndarray<int, 3, global> a1, a2, a3, a4, a5, a6;
  ndarray<ndarray<int, 3, global>, 1> exc(RD(THREADS));
  if (MYTHREAD == 0) {
    a1.create(RD(PT(0, 0, 0), PT(5, 5, 5)));
    a3.create(RD(PT(0, 0, 0), PT(3, 3, 3)));
    a5.create(RD(PT(1, 1, 1), PT(4, 4, 4)));
  }
  if (MYTHREAD == THREADS-1) {
    a2.create(RD(PT(1, 1, 1), PT(4, 4, 4)));
    a4.create(RD(PT(0, 0, 0), PT(3, 3, 3)));
    a6.create(RD(PT(0, 0, 0), PT(5, 5, 5)));
  }
  exc.exchange(a2);
  if (MYTHREAD == 0) {
    a2 = exc[THREADS-1];
  }
  exc.exchange(a4);
  if (MYTHREAD == 0) {
    a4 = exc[THREADS-1];
  }
  exc.exchange(a6);
  if (MYTHREAD == 0) {
    a6 = exc[THREADS-1];
  }
  copy_plan<int, 3> plan;
  if (MYTHREAD == 0) {
    plan.add(a2, a1, 1);
    plan.add(a4, a3, 2);
    plan.add(a6, a5, 1);
  }
  plan.setup();
  if (MYTHREAD == 0) {
    set(a1, 1);
    set(a3, 3);
    set(a5, 5);
  }
  if (MYTHREAD == THREADS-1) {
    set(a2, 2);
    set(a4, 4);
    set(a6, 6);
  }
  barrier();
  plan.copy(1);
  barrier();
  if (MYTHREAD == 0) {
    check(a1, a1, 1, 1);
    check(a1, a2, 1, 2);
    check(a3, a3, 3, 3);
    check(a4, a4, 4, 4);
    check(a5, a5, 5, 5);
    check(a5, a6, 5, 6);
    set(a1, 7);
    set(a3, 8);
    set(a5, 9);
  }
  barrier();
  plan.copy(2);
  barrier();
  if (MYTHREAD == 0) {
    check(a1, a1, 7, 7);
    check(a1, a2, 1, 2);
    check(a3, a3, 8, 8);
    check(a3, a4, 8, 4);
    check(a5, a5, 9, 9);
    check(a5, a6, 5, 6);
    cout << "done." << endl;
  }
  finalize();
  return 0;
}
