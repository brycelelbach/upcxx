#include "FT.h"

#if USE_FFTW3
# include <fftw3.h>
#else
# include <fftw.h>
#endif

static int s_nx, s_ny, s_nz;
static int numPencilsInYDimSlab, numPencilsInZDimSlab;
static fftw_plan forwardPlan_x, forwardPlan_y, forwardPlan_z;
static fftw_plan backwardPlan_x, backwardPlan_y, backwardPlan_z;

void FT::createPlans(ndarray<Complex, 3> subArray1,
                     ndarray<Complex, 3> subArray2,
                     ndarray<Complex, 3> subArray3,
                     ndarray<Complex, 3> subArray4,
                     ndarray<Complex, 3> subArray5,
                     ndarray<Complex, 3> subArray6,
                     int p_nx, int p_ny, int p_nz,
                     int p_numPencilsInYDimSlab,
                     int p_numPencilsInZDimSlab) {
  fftw_complex *array1 = (fftw_complex*) subArray1.storage_ptr();
  fftw_complex *array2 = (fftw_complex*) subArray2.storage_ptr();
  fftw_complex *array3 = (fftw_complex*) subArray3.storage_ptr();
  fftw_complex *array4 = (fftw_complex*) subArray4.storage_ptr();
  fftw_complex *array5 = (fftw_complex*) subArray5.storage_ptr();
  fftw_complex *array6 = (fftw_complex*) subArray6.storage_ptr();

  s_nx = p_nx;
  s_ny = p_ny;
  s_nz = p_nz;
  numPencilsInYDimSlab = p_numPencilsInYDimSlab;
  numPencilsInZDimSlab = p_numPencilsInZDimSlab;

#if USE_FFTW3
  forwardPlan_y = fftw_plan_many_dft(1, &s_ny, s_nz,
                                     array1, NULL, s_nz+PADDING, 1,
                                     array1, NULL, s_nz+PADDING, 1,
                                     FFTW_FORWARD, FFTW_MEASURE);
  backwardPlan_y = fftw_plan_many_dft(1, &s_nz, s_nx,
                                      array3, NULL, s_nx+PADDING, 1,
                                      array4, NULL, s_nx+PADDING, 1,
                                      FFTW_BACKWARD, FFTW_MEASURE);
# ifdef SLABS
  forwardPlan_x = fftw_plan_many_dft(1, &s_nx, s_nz,
                                     array2, NULL,
                                     numPencilsInYDimSlab*(s_nz+PADDING), 1,
                                     array3, NULL, 1, s_nx+PADDING,
                                     FFTW_FORWARD, FFTW_MEASURE);
  backwardPlan_x = fftw_plan_many_dft(1, &s_ny, s_nx,
                                      array5, NULL,
                                      numPencilsInZDimSlab*(s_nx+PADDING), 1,
                                      array6, NULL, 1, s_ny+PADDING,
                                      FFTW_BACKWARD, FFTW_MEASURE);
  forwardPlan_z = fftw_plan_many_dft(1, &s_nz, numPencilsInYDimSlab,
                                     array1, NULL, 1, s_nz+PADDING,
                                     array1, NULL, 1, s_nz+PADDING,
                                     FFTW_FORWARD, FFTW_MEASURE);
  backwardPlan_z = fftw_plan_many_dft(1, &s_nx, numPencilsInZDimSlab,
                                      array4, NULL, 1, s_nx+PADDING,
                                      array4, NULL, 1, s_nx+PADDING,
                                      FFTW_BACKWARD, FFTW_MEASURE);
# else
  forwardPlan_x = fftw_plan_many_dft(1, &s_nx, s_nz,
                                     array2, NULL, s_nz+PADDING, 1,
                                     array3, NULL, 1, s_nx+PADDING,
                                     FFTW_FORWARD, FFTW_MEASURE);
  backwardPlan_x = fftw_plan_many_dft(1, &s_ny, s_nx,
                                      array5, NULL, s_nx+PADDING, 1,
                                      array6, NULL, 1, s_ny+PADDING,
                                      FFTW_BACKWARD, FFTW_MEASURE);
  forwardPlan_z = fftw_plan_many_dft(1, &s_nz, 1,
                                     array1, NULL, 1, s_nz+PADDING,
                                     array1, NULL, 1, s_nz+PADDING,
                                     FFTW_FORWARD, FFTW_MEASURE);
  backwardPlan_z = fftw_plan_many_dft(1, &s_nx, 1,
                                      array4, NULL, 1, s_nx+PADDING,
                                      array4, NULL, 1, s_nx+PADDING,
                                      FFTW_BACKWARD, FFTW_MEASURE);
# endif
#else // USE_FFTW3
  forwardPlan_y = fftw_create_plan_specific(s_ny, FFTW_FORWARD,
                                            FFTW_MEASURE | FFTW_IN_PLACE,
                                            array1, s_nz+PADDING,
                                            array1, s_nz+PADDING);
  backwardPlan_y = fftw_create_plan_specific(s_nz, FFTW_BACKWARD,
                                             FFTW_MEASURE,
                                             array3, s_nx+PADDING,
                                             array4, s_nx+PADDING);
# ifdef SLABS
  forwardPlan_x = fftw_create_plan_specific(s_nx, FFTW_FORWARD, FFTW_MEASURE,
                                            array2,
                                            numPencilsInYDimSlab*(s_nz+PADDING),
                                            array3, 1);
  backwardPlan_x = fftw_create_plan_specific(s_ny, FFTW_BACKWARD,
                                             FFTW_MEASURE, array5,
                                             numPencilsInZDimSlab*(s_nx+PADDING),
                                             array6, 1);
  forwardPlan_z = fftw_create_plan_specific(s_nz, FFTW_FORWARD,
                                            FFTW_MEASURE | FFTW_IN_PLACE,
                                            array1, 1, array1, 1);
  backwardPlan_z = fftw_create_plan_specific(s_nx, FFTW_BACKWARD,
                                             FFTW_MEASURE | FFTW_IN_PLACE,
                                             array4, 1, array4, 1);
# else
  forwardPlan_x = fftw_create_plan_specific(s_nx, FFTW_FORWARD, FFTW_MEASURE,
                                            array2, s_nz+PADDING, array3, 1);
  backwardPlan_x = fftw_create_plan_specific(s_ny, FFTW_BACKWARD, FFTW_MEASURE,
                                             array5, s_nx+PADDING, array6, 1);
  forwardPlan_z = fftw_create_plan_specific(s_nz, FFTW_FORWARD,
                                            FFTW_MEASURE | FFTW_IN_PLACE,
                                            array1, 1, array1, 1);
  backwardPlan_z = fftw_create_plan_specific(s_nx, FFTW_BACKWARD,
                                             FFTW_MEASURE | FFTW_IN_PLACE,
                                             array4, 1, array4, 1);
# endif
#endif // USE_FFTW3
}


