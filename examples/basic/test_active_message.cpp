/**
 * \example test_active_message.cpp
 */

#include <upcxx.h>
#include <iostream>
#include <cstdio>

using namespace upcxx;

const int count = 128;
const int expected_a = 12345;
const int expected_b = 23456;
const int expected_c = 34567;
const int expected_d = 45678;
void *expected_p0 = (void *)0xFF00EE22;
void *expected_p1 = (void *)0x11AA22BB;

void unpack2(uint32_t src_rank, void *payload, size_t nbytes, int a, int b)
{
  printf("Rank %u unpack2: src_rank %u, nbytes %lu, a %d, b %d\n",
         myrank(), src_rank, nbytes, a , b);

  assert(a == expected_a);
  assert(b == expected_b);

  int *p = (int *)payload;
  for (uint32_t i = 0; i < count; i++) {
    int expected = src_rank*1000 + i;
    if (p[i] != expected) {
      printf("Rank %u error: p[%u]=%d != %d as expected!\n",
             myrank(), i, p[i], expected);
      exit(1);
    }
  }
}

void unpack4(uint32_t src_rank, void *payload, size_t nbytes, int a, int b, int c, int d)
{
  printf("Rank %u unpack4: src_rank %u, nbytes %lu, a %d, b %d, c %d, d %d\n",
         myrank(), src_rank, nbytes, a , b, c, d);

  assert(a == expected_a);
  assert(b == expected_b);
  assert(c == expected_c);
  assert(d == expected_d);

  int *p = (int *)payload;
  for (uint32_t i = 0; i < count; i++) {
    int expected = src_rank*1000 + i;
    if (p[i] != expected) {
      printf("Rank %u error: p[%u]=%d != %d as expected!\n",
             myrank(), i, p[i], expected);
      exit(1);
    }
  }
}

void unpack2p2i(uint32_t src_rank, void *payload, size_t nbytes, void *p0, void *p1, int a, int b)
{
  printf("Rank %u unpack4: src_rank %u, nbytes %lu, p0 %p, p1 %p, a %d, b %d\n",
         myrank(), src_rank, nbytes, p0, p1, a , b);
  assert(a == expected_a);
  assert(b == expected_b);
  assert(p0 == expected_p0);
  assert(p1 == expected_p1);
  int *p = (int *)payload;
  for (uint32_t i = 0; i < count; i++) {
    int expected = src_rank*1000 + i;
    if (p[i] != expected) {
      printf("Rank %u error: p[%u]=%d != %d as expected!\n",
             myrank(), i, p[i], expected);
      exit(1);
    }
  }
}

int main(int argc, char **argv)
{
  init(&argc, &argv);

  printf("Rank %u will send %u unpack2 active messages...\n",
         myrank(), ranks());

  int *payload;
  payload = new int[count];

  for (uint32_t i = 0; i < count; i++) {
    payload[i] = myrank()*1000 + i;
  }

  for (uint32_t i = 0; i < ranks(); i++) {
    uint32_t dst_rank = (i+1)%ranks();
    am_send(dst_rank, unpack2, payload, sizeof(int)*count, expected_a, expected_b);
  }

  barrier();

  printf("\nRank %u will send %u unpack4 active messages...\n",
         myrank(), ranks());

  for (uint32_t i = 0; i < ranks(); i++) {
    uint32_t dst_rank = (i+1)%ranks();
    am_send(dst_rank, unpack4, payload, sizeof(int)*count, expected_a, expected_b, expected_c, expected_d);
  }

  barrier();

  printf("\nRank %u will send %u unpack2p2i active messages...\n",
         myrank(), ranks());

  for (uint32_t i = 0; i < ranks(); i++) {
    uint32_t dst_rank = (i+1)%ranks();
    am_send(dst_rank, unpack2p2i, payload, sizeof(int)*count, expected_p0, expected_p1, expected_a, expected_b);
  }

  if (myrank() == 0) {
    printf("Passed test_active_message!\n");
  }

  upcxx::finalize();
  return 0;
}
