/**
 * sample_sort.cpp -- a modified version of the sample sort algorithm
 *
 * The most difficult case is that some of the keys are identical,
 * which requires careful handling to be correct!
 *
 * Use the Mersenne Twister random number generator (SFMT)
 */

#include <upcxx.h>

using namespace upcxx;

#include <stdio.h>
#include <stdlib.h> // for the qsort() and rand()
#include <strings.h> // for bzero()
#include <time.h>
#include <sys/time.h>

//#include <iostream>     // std::cout
//#include <algorithm>    // std::sort
//#include <vector>       // std::vector

#define DEBUG

#define VERIFY

#define SFMT_MEXP 19937

extern "C" {
#include "../SFMT/SFMT.h" // for the Mersenne Twister random number generator
}

#define ELEMENT_T uint64_t
#define RANDOM_SEED 12345
#define SAMPLES_PER_THREAD 128
//#define KEYS_PER_THREAD 4 * 1024 * 1024
#define KEYS_PER_THREAD 4

typedef struct {
  // shared void *ptr;
  ptr_to_shared<void> ptr;
  uint64_t nbytes;
} buffer_t;

// shared [THREADS] buffer_t all_buffers[THREADS][THREADS];
buffer_t *all_buffers_src;
buffer_t *all_buffers_dst;

// shared [] ELEMENT_T * shared sorted[THREADS];
shared_array< ptr_to_shared<ELEMENT_T>, 1 > sorted; 

ELEMENT_T *splitters;

shared_array<ELEMENT_T, 1> keys;
 
// shared uint64_t sorted_key_counts[THREADS];
shared_array<uint64_t, 1> sorted_key_counts;

sfmt_t sfmt;

double mysecond()
{
  struct timeval tv;
  gettimeofday(&tv, 0);
  return tv.tv_sec + ((double) tv.tv_usec / 1000000);
}

void init_keys(ELEMENT_T *my_keys, uint64_t my_key_size)
{
#ifdef DEBUG
  printf("Thread %d, my_key_size %llu\n", MYTHREAD, my_key_size);
#endif
  sfmt_init_gen_rand(&sfmt, RANDOM_SEED + MYTHREAD);
  // The sfmt library only takes integer type of size
  uint64_t i;

  for (i = 0; i < my_key_size; i++) {
    my_keys[i] = (ELEMENT_T)sfmt_genrand_uint64(&sfmt) % (KEYS_PER_THREAD * THREADS);
  }
}

// Integer comparison function for qsort
int compare_element(const void * a, const void * b)
{
  if ( *(ELEMENT_T *)a > *(ELEMENT_T *)b ) {
    return 1;
  } else if ( *(ELEMENT_T *)a == *(ELEMENT_T *)b ) {
    return 0;
  }
  return -1; //*(ELEMENT_T *)a < *(ELEMENT_T *)b

}

void compute_splitters(uint64_t key_count, 
                       int samples_per_thread)
{
  splitters = (ELEMENT_T *)malloc(sizeof(ELEMENT_T) * THREADS);
  assert(splitters != NULL);

  if (MYTHREAD == 0) {
    ELEMENT_T *candidates;
    int candidate_count = THREADS * samples_per_thread;
    int i;

    candidates = (ELEMENT_T *)malloc(sizeof(ELEMENT_T) * candidate_count);
    assert(candidates != NULL);

    ELEMENT_T *my_splitters = (ELEMENT_T *)malloc(sizeof(ELEMENT_T) * THREADS);
    assert(my_splitters != NULL);

    // Sample the key space to find the partition splitters
    // Oversample by a factor "samples_per_thread"
    for (i = 0; i < candidate_count; i++) {
      uint64_t s = sfmt_genrand_uint64(&sfmt) % key_count;
      candidates[i] = keys[s]; // global accesses on keys
    }

    qsort(candidates, candidate_count, sizeof(ELEMENT_T), compare_element);
  
    // Subsample the candidates for the key splitters
    my_splitters[0] = candidates[0];  // We won't use this one.
    for (i = 1; i < THREADS; i++) {
      my_splitters[i] = candidates[i * samples_per_thread]; 
    }
    /*
    upc_all_broadcast(splitters, my_splitters, 
                      THREADS*sizeof(ELEMENT_T), 
                      UPC_IN_MYSYNC | UPC_OUT_MYSYNC);
    */
    upcxx::upcxx_bcast(my_splitters, splitters, sizeof(ELEMENT_T)*THREADS, 0);

    free(candidates);
    free(my_splitters);
  } else {
    // MYTHREAD != 0
    /*
    upc_all_broadcast(splitters, my_splitters, 
                      THREADS*sizeof(ELEMENT_T), 
                      UPC_IN_MYSYNC | UPC_OUT_MYSYNC);
    */
    upcxx::upcxx_bcast(NULL, splitters, sizeof(ELEMENT_T)*THREADS, 0);
  }
}

