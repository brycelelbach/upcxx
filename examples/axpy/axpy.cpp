/**
 * AXPY example
 *
 * usage example: mpirun -n 4 ./axpy
 */

#include <cstdio>

#include "degas.h"
#include "shared_array.h"
#include "startup.h" // single-thread execution

using namespace upcxx;

// Syntactic sugar
typedef event* async_event;
typedef range place;
typedef shared_array<float> Seq;

const size_t N = 128; // vector size

// cyclic distribution of global array x and y
Seq x(N);
Seq y(N);

void saxpy(group g,
           float a)
{
  for (int i = my_node.id(); i < x.size(); i += g.size())
    y[i] = a * x[i] + y[i];
}

void print_floats(Seq& result,
                  size_t limit=16)
{
  for (size_t k=0; k<min(N,limit); ++k) {
    printf(" %3.0f", float(result[k]));
    if (((k+1) & 15) == 0)  printf("\n");
  }
  printf("\n");
}

void report_discrepancies(Seq& result,
                          float answer)
{
  int nwrong = 0;

  for (int k=0; k<N; ++k)
    if (result[k] != answer)
      ++nwrong;

  if (nwrong>0)
    printf("FAILED: found %d discrepancies\n", nwrong);
  else
    printf("PASSED: all values correct\n");
}

int main(int argc, char **argv)
{
  upcxx::init(&argc, &argv);
  int P = global_machine.cpu_count();
  place cpus = global_machine.get_nodes(P);

  set(&x, (float)1.0);
  set(&y, (float)20.0);

  printf("Running AXPY on %d cpu places...\n", P);

  async(my_node, cpus, streaming(P))(saxpy, streaming(P), 2.0f);
  upcxx::wait();

  print_floats(y);

  printf("Verifying results ...\n");
  report_discrepancies(y, 22);
  
  return 0;
}