void FT::pencilToPencilStrideForwardY(ndarray<Complex, 2> inArray,
                                      ndarray<Complex, 1> outArray) {
  fftw_complex *in = (fftw_complex*) inArray.storage_ptr();
  fftw_complex *out = (fftw_complex*) outArray.storage_ptr();

#if USE_FFTW3
  fftw_execute_dft(forwardPlan_y, in, out);
#else
  fftw(forwardPlan_y, s_nz, in, s_nz+PADDING, 1, out, s_nz+PADDING, 1);
#endif
}

void FT::pencilToPencilStrideBackwardY(ndarray<Complex, 2> inArray,
                                       ndarray<Complex, 2> outArray) {
  fftw_complex *in = (fftw_complex*) inArray.storage_ptr();
  fftw_complex *out = (fftw_complex*) outArray.storage_ptr();

#if USE_FFTW3
  fftw_execute_dft(backwardPlan_y, in, out);
#else
  fftw(backwardPlan_y, s_nx, in, s_nx+PADDING, 1, out, s_nx+PADDING, 1);
#endif
}

#ifdef SLABS
void FT::slabToUnitStrideForwardX(ndarray<Complex, 2> inArray,
                                  ndarray<Complex, 2> outArray) {
  fftw_complex *in = (fftw_complex*) inArray.storage_ptr();
  fftw_complex *out = (fftw_complex*) outArray.storage_ptr();

# if USE_FFTW3
  fftw_execute_dft(forwardPlan_x, in, out);
# else
  fftw(forwardPlan_x, s_nz, in, numPencilsInYDimSlab*(s_nz+PADDING), 1,
       out, 1, s_nx+PADDING);
# endif
}