// re-distribute the keys according the splitters
void redistribute(uint64_t key_count)
{
  uint64_t i;
  int k;
  uint64_t offset;
  uint64_t *hist;
  ELEMENT_T *my_splitters = (ELEMENT_T *)&splitters[0];
  ELEMENT_T *my_keys = (ELEMENT_T *)&keys[MYTHREAD];
  ELEMENT_T my_new_key_count;

  hist = (uint64_t *)malloc(sizeof(uint64_t) * THREADS);
  assert(hist != NULL);

#ifdef DEBUG
  upcxx::barrier();
  for (k = 0; k < THREADS; k++) {
    int t;
    if (MYTHREAD == k) {
      printf("Thread %d:\n", k);
      for (t = 0; t < THREADS; t++) {
        printf("my_splitters[%d]=%llu ", t, my_splitters[t]);
      }
      printf("\n");
    }
    upcxx::barrier();
  }
#endif

  // local sort
  qsort(my_keys, KEYS_PER_THREAD, sizeof(ELEMENT_T), compare_element);

  // compute the local histogram from the splitters
  bzero(hist, sizeof(uint64_t) * THREADS);
  k = 0;
  for (i = 0; i < KEYS_PER_THREAD; i++) {
    if (my_keys[i] < my_splitters[k+1]) {
      hist[k]++;
    } else {
#ifdef DEBUG
      printf("Thread %d, hist[%d]=%llu\n", MYTHREAD, k, hist[k]);
#endif
      while (my_keys[i] >= my_splitters[k+1] && (k < THREADS - 1))
        k++;
      if (k == THREADS - 1) {
        hist[k] = KEYS_PER_THREAD - i;
#ifdef DEBUG
        printf("Thread %d, hist[%d]=%llu\n", MYTHREAD, k, hist[k]);
#endif
        break;
      } else {
        hist[k]++;
      }
    } 
  }

#ifdef DEBUG
  barrier();
  for (k = 0; k < THREADS; k++) {
    int i;
    if (MYTHREAD == k) {
      printf("Thread %d: ", k);
      for (i = 0; i < THREADS; i++) {
        printf("hist[%d]=%llu, ", i, hist[i]);
      }
      printf("\n"); fflush(stdout);
      for (i = 0; i < MIN(64, KEYS_PER_THREAD); i++) {
        printf("my_keys[%d]=%llu ", i, my_keys[i]);
      }
      printf("\n");
    }
    barrier();
  }
#endif

  all_buffers_src = (buffer_t *)malloc(sizeof(buffer_t) * THREADS);
  assert(all_buffers_src != NULL); 
 
  all_buffers_dst = (buffer_t *)malloc(sizeof(buffer_t) * THREADS);
  assert(all_buffers_dst != NULL);

  // Set up the buffer pointers
  offset = 0; // offset in elements
  for (i = 0; i < THREADS; i++) {
    all_buffers_src[i].ptr = &keys[MYTHREAD] + offset;
    all_buffers_src[i].nbytes = sizeof(ELEMENT_T) * hist[i];
    offset += hist[i];
  }

  upcxx_alltoall(all_buffers_src, all_buffers_dst, sizeof(buffer_t));

  sorted_key_counts[MYTHREAD] = 0;
  barrier();
  for (i = 0; i < THREADS; i++) {
    sorted_key_counts[MYTHREAD] += all_buffers_dst[i].nbytes / sizeof(ELEMENT_T);
  }

  // printf("sorted_key_counts[%d]=%llu\n", MYTHREAD, sorted_key_counts[MYTHREAD]);

  // sorted[MYTHREAD] = (shared [] ELEMENT_T *)upc_alloc(offset * sizeof(ELEMENT_T));
  sorted[MYTHREAD] = upcxx::allocate<ELEMENT_T>(MYTHREAD, offset);
  assert(sorted[MYTHREAD].get() != NULL);

  upcxx::barrier();

  // alltoallv
  offset = 0; // offset in bytes
  for (i = 0; i < THREADS; i++) {
    // Send data only if the size is non-zero
    if (all_buffers_dst[i].nbytes) {
#ifdef DEBUG
      printf("Thread %d, copy %llu bytes from (%d, %p) to (%d, %p)\n",
             MYTHREAD, all_buffers_dst[i].nbytes,
             (int)(all_buffers_dst[i].ptr.tid()),
             all_buffers_dst[i].ptr.raw_ptr(),
             (sorted[MYTHREAD].get() + offset / sizeof(ELEMENT_T)).tid(),
             (sorted[MYTHREAD].get() + offset / sizeof(ELEMENT_T)).raw_ptr());
#endif               
//       upc_memcpy(sorted[MYTHREAD] + offset / sizeof(ELEMENT_T),
//                  all_buffers[i][MYTHREAD].ptr,
//                  all_buffers[i][MYTHREAD].nbytes);
      upcxx::async_copy(all_buffers_dst[i].ptr,
                        sorted[MYTHREAD].get() + offset / sizeof(ELEMENT_T),  
                        all_buffers_dst[i].nbytes);    
      offset += all_buffers_dst[i].nbytes;
    }
  }
  upcxx::async_copy_fence();
  upcxx::barrier();

#ifdef DEBUG
  upcxx::barrier();
  for (k = 0; k < THREADS; k++) {
    int i;
    if (MYTHREAD == k) {
      printf("Thread %d:\n", k);
      for (i = 0; i < MIN(64, sorted_key_counts[k]); i++) {
        printf("my_sorted[%d]=%llu ", i, sorted[k].get()[i].get());
      }
      printf("\n");
    }
  upcxx:barrier();
  }
#endif
}

