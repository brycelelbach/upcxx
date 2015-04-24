/**
 * \example test_team.cpp
 *
 * Test teams and collective operations on teams
 *
 */

#include <upcxx.h>

#include <iostream>
#include <stdio.h>
#include <unistd.h> // for sleep()
#include <cmath> // for sqrt()

using namespace upcxx;

shared_array<int> sa; // size == ranks()

int test_barrier(const team &t)
{
  //std::cout << global_myrank() << ": starts team barrier on " << t << "\n";
  t.barrier();
  //std::cout << global_myrank() << ": finished team barrier on " << t << "\n";

  // Delayed test
  uint32_t slow_rank;
  for (slow_rank=0; slow_rank<t.size(); slow_rank++) {
    uint32_t slow_global_rank = t.team_rank_to_global(slow_rank);
    int expected = 123456789 + slow_global_rank;
    
    if (t.myrank() == slow_rank) {
      sa[slow_global_rank] = 0;
    }
    t.barrier();
    
    if (t.myrank() == slow_rank) {
      sleep(1); // sleep 1 sec
      sa[slow_global_rank] = expected;
    }
    t.barrier();
    
    if (sa[slow_global_rank] != expected) {
      std::cerr << global_myrank() << ": team barrier error: I exited the barrier before the slow rank enters it. Team " << t << "\n";
      exit(1);
    }
    t.barrier();
  } // close of for (slow_rank=0; slow_rank<t.ranks(); slow_rank++)
  return UPCXX_SUCCESS;
}

template<typename T>
int test_bcast(team t, size_t count)
{
  global_ptr<T> src;
  global_ptr<T> dst;
    
  src = allocate<T>(myrank(), count);
  dst = allocate<T>(myrank(), count);
    
#ifdef DEBUG
  cerr << global_myrank() << " scr: " << src << "\n";
  cerr << global_myrank() << " dst: " << dst << "\n";
#endif

  t.barrier();
  
  for (uint32_t root = 0; root < t.size(); root++) {
    // Initialize data pointed by host_ptr by a local pointer
    T *lsrc = (T *)src;
    T *ldst = (T *)dst;
    for (size_t i=0; i<count; i++) {
      lsrc[i] = (T)(t.color()*10000 + root*100 + i);
      ldst[i] = (T)0;
    }

    t.bcast(lsrc, ldst, sizeof(T)*count, root);

    // Verify the received data
    for (size_t i=0; i<count; i++) {
      T expected = (T)(t.color()*10000 + root*100 + i);
      if (ldst[i] != expected) {
        std::cout << global_myrank() << ": test_bcast error!  Expected to receive "
                  << expected << " at " << i << "th element, but received " << ldst[i]
                  << " instead.\n";
        exit(1);
      }
    }
  }
  t.barrier();
}

int main(int argc, char **argv)
{
  init(&argc, &argv);
  sa.init(ranks());
 
  team *row_team, *col_team;
  uint32_t nrows, ncols, row_id, col_id;

  if (argc > 1) {
    ncols = atoi(argv[1]);
  }

  // try to fix ncols if it is incorrect
  if (ncols < 1 || ncols > ranks()) {
    ncols = 1;
  }
  nrows = ranks() / ncols;
  
  std::cout << "team_all: " << team_all << "\n";

  if (myrank() == 0)
    std::cout << "Testing team barrier on team_all...\n";
  
  test_barrier(team_all);

  team_all.barrier();

  if (myrank() == 0)
     std::cout << "Testing team bcast on team_all...\n";

  test_bcast<size_t>(team_all, 1024);

  if (myrank() == 0)
    std::cout << "Passed testing team_all!\n";
  
  if (myrank() == 0)
    printf("Splitting team_all into %u row teams and %u column teams...\n",
           nrows, ncols);
 
  row_id = myrank() / ncols;
  col_id = myrank() % ncols;

  printf("Global rank %u: row_id %u, col_id %u\n", myrank(), row_id, col_id);
  team_all.split(row_id, col_id, row_team); // Row teams
  team_all.split(col_id, row_id, col_team); // Col teams

  barrier();
  
  if (myrank() == 0)
    std::cout << "Finish team split\n";

  if (myrank() == 0)
    std::cout << "Testing collectives on row teams...\n";

  test_barrier(*row_team);
  test_bcast<uint32_t>(*row_team, 4096);
  
  if (myrank() == 0)
    std::cout << "Passed testing collectives on row teams...\n";

  if (myrank() == 0)
    std::cout << "Testing collectives on column teams...\n";

  test_barrier(*col_team);
  test_bcast<double>(*col_team, 4096);

  if (myrank() == 0)
    std::cout << "Passed testing collectives on column teams...\n";

  finalize();
  return 0;
}
