#ifndef MYFFT_H__
#define MYFFT_H__ 

#include <math.h>

#ifndef MAX
#define MAX(A, B) ((A) > (B) ? (A) : (B))
#endif
#ifndef MIN
#define MIN(A, B) ((A) < (B) ? (A) : (B))
#endif

#ifdef __cplusplus
extern "C"{
#endif
  
  /*
  static inline double time()
  {
    struct timeval tv;
    gettimeofday(&tv, 0);
    return tv.tv_sec + ((double) tv.tv_usec / 1000000);
  }
  */
  
  typedef struct {
    double real;
    double imag;
  } complex_t;

  typedef struct {
    float real;
    float imag;
  } complex_float_t;

  typedef enum {FFT_FORWARD=1, FFT_BACKWARD=-1} fftdir_t;

  /** 
   * in, istride and idist describe the input * array(s). There are
   * howmany input arrays; the first one is pointed to by in, the second
   * one is pointed to by in + idist, and so on, up to in + (howmany -
   * 1) * idist. Each input array consists of complex numbers (see
   * Section Data Types), which are not necessarily contiguous in
   * memory. Specifically, in[0] is the first element of the first
   * array, in[istride] is the second element of the first array, and so
   * on. In general, the i-th element of the j-th input array will be in
   * position in[i * istride + j * idist].  out, ostride and odist
   * describe the output array(s). The format is the same as for the
   * input array.  In-place transforms: If the plan specifies an
   * in-place transform, ostride and odist are always ignored. If out is
   * NULL, out is ignored, too. Otherwise, out is interpreted as a
   * pointer to an array of n complex numbers, that FFTW will use as
   * temporary space to perform the in-place computation. out is used as
   * scratch space and its contents destroyed. In this case, out must be
   * an ordinary array whose elements are contiguous in memory (no
   * striding).
   */
  struct fft_plan_t {
    int instride;
    int indist;
    int outstride;
    int outdist;
    fftdir_t direction;
    int len;
    int numffts;
    void *lib_plan;
  };

  int fft_init();

  void fft_plan_1d(fftdir_t direction, 
                   int len, 
                   int numffts,
                   complex_t *out,
                   int outstride, 
                   int outdist,
                   complex_t *in,
                   int instride, 
                   int indist,
                   struct fft_plan_t *plan);

  void run_1d_fft(complex_t * out, complex_t *in, struct fft_plan_t *plan);

  char *fft_id_str();

  // local 2-D FFT
  void fft2(complex_t *out, complex_t *in, int n0, int n1);

#ifdef __cplusplus
}
#endif

#endif
