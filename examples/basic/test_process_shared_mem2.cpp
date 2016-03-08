/**
 * \example test_process_shared_mem2.cpp
 *
 * Test process-shared memory features
 */
#include <upcxx.h>
#include <iostream>

using namespace upcxx;

int main(int argc, char **argv)
{
  init(&argc, &argv);

  // print out pshm_teams
  if (myrank() == 0) {
    for (uint32_t i=0; i<pshm_teams->size(); i++) {
      std::cout << "PSHM team " << i << ": ( ";
      for (uint32_t j=0; j<(*pshm_teams)[i].size(); j++) {
        std::cout << (*pshm_teams)[i][j] << " ";
      }
      std::cout << ")\n";
    }
  }
  barrier();
  
  void *p = allocate(0);
  global_ptr<void> gp = global_ptr<void>(p, (myrank()+1)%ranks());
  std::cout << "Rank " << myrank() << " p " << p 
            << " gp "  << gp << " localizing gp " << (void *)gp
            << std::endl;

  barrier();

  global_ptr<void> fake_gp = global_ptr<void>((void *)123, (myrank()+2)%ranks());
  std::cout << "Rank " << myrank() << " fake_gp " << fake_gp
            << " localizing fake_gp " << (void *)fake_gp
            << std::endl;

  barrier();

  if (!myrank())
    std::cout << "test_process_shared_mem2 passed!\n";

  finalize();
  return 0;
}
