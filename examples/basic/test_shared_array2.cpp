#include <upcxx.h>
#include <stdio.h>

// size 6*ranks() and blk_sz 2
upcxx::shared_array<int> a(6*upcxx::ranks(), 2);

int main(int argc, char **argv)
{
  if (upcxx::myrank() == 0) {
    printf("ranks() %d, array size %lu, block size %lu.\n",
           upcxx::ranks(), a.size(), a.get_blk_sz());
    for (uint32_t i = 0; i < 6*upcxx::ranks(); i++) {
      a[i] = i;
    }
    for (uint32_t i = 0; i < 6*upcxx::ranks(); i++) {
      printf("a[%u] = %d ", i, (int)a[i]);
    }
    printf("\n\n");

    a.set_blk_sz(3);
    printf("ranks() %d, array size %lu, set block size %lu.\n",
           upcxx::ranks(), a.size(), a.get_blk_sz());
    for (uint32_t i = 0; i < 6*upcxx::ranks(); i++) {
      printf("a[%u] = %d ", i, (int)a[i]);
    }
    printf("\n\n");

    a.set_blk_sz(1);
    printf("ranks() %d, array size %lu, set block size %lu.\n",
           upcxx::ranks(), a.size(), a.get_blk_sz());
    for (uint32_t i = 0; i < 6*upcxx::ranks(); i++) {
      printf("a[%u] = %d ", i, (int)a[i]);
    }
    printf("\n\n");

    a.set_blk_sz(6);
    printf("ranks() %d, array size %lu, set block size %lu.\n",
           upcxx::ranks(), a.size(), a.get_blk_sz());
    for (uint32_t i = 0; i < 6*upcxx::ranks(); i++) {
      printf("a[%u] = %d ", i, (int)a[i]);
    }
    printf("\n\n");
  }

  upcxx::barrier();
  if (upcxx::myrank() == 0)
    printf("test_shared_array2 passed!\n");

  return 0;
}
