#include <upcxx.h>
#include <stdio.h>

upcxx::shared_array<int> a;

int main(int argc, char **argv)
{
  upcxx::init(&argc, &argv);

  // init a with size 6*THREADS and blk_sz 2 
  a.init(6*THREADS, 2);

  if (MYTHREAD == 0) {
    printf("THREADS %d, array size %lu, block size %lu.\n",
           THREADS, a.size(), a.get_blk_sz());
    for (uint32_t i = 0; i < 6*THREADS; i++) {  
      a[i] = i;
    }
    for (uint32_t i = 0; i < 6*THREADS; i++) {  
      printf("a[%u] = %d ", i, (int)a[i]);
    }
    printf("\n\n");

    a.set_blk_sz(3);
    printf("THREADS %d, array size %lu, set block size %lu.\n",
           THREADS, a.size(), a.get_blk_sz());
    for (uint32_t i = 0; i < 6*THREADS; i++) {  
      printf("a[%u] = %d ", i, (int)a[i]);
    }
    printf("\n\n");

    a.set_blk_sz(1);
    printf("THREADS %d, array size %lu, set block size %lu.\n",
           THREADS, a.size(), a.get_blk_sz());
    for (uint32_t i = 0; i < 6*THREADS; i++) {  
      printf("a[%u] = %d ", i, (int)a[i]);
    }
    printf("\n\n");

    a.set_blk_sz(6);
    printf("THREADS %d, array size %lu, set block size %lu.\n",
           THREADS, a.size(), a.get_blk_sz());
    for (uint32_t i = 0; i < 6*THREADS; i++) {  
      printf("a[%u] = %d ", i, (int)a[i]);
    }
    printf("\n\n");
  }

  upcxx::finalize();
  return 0;
}