// Parallel sort across multiple threads
void sample_sort(uint64_t key_count)
{
  // shared int *result_keys;
  ptr_to_shared<int> result_keys;
  // shared unsigned int *histogram;
  ptr_to_shared<unsigned int> histogram;
  
  double starttime = mysecond();
  compute_splitters(KEYS_PER_THREAD, SAMPLES_PER_THREAD);
  upcxx::barrier();
  double cs_time = mysecond() - starttime;

  // Re-distribute the keys based on the splitters
  // Cannot use upc_all_exchange here because it's an alltoallv operation.
  redistribute(key_count);
  // There is a barrier at the end of redistribute().
  double rd_time = mysecond() - cs_time;

  // local sort
  // printf("qsort, sorted_key_counts[%d]=%llu\n", MYTHREAD, sorted_key_counts[MYTHREAD]);
  qsort((ELEMENT_T *)sorted[MYTHREAD].get(), 
        sorted_key_counts[MYTHREAD].get(), sizeof(ELEMENT_T), 
        compare_element);
  upcxx::barrier();
  double sort_time = mysecond() - rd_time;

  if (MYTHREAD == 0) {
    printf("Time for computing the splitters: %lg\n", cs_time);
    printf("Time for redistributing the keys: %lg\n", rd_time);
    printf("Time for the final local sort: %lg\n", sort_time);
  }
}

