// Test copy of unit-width and non-unit-width ghost zones.

#include <iostream>
#include <upcxx.h>
#include <upcxx/array.h>

using namespace upcxx;

void test(ndarray<int, 3> A) {
  ndarray<ndarray<int, 3, global>, 1> B(RD(ranks()));
  B.exchange(A);
  upcxx_foreach (p, A.domain()) {
    A[p] = 128 * 128 * 128 * p[1] + 128 * 128 * p[2] + 128 * p[3] + myrank();
  };
  barrier();
  if (myrank() == 0) {
    int checked = 0;
    B[1].copy(A);
    std::cout << "domains: " << A.domain() << ", " << B[1].domain() << std::endl;
    std::cout << "intersection: " << (A.domain() * B[1].domain()) << std::endl;
    upcxx_foreach (p, A.domain() * B[1].domain()) {
      if (A[p] != B[1][p]) {
        std::cout << "error: mismatch at " << p << ": " << A[p]
                  << " != " << B[1][p] << std::endl;
      }
      checked++;
    };
    std::cout << "checked " << checked << " elements" << std::endl;
  }
}

int main(int argc, char** argv) {
  init(&argc, &argv);
  if (ranks() < 2) {
    std::cout << "test requires at least 2 ranks" << std::endl;
    finalize();
    std::exit(0);
  }
  int start = 128*myrank(), end = 129*(myrank()+1), end2 = 130*(myrank()+1);
  ndarray<int, 3> A(RD(PT(0, 0, start), PT(128, 128, end)));
  ndarray<int, 3> B(RD(PT(0, start, 0), PT(128, end, 128)));
  ndarray<int, 3> C(RD(PT(start, 0, 0), PT(end, 128, 128)));
  ndarray<int, 3> D(RD(PT(0, 0, start), PT(128, 128, end2)));
  ndarray<int, 3> E(RD(PT(0, start, 0), PT(128, end2, 128)));
  ndarray<int, 3> F(RD(PT(start, 0, 0), PT(end2, 128, 128)));
  ndarray<int, 3> G(RD(PT(0, start, start), PT(128, end, end)));
  ndarray<int, 3> H(RD(PT(start, start, 0), PT(end, end, 128)));
  ndarray<int, 3> I(RD(PT(start, 0, start), PT(end, 128, end)));
  ndarray<int, 3> J(RD(PT(0, start, start), PT(128, end2, end2)));
  ndarray<int, 3> K(RD(PT(start, start, 0), PT(end2, end2, 128)));
  ndarray<int, 3> L(RD(PT(start, 0, start), PT(end2, 128, end2)));
  ndarray<int, 3> M(RD(PT(start, start, start), PT(end, end, end)));
  ndarray<int, 3> N(RD(PT(start, start, start), PT(end2, end2, end2)));
  test(A);
  test(B);
  test(C);
  test(D);
  test(E);
  test(F);
  test(G);
  test(H);
  test(I);
  test(J);
  test(K);
  test(L);
  test(M);
  test(N);
  finalize();
}
