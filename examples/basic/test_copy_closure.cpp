#include <cstdio>
#include <upcxx.h>
#include <upcxx/finish.h>

#ifndef UPCXX_HAVE_CXX11
int main(int argc, char **argv)
{
  upcxx::init(&argc, &argv);
  printf("This test is for C++11 only.  Please enable C++11 with UPC++ by --enable-cxx11 at configure time.\n");
  upcxx::finalize();
  return 0;
}
#else // UPCXX_HAVE_CXX11

template <typename T>
void lambda_wrapper(upcxx::global_ptr<T> remote_lambda)
{
  upcxx::global_ptr<T> my_lambda = upcxx::allocate<T>(upcxx::myrank(), 1);
  upcxx::copy(remote_lambda, my_lambda, 1);
  (*(T*)my_lambda)(); // execute the lambda
  upcxx::deallocate(my_lambda);
  upcxx::deallocate(remote_lambda);
}

int main(int argc, char **argv)
{
  upcxx::init(&argc, &argv);

  int i, j;
  upcxx::global_ptr<int> counter_ptr = upcxx::allocate<int>(upcxx::myrank(), 1);
  *counter_ptr = 0;

  upcxx::barrier();
  upcxx_finish {
    for (i=0; i<3; i++) {
      for (j=0; j<2; j++) {
        upcxx::rank_t src_rank = upcxx::myrank();
        auto lambda = [=] () {
          printf("lambda task from src rank %u, i %d, j %d, executing on rank %u\n",
                 src_rank, i, j, upcxx::myrank());
          *counter_ptr = *counter_ptr + upcxx::myrank();
        };
        upcxx::global_ptr<decltype(lambda)> remote_lambda
          = upcxx::allocate<decltype(lambda)>(src_rank, 1);
        memcpy(remote_lambda.raw_ptr(), &lambda, sizeof(decltype(lambda)));
        upcxx::async((src_rank+1)%upcxx::ranks())(lambda_wrapper<decltype(lambda)>, remote_lambda);
      }
    }
  } // close finish
  upcxx::barrier();

  printf("myrank %u, counter updated by my right (+1) neighbor is %d\n",
         upcxx::myrank(), (int)(*counter_ptr));

  upcxx::barrier();
  if (upcxx::myrank() == 0)
    printf("test_copy_closure passed!\n");

  upcxx::finalize();
  return 0;
}
#endif // close #ifndef UPCXX_HAVE_CXX11