void FT::slabToUnitStrideBackwardX(ndarray<Complex, 2> inArray,
                                   ndarray<Complex, 2> outArray) {
  fftw_complex *in = (fftw_complex*) inArray.storage_ptr();
  fftw_complex *out = (fftw_complex*) outArray.storage_ptr();

# if USE_FFTW3
  fftw_execute_dft(backwardPlan_x, in, out);
# else
  fftw(backwardPlan_x, s_nx, in, numPencilsInZDimSlab*(s_nx+PADDING), 1,
       out, 1, s_ny+PADDING);
# endif
}

void FT::unitToUnitStrideForwardZ(ndarray<Complex, 1> inArray,
                                  ndarray<Complex, 1> outArray) {
  fftw_complex *in = (fftw_complex*) inArray.storage_ptr();
  fftw_complex *out = (fftw_complex*) outArray.storage_ptr();

# if USE_FFTW3
  fftw_execute_dft(forwardPlan_z, in, out);
# else
  fftw(forwardPlan_z, numPencilsInYDimSlab, in, 1, s_nz+PADDING,
       out, 1, s_nz+PADDING);
# endif
}

void FT::unitToUnitStrideBackwardZ(ndarray<Complex, 1> inArray,
                                   ndarray<Complex, 1> outArray) {
  fftw_complex *in = (fftw_complex*) inArray.storage_ptr();
  fftw_complex *out = (fftw_complex*) outArray.storage_ptr();

# if USE_FFTW3
  fftw_execute_dft(backwardPlan_z, in, out);
# else
  fftw(backwardPlan_z, numPencilsInZDimSlab, in, 1, s_nx+PADDING,
       out, 1, s_nx+PADDING);
# endif
}

#else // SLABS
void FT::pencilToUnitStrideForwardX(ndarray<Complex, 2> inArray,
                                    ndarray<Complex, 2> outArray) {
  fftw_complex *in = (fftw_complex*) inArray.storage_ptr();
  fftw_complex *out = (fftw_complex*) outArray.storage_ptr();

# if USE_FFTW3
  fftw_execute_dft(forwardPlan_x, in, out);
# else
  fftw(forwardPlan_x, s_nz, in, s_nz+PADDING, 1, out, 1, s_nx+PADDING);
# endif
}

void FT::pencilToUnitStrideBackwardX(ndarray<Complex, 2> inArray,
                                     ndarray<Complex, 2> outArray) {
  fftw_complex *in = (fftw_complex*) inArray.storage_ptr();
  fftw_complex *out = (fftw_complex*) outArray.storage_ptr();

# if USE_FFTW3
  fftw_execute_dft(backwardPlan_x, in, out);
# else
  fftw(backwardPlan_x, s_nx, in, s_nx+PADDING, 1, out, 1, s_ny+PADDING);
# endif
}

void FT::unitToUnitStrideForwardZ(ndarray<Complex, 1> inArray,
                                  ndarray<Complex, 1> outArray) {
  fftw_complex *in = (fftw_complex*) inArray.storage_ptr();
  fftw_complex *out = (fftw_complex*) outArray.storage_ptr();

# if USE_FFTW3
  fftw_execute_dft(forwardPlan_z, in, out);
# else
  fftw(forwardPlan_z, 1, in, 1, s_nz+PADDING, out, 1, s_nz+PADDING);
# endif
}

void FT::unitToUnitStrideBackwardZ(ndarray<Complex, 1> inArray,
                                   ndarray<Complex, 1> outArray) {
  fftw_complex *in = (fftw_complex*) inArray.storage_ptr();
  fftw_complex *out = (fftw_complex*) outArray.storage_ptr();

# if USE_FFTW3
  fftw_execute_dft(backwardPlan_z, in, out);
# else
  fftw(backwardPlan_z, 1, in, 1, s_nx+PADDING, out, 1, s_nx+PADDING);
# endif
}
#endif // SLABS
