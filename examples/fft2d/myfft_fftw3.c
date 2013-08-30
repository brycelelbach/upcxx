#include <fftw3.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#if MAKE_THREAD_SAFE
#include <pthread.h>
#endif

#include "myfft.h"

static unsigned int num_threads=1;

int fft_init()
{
#if FFTW_MT
  char *buf;
  
  buf = getenv("OMP_NUM_THREADS");
  if (buf != NULL)
    num_threads = atoi(buf);

  if (num_threads < 1)
    num_threads = 1;

  // printf("FFTW: num_threads %d\n", num_threads);

  return fftw_init_threads();
#else  
  return 1;
#endif
}

char *fft_id_str(char *buffer) {
  strcpy(buffer, "FFTW3");
  return buffer;
}

void fft_plan_1d(fftdir_t direction, 
                 int len, 
                 int numffts, 
                 complex_t *sample_outarray,
                 int outstride, 
                 int outdist, 
                 complex_t *sample_inarray,
                 int instride, 
                 int indist,
                 struct fft_plan_t *plan) 
{
  fftw_plan *lib_plan;
#if MAKE_THREAD_SAFE
  static pthread_mutex_t plan_lock = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_lock(&plan_lock);
#endif

#if FFTW_MT
  fftw_plan_with_nthreads(num_threads);
#endif
  
  lib_plan = malloc(sizeof(fftw_plan));
  *lib_plan = fftw_plan_many_dft(1, &len, numffts,
                               (fftw_complex*) sample_inarray, NULL, instride, indist,
                               (fftw_complex*) sample_outarray, NULL, outstride, outdist, 
                               (direction == FFT_FORWARD ? FFTW_FORWARD : FFTW_BACKWARD),
                               FFTW_ESTIMATE); // FFTW_MEASURE); 
  
  plan->instride = instride;
  plan->outstride =outstride;
  plan->indist = indist;
  plan->outdist = outdist;
  plan->direction = direction;
  plan->len = len;
  plan->numffts = numffts;
  plan->lib_plan = lib_plan;
#if MAKE_THREAD_SAFE
  pthread_mutex_unlock(&plan_lock);
#endif
}

void run_1d_fft(complex_t *out, complex_t *in, struct fft_plan_t *plan) 
{
  fftw_plan *lib_plan = (fftw_plan*) plan->lib_plan;
  fftw_execute_dft(*lib_plan, (fftw_complex *)in, (fftw_complex *)out);
}


/**
 * Compute 2-D FFT from *in to *out, note that the performance is not
 * optimized here as we create an ad hoc plan everytime instread of
 * resuing it.
 */
void fft2(complex_t *out, complex_t *in, int n0, int n1)
{
  fftw_plan plan;

  plan = fftw_plan_dft_2d(n0, n1, (fftw_complex *)in, (fftw_complex *)out,
                          FFTW_FORWARD, FFTW_ESTIMATE);
  fftw_execute(plan); 
  fftw_destroy_plan(plan);
}
