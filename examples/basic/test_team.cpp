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

shared_array<int> sa;

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
int test_bcast(const team &t, size_t count)
{
  global_ptr<T> src;
  global_ptr<T> dst;

  src = allocate<T>(myrank(), count);
  dst = allocate<T>(myrank(), count);

#ifdef DEBUG
  std::cerr << global_myrank() << " scr: " << src << "\n";
  std::cerr << global_myrank() << " dst: " << dst << "\n";
#endif

  t.barrier();

  for (uint32_t root = 0; root < t.size(); root++) {
    // Initialize data pointed by host_ptr by a local pointer
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

  deallocate(src);
  deallocate(dst);

  return UPCXX_SUCCESS;
}

template<typename T>
int test_gather(const team &t, size_t count)
{
  global_ptr<T> src;
  global_ptr<T> dst;

  // The total number of elements that the root expects to receive
  size_t allcounts = count * t.size();

  src = allocate<T>(myrank(), count);
  dst = allocate<T>(myrank(), allcounts);

  assert(src != NULL);
  assert(dst != NULL);

  t.barrier();

  for (uint32_t root = 0; root < t.size(); root++) {
    // Initialize data pointed by host_ptr by a local pointer
    T *lsrc = (T *)src;
    T *ldst = (T *)dst;
    for (size_t i=0; i<count; i++) {
      lsrc[i] = (T)(t.color()*10000 + root * 100 + t.myrank()*count + i);
    }
    for (size_t i=0; i<allcounts; i++) {
      ldst[i] = (T)0;
    }

    t.gather(lsrc, ldst, sizeof(T)*count, root);

    // Verify the received data on the root
    if (t.myrank() == root) {
      for (size_t i=0; i<allcounts; i++) {
        T expected = (T)(t.color()*10000 + root*100 + i);
        if (ldst[i] != expected) {
          std::cout << global_myrank() << ": test_gather error!  Expected to receive "
                    << expected << " at " << i << "th element, but received " << ldst[i]
                    << " instead.\n";
          exit(1);
        }
      }
    }
  }
  t.barrier();

  deallocate(src);
  deallocate(dst);

  return UPCXX_SUCCESS;
}

template<typename T>
int test_scatter(const team &t, size_t count)
{
  global_ptr<T> src;
  global_ptr<T> dst;

  // The total number of elements that the root expects to receive
  size_t allcounts = count * t.size();

  src = allocate<T>(myrank(), allcounts);
  dst = allocate<T>(myrank(), count);

  assert(src != NULL);
  assert(dst != NULL);

  t.barrier();

  for (uint32_t root = 0; root < t.size(); root++) {
    // Initialize data pointed by host_ptr by a local pointer
    T *lsrc = (T *)src;
    T *ldst = (T *)dst;
    for (size_t i=0; i<allcounts; i++) {
      lsrc[i] = (T)(t.color()*10000 + root * 100 + i);
    }
    for (size_t i=0; i<count; i++) {
      ldst[i] = (T)0;
    }

    t.scatter(lsrc, ldst, sizeof(T)*count, root);

    // Verify the received data
    {
      for (size_t i=0; i<count; i++) {
        T expected = (T)(t.color()*10000 + root*100 + t.myrank()*count + i);
        if (ldst[i] != expected) {
          std::cout << global_myrank() << ": test_scatter error!  Expected to receive "
                    << expected << " at " << i << "th element, but received " << ldst[i]
                    << " instead.\n";
          exit(1);
        }
      }
    }
  }
  t.barrier();

  deallocate(src);
  deallocate(dst);

  return UPCXX_SUCCESS;
}

template<typename T>
int test_allgather(const team &t, size_t count)
{
  global_ptr<T> src;
  global_ptr<T> dst;

  // The total number of elements that the root expects to receive
  size_t allcounts = count * t.size();

  src = allocate<T>(myrank(), count);
  dst = allocate<T>(myrank(), allcounts);

  assert(src != NULL);
  assert(dst != NULL);

  t.barrier();

  // Not really using root in allgather, just iterating the loop a few times
  for (uint32_t root = 0; root < t.size(); root++) {
    // Initialize data pointed by host_ptr by a local pointer
    T *lsrc = (T *)src;
    T *ldst = (T *)dst;
    for (size_t i=0; i<count; i++) {
      lsrc[i] = (T)(t.color()*10000 + root * 100 + t.myrank()*count + i);
    }
    for (size_t i=0; i<allcounts; i++) {
      ldst[i] = (T)0;
    }

    t.allgather(lsrc, ldst, sizeof(T)*count);

    // Verify the received data
    {
      for (size_t i=0; i<allcounts; i++) {
        T expected = (T)(t.color()*10000 + root*100 + i);
        if (ldst[i] != expected) {
          std::cout << global_myrank() << ": test_allgather error!  Expected to receive "
                    << expected << " at " << i << "th element, but received " << ldst[i]
                    << " instead.\n";
          exit(1);
        }
      }
    }
  }
  t.barrier();

  deallocate(src);
  deallocate(dst);

  return UPCXX_SUCCESS;
}

template<typename T>
int test_alltoall(const team &t, size_t count)
{
  global_ptr<T> src;
  global_ptr<T> dst;

  // The total number of elements that the root expects to receive
  size_t allcounts = count * t.size();

  src = allocate<T>(myrank(), allcounts);
  dst = allocate<T>(myrank(), allcounts);

  assert(src != NULL);
  assert(dst != NULL);

  t.barrier();

  // Not really using root in alltoall, just iterating the loop a few times
  for (uint32_t root = 0; root < t.size(); root++) {
    // Initialize data pointed by host_ptr by a local pointer
    T *lsrc = (T *)src;
    T *ldst = (T *)dst;
    for (size_t i=0; i<allcounts; i++) {
      lsrc[i] = (T)(t.color()*10000 + root * 100 + t.myrank()*10 + i);
    }
    for (size_t i=0; i<allcounts; i++) {
      ldst[i] = (T)0;
    }

    t.alltoall(lsrc, ldst, sizeof(T)*count);

    // Verify the received data
    {
      for (size_t i=0; i<allcounts; i++) {
        T expected = (T)(t.color()*10000 + root*100 + i/count*10 + t.myrank()*count + i%count);
        if (ldst[i] != expected) {
          std::cout << global_myrank() << ": test_alltoall error!  Expected to receive "
                    << expected << " at " << i << "th element, but received " << ldst[i]
                    << " instead.\n";
          exit(1);
        }
      }
    }
  }
  t.barrier();

  deallocate(src);
  deallocate(dst);
  return UPCXX_SUCCESS;
}

// Note: we are assuming the floating-point number operations are
// commutative and associative in the reduce
template<typename T>
int test_reduce(const team &t, size_t count)
{
  global_ptr<T> src;
  global_ptr<T> dst;

  src = allocate<T>(myrank(), count);
  dst = allocate<T>(myrank(), count);

  assert(src != NULL);
  assert(dst != NULL);

  t.barrier();

  // Test reduce sum
  for (uint32_t root = 0; root < t.size(); root++) {
    // Initialize data pointed by host_ptr by a local pointer
    T *lsrc = (T *)src;
    T *ldst = (T *)dst;
    for (size_t i=0; i<count; i++) {
      lsrc[i] = (T)t.myrank() * i;
    }
    for (size_t i=0; i<count; i++) {
      ldst[i] = (T)0;
    }

    // YZ: Need a find a way to translate T to UPCXX_ data type
    t.reduce(lsrc, ldst, count, root, UPCXX_SUM);

    // Verify the received data on the root
    if (t.myrank() == root) {
      for (size_t i=0; i<count; i++) {
        T expected = (T)(i * (t.size()-1) * (t.size() / 2.0));
        if (ldst[i] != expected) {
          std::cout << global_myrank() << ": test_reduction error!  Expected to receive "
                    << expected << " at " << i << "th element, but received " << ldst[i]
                    << " instead.\n";
          exit(1);
        }
      }
    }
  }
  t.barrier();

  // Test reduce max

  // Test reduce

  deallocate(src);
  deallocate(dst);

  return UPCXX_SUCCESS;
}

template<typename T>
int test_allreduce(const team &t, size_t count)
{
  return UPCXX_SUCCESS;
}

int main(int argc, char **argv)
{
  team *row_team, *col_team;
  uint32_t nrows, ncols = 1, row_id, col_id;

  upcxx::init(&argc, &argv);
  sa.init(ranks()); // size = ranks()

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
    std::cout << "Testing team gather on team_all...\n";

  test_gather<size_t>(team_all, 128);

  if (myrank() == 0)
    std::cout << "Testing team scatter on team_all...\n";

  test_scatter<size_t>(team_all, 8);

  if (myrank() == 0)
    std::cout << "Testing team allgather on team_all...\n";

  test_allgather<size_t>(team_all, 32);

  if (myrank() == 0)
    std::cout << "Testing team alltoall on team_all...\n";

  test_alltoall<size_t>(team_all, 64);

  if (myrank() == 0)
    std::cout << "Testing team reduce on team_all...\n";

  test_reduce<unsigned long long>(team_all, 1024);
  test_reduce<double>(team_all, 1024);

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
    std::cout << "Testing team barrier on row teams...\n";

  test_barrier(*row_team);

  if (myrank() == 0)
      std::cout << "Testing team bcast on row teams...\n";

  test_bcast<uint32_t>(*row_team, 4096);

  if (myrank() == 0)
      std::cout << "Testing team gather on row teams...\n";

  test_gather<uint32_t>(*row_team, 100);

  if (myrank() == 0)
      std::cout << "Testing team scatter on row teams...\n";

  test_scatter<uint32_t>(*row_team, 99);

  if (myrank() == 0)
      std::cout << "Testing team allgather on row teams...\n";

  test_allgather<uint32_t>(*row_team, 101);

  if (myrank() == 0)
    std::cout << "Testing team alltoall on row teams...\n";

  test_alltoall<uint32_t>(*row_team, 600);

  for (size_t sz = 1; sz < 4096*1024; sz *=4) {
    if (myrank() == 0)
      std::cout << "Testing team reduce (sz=" << sz << " bytes) on row teams...\n";

    test_reduce<int>(*row_team, sz);
  }

  if (myrank() == 0)
    std::cout << "Passed testing collectives on row teams...\n";

  if (myrank() == 0)
    std::cout << "Testing team barrier on column teams...\n";

  test_barrier(*col_team);

  if (myrank() == 0)
      std::cout << "Testing team bcast on column teams...\n";

  test_bcast<double>(*col_team, 4096);

  if (myrank() == 0)
      std::cout << "Testing team gather on column teams...\n";

  test_gather<double>(*col_team, 1);

  if (myrank() == 0)
      std::cout << "Testing team scatter on column teams...\n";

  test_scatter<double>(*col_team, 1024);

  if (myrank() == 0)
    std::cout << "Testing team allgather on column teams...\n";

  test_allgather<double>(*col_team, 512);

  if (myrank() == 0)
    std::cout << "Testing team alltoall on column teams...\n";

  test_alltoall<double>(*col_team, 311);

  for (size_t sz = 1; sz < 4096*1024; sz *=4) {
    if (myrank() == 0)
      std::cout << "Testing team reduce (sz=" << sz << " bytes) on column teams...\n";

    test_reduce<double>(*col_team, sz);
  }

  if (myrank() == 0)
    std::cout << "Passed testing collectives on column teams...\n";

  upcxx::finalize();
  return 0;
}