int main(int argc, char **argv)
{
  upcxx::init(&argc, &argv);

  uint64_t my_key_size = KEYS_PER_THREAD; // assume key_size is a multiple of THREADS
  uint64_t total_key_size = KEYS_PER_THREAD * THREADS;

  // keys = upc_all_alloc(total_key_size, sizeof(ELEMENT_T));
  // assert(keys != NULL);
  keys.init(total_key_size);
  sorted.init(THREADS);
  sorted_key_counts.init(THREADS);

#ifdef VERIFY
  // shared [] ELEMENT_T *keys_copy;
  ptr_to_shared<ELEMENT_T>keys_copy;
#endif
     
  // initialize the keys with random numbers
  init_keys((ELEMENT_T *)&keys[MYTHREAD], my_key_size);
  barrier();

#ifdef VERIFY
  if (MYTHREAD == 0) {
    int t;
    // gather the keys to thread 0
    // keys_copy = upc_alloc(sizeof(ELEMENT_T) * total_key_size);
    keys_copy = upcxx::allocate<ELEMENT_T>(MYTHREAD, total_key_size);
    assert(keys_copy != NULL);
    for (t = 0; t < THREADS; t++) {
      // upc_memcpy(&keys_copy[KEYS_PER_THREAD * t], &keys[t], sizeof(ELEMENT_T)*KEYS_PER_THREAD);
      upcxx::copy<ELEMENT_T>(&keys[t], keys_copy + (KEYS_PER_THREAD * t), KEYS_PER_THREAD);
    }
  }

#ifdef DEBUG
  if (MYTHREAD == 0) {
    int i;
    printf("Before sorting...\n");
    for (i = 0; i < MIN(64, total_key_size); i++) {
      printf("keys[%d]=%llu, ", i, keys[i].get());
    }
    printf("\n");
  }
#endif

  // Cannot use upc_all_gather because it requires a blocked layout
  // of the array, which is cyclic in out case!
  // Maybe we can still use upc_all_gather by casting "keys" to
  // (shared [KEYS_PER_THREAD] ELEMENT_T *) first.
  /*
    upc_all_gather(keys_copy, // shared void * restrict dst,
    keys,      // shared const void * restrict src, 
    my_key_size * sizeof(ELEMENT_T), // uint64_t nbytes,
    UPC_IN_MYSYNC | UPC_OUT_MYSYNC);
  */
#endif

  if (MYTHREAD == 0){
    printf("Somple sort... number of keys %llu.\n", total_key_size);
  }

  barrier();
  double starttime = mysecond();
  sample_sort(total_key_size);
  barrier();
  double total_time = mysecond() - starttime;

  if (MYTHREAD == 0) {
    printf("Sample sort time = %lg sec, %lg keys/s \n", total_time, 
           (double)total_key_size/total_time);
  }

#ifdef VERIFY
  if (MYTHREAD == 0) {
    uint64_t i, num_errors;
    ELEMENT_T *local_copy = (ELEMENT_T *)keys_copy;

    if (total_key_size > 1024 * 1024) {
      printf("Warning: total_key_size %llu is large, verification time may be long.\n",
             total_key_size);
    }

    printf("Verifying results...\n");

#ifdef DEBUG
  {
    int i;
    printf("Before sorting...\n");
    for (i = 0; i < MIN(64, total_key_size); i++) {
      printf("local_copy[%d]=%llu, ", i, local_copy[i]);
    }
    printf("\n");
  }
#endif

    qsort(local_copy, total_key_size, sizeof(ELEMENT_T), compare_element);

#ifdef DEBUG
    {
      int i;
      printf("After sorting...\n");
      for (i = 0; i < MIN(64, total_key_size); i++) {
        printf("local_copy[%d]=%llu, ", i, local_copy[i]);
      }
      printf("\n");
    }
#endif

    // compare sorted with keys
    num_errors = 0;
    int t = 0;
    uint64_t index = 0;
    ELEMENT_T current;
    for (i = 0; i < total_key_size; i++) {
#ifdef DEBUG
      printf("verifying: i %llu, t %d, index %llu\n", i, t, index);
#endif
      while (sorted_key_counts[t] == 0) t++;
      
      current = sorted[t].get()[index];
      if (local_copy[i] != current) {
#ifdef DEBUG
        printf("Verification error: %llu != expected %llu.\n", current, local_copy[i]);
#endif
        num_errors++;
      }
      
      index++;
      if (index == sorted_key_counts[t]) {
        t++;
        index = 0;
      }
    }

    if (num_errors) {
      printf("Verification errors %llu.\n", num_errors);
    } else {
      printf("Verification successful!\n");
    }
  }
  barrier();
#endif // end of VERIFY

#ifdef DEBUG
  if (MYTHREAD == 0) {
    int i, t;
    printf("After sorting...\n");
    for (t = 0; t < THREADS; t++) {
      printf("sorted_key_counts[%d] = %llu\n", t, sorted_key_counts[t].get());
      for (i = 0; i < MIN(64, sorted_key_counts[t]); i++) {
        printf("sorted[%d]=%llu, ", i, sorted[t].get()[i].get());
      }
      printf("\n");
    }
  }
#endif

  return 0;
}
