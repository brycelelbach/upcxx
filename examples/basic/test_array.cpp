/* Dan's tester for Ti-array operations */
#include <iostream>
#include <upcxx.h>
#include <array.h>

#if __cplusplus >= 201103L
// test passing raw initializer lists to array ops
#define POINTLIT(...) { __VA_ARGS__ }
#else
#define POINTLIT POINT
#endif

#ifndef Array
# define Array ndarray
#endif
#ifndef Global
# define Global , global
#endif

using namespace std;
using namespace upcxx;

int main() {
  cout << "Running array operations tests.." << endl;
  { /* test some descriptor operations */
    Array<long, 1 Global> x = ARRAY(long, ((0), (100), (1))); // x has domain 0..99
    for (int i=0; i <= 99; i++) x[i] = i; // init to easy values
    foreach (p, x.domain()) {
      if (x[p] != p[1]) 
        cout << "Mismatch detected in x at " << p << ": expected: " <<
          p[1] << "  got: " << x[p] << endl;
    };

    rectdomain<1> interiorpts = x.domain().shrink(5);
    Array<long, 1 Global> y = x.constrict(interiorpts);  // y has domain 5..94
    if (y.domain().min()[1] != 5 || y.domain().max()[1] != 94)
      cout << "Constrict failed. y.domain=" << y.domain() << endl;
    foreach (p, y.domain()) {
      if (y[p] != p[1]) 
        cout << "Mismatch detected in y at " << p << ": expected: " <<
          p[1] << "  got: " << y[p] << endl;
    };

    // test translation
    Array<long, 1 Global> z = y.translate(POINTLIT(100)); // z has domain 105..194
    if (z.domain().min()[1] != 105 || z.domain().max()[1] != 194)
      cout << "Translate failed. z.domain=" << z.domain() << endl;
    foreach (p, z.domain()) {
      if (z[p] != p[1]-100) 
        cout << "Mismatch detected in z at " << p << ": expected: " <<
          (p[1]-100) << "  got: " << z[p] << endl;
    };
    Array<long, 1 Global> z2 = z.translate(POINTLIT(100)); // z2 has domain 205..294
    if (z2.domain().min()[1] != 205 || z2.domain().max()[1] != 294)
      cout << "Translate failed. z2.domain=" << z2.domain() << endl;
    foreach (p, z2.domain()) {
      if (z2[p] != p[1]-200) 
        cout << "Mismatch detected in z2 at " << p << ": expected: " <<
          (p[1]-200) << "  got: " << z2[p] << endl;
    };
    Array<long, 1 Global> z3 = z2.translate(POINTLIT(-250)); // z has domain -45..44
    if (z3.domain().min()[1] != -45 || z3.domain().max()[1] != 44)
      cout << "Translate failed. z3.domain=" << z3.domain() << endl;
    foreach (p, z3.domain()) {
      if (z3[p] != p[1]+50) 
        cout << "Mismatch detected in z3 at " << p << ": expected: " <<
          (p[1]+50) << "  got: " << z3[p] << endl;
    };
  }

  /* some basic array copy tests */
  { /* contiguous copy with differing base */
    Array<int, 1 Global> x = ARRAY(int, ((1), (101)));
    Array<int, 1 Global> y = ARRAY(int, ((50), (151)));
    foreach (p, x.domain()) { x[p] = p[1]; };
    foreach (p, y.domain()) { y[p] = p[1]+1000; };
    x.copy(y);
    foreach (p, x.domain()) {
      int expected;
      if (p[1] < 50) expected = p[1];
      else expected = p[1]+1000;
      if (x[p] != expected) 
        cout << "Mismatch detected in copy test 1 at " << p <<
          ": expected: " << expected <<
          "  got: " << x[p] << endl;
    };
  }
 
  { /* contiguous copy with non-trivial stride */
    Array<int, 1 Global> x = ARRAY(int, ((10), (101), (10)));
    Array<int, 1 Global> y = (ARRAY(int, ((1), (11)))).inject(POINTLIT(10));
    foreach (p, x.domain()) { x[p] = p[1]; };
    foreach (p, y.domain()) { y[p] = p[1]+1000; };
    x.copy(y);
    foreach (p, x.domain()) {
      int expected = p[1]+1000;
      if (x[p] != expected) 
        cout << "Mismatch detected in copy test 2 at " << p <<
          ": expected: " << expected <<
          "  got: " << x[p] << endl;
    };
  }

  { /* tranpose from contiguous -> contiguous */
    Array<int, 2 Global> x = ARRAY(int, ((1, 11), (6, 21)));
    Array<int, 2 Global> y = (ARRAY(int, ((11, 1), (21, 6)))).permute(POINTLIT(2,1));
    foreach (p, x.domain()) { x[p] = p[1]*100+p[2]; };
    foreach (p, y.domain()) { y[p] = p[1]*100+p[2]+1000; };
    x.copy(y);
    foreach (p, x.domain()) {
      int expected = p[1]*100+p[2]+1000;
      if (x[p] != expected) 
        cout << "Mismatch detected in copy test 3 at " << p <<
          ": expected: " << expected <<
          "  got: " << x[p] << endl;
    };
  }

  { /* equal sideFactor/stride ratio */
    Array<int, 2 Global> x = ARRAY(int, ((1, 11), (6, 21), (2, 1)));
    Array<int, 2 Global> y = ARRAY(int, ((1, 11), (6, 16)));
    foreach (p, x.domain()) { x[p] = p[1]*100+p[2]; };
    foreach (p, y.domain()) { y[p] = p[1]*100+p[2]+1000; };
    x.copy(y);
    foreach (p, x.domain()) {
      int expected;
      if (p[2] > 15) expected = p[1]*100+p[2];
      else expected = p[1]*100+p[2]+1000;
      if (x[p] != expected) 
        cout << "Mismatch detected in copy test 4 at " << p <<
          ": expected: " << expected <<
          "  got: " << x[p] << endl;
    };
  }

  {
    // test overlap (contiguous)
    Array<int, 1 Global> x = ARRAY(int, ((0), (7)));
    foreach (p, x.domain()) { x[p] = 100+p[1]; };
    Array<int, 1 Global> y = x.translate(POINTLIT(2));
    x.copy(y);
    foreach (p, x.domain()) {
      int expected;
      if (p[1] < 2) expected = 100+p[1];
      else expected = 100+p[1]-2;
      if (x[p] != expected) 
        cout << "Mismatch detected in copy overlap test 1 at " << p <<
          ": expected: " << expected <<
          "  got: " << x[p] << endl;
    };
  }

  {
    // test overlap (non-contiguous)
    Array<int, 1 Global> x = (ARRAY(int, ((0), (61)))).constrict(RECTDOMAIN((0), (61), (10)));
    foreach (p, x.domain()) { x[p] = 100+p[1]; };
    Array<int, 1 Global> y = x.translate(POINTLIT(20));
    x.copy(y);
    foreach (p, x.domain()) {
      int expected;
      if (p[1] < 20) expected = 100+p[1];
      else expected = 100+p[1]-20;
      if (x[p] != expected) 
        cout << "Mismatch detected in copy overlap test 2 at " << p <<
          ": expected: " << expected <<
          "  got: " << x[p] << endl;
    };
  }

  cout << "done." << endl;
  finalize();
  return 0;
}
