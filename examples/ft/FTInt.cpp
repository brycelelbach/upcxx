#include "FT.h"
#include <fftw.h>

static int nx, ny, nz;
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

  nx = p_nx;
  ny = p_ny;
  nz = p_nz;
  numPencilsInYDimSlab = p_numPencilsInYDimSlab;
  numPencilsInZDimSlab = p_numPencilsInZDimSlab;

  forwardPlan_y = fftw_create_plan_specific(ny, FFTW_FORWARD,
                                            FFTW_MEASURE | FFTW_IN_PLACE,
                                            array1, nz+PADDING,
                                            array1, nz+PADDING);
  backwardPlan_y = fftw_create_plan_specific(nz, FFTW_BACKWARD,
                                             FFTW_MEASURE,
                                             array3, nx+PADDING,
                                             array4, nx+PADDING);
#ifdef SLABS
  forwardPlan_x = fftw_create_plan_specific(nx, FFTW_FORWARD, FFTW_MEASURE,
                                            array2,
                                            numPencilsInYDimSlab*(nz+PADDING),
                                            array3, 1);
  backwardPlan_x = fftw_create_plan_specific(ny, FFTW_BACKWARD,
                                             FFTW_MEASURE, array5,
                                             numPencilsInZDimSlab*(nx+PADDING),
                                             array6, 1);
  forwardPlan_z = fftw_create_plan_specific(nz, FFTW_FORWARD,
                                            FFTW_MEASURE | FFTW_IN_PLACE,
                                            array1, 1, array1, 1);
  backwardPlan_z = fftw_create_plan_specific(nx, FFTW_BACKWARD,
                                             FFTW_MEASURE | FFTW_IN_PLACE,
                                             array4, 1, array4, 1);
#else
  forwardPlan_x = fftw_create_plan_specific(nx, FFTW_FORWARD, FFTW_MEASURE,
                                            array2, nz+PADDING, array3, 1);
  backwardPlan_x = fftw_create_plan_specific(ny, FFTW_BACKWARD, FFTW_MEASURE,
                                             array5, nx+PADDING, array6, 1);
  forwardPlan_z = fftw_create_plan_specific(nz, FFTW_FORWARD,
                                            FFTW_MEASURE | FFTW_IN_PLACE,
                                            array1, 1, array1, 1);
  backwardPlan_z = fftw_create_plan_specific(nx, FFTW_BACKWARD,
                                             FFTW_MEASURE | FFTW_IN_PLACE,
                                             array4, 1, array4, 1);
#endif
}


void FT::pencilToPencilStrideForwardY(ndarray<Complex, 2> inArray,
                                      ndarray<Complex, 1> outArray) {
  fftw_complex *in = (fftw_complex*) inArray.storage_ptr();
  fftw_complex *out = (fftw_complex*) outArray.storage_ptr();

  fftw(forwardPlan_y, nz, in, nz+PADDING, 1, out, nz+PADDING, 1);
}

void FT::pencilToPencilStrideBackwardY(ndarray<Complex, 2> inArray,
                                       ndarray<Complex, 2> outArray) {
  fftw_complex *in = (fftw_complex*) inArray.storage_ptr();
  fftw_complex *out = (fftw_complex*) outArray.storage_ptr();

  fftw(backwardPlan_y, nx, in, nx+PADDING, 1, out, nx+PADDING, 1);
}

#ifdef SLABS
void FT::slabToUnitStrideForwardX(ndarray<Complex, 2> inArray,
                                  ndarray<Complex, 2> outArray) {
  fftw_complex *in = (fftw_complex*) inArray.storage_ptr();
  fftw_complex *out = (fftw_complex*) outArray.storage_ptr();

  fftw(forwardPlan_x, nz, in, numPencilsInYDimSlab*(nz+PADDING), 1,
       out, 1, nx+PADDING);
}

void FT::slabToUnitStrideBackwardX(ndarray<Complex, 2> inArray,
                                   ndarray<Complex, 2> outArray) {
  fftw_complex *in = (fftw_complex*) inArray.storage_ptr();
  fftw_complex *out = (fftw_complex*) outArray.storage_ptr();

  fftw(backwardPlan_x, nx, in, numPencilsInZDimSlab*(nx+PADDING), 1,
       out, 1, ny+PADDING);
}

void FT::unitToUnitStrideForwardZ(ndarray<Complex, 1> inArray,
                                  ndarray<Complex, 1> outArray) {
  fftw_complex *in = (fftw_complex*) inArray.storage_ptr();
  fftw_complex *out = (fftw_complex*) outArray.storage_ptr();

  fftw(forwardPlan_z, numPencilsInYDimSlab, in, 1, nz+PADDING,
       out, 1, nz+PADDING);
}

void FT::unitToUnitStrideBackwardZ(ndarray<Complex, 1> inArray,
                                   ndarray<Complex, 1> outArray) {
  fftw_complex *in = (fftw_complex*) inArray.storage_ptr();
  fftw_complex *out = (fftw_complex*) outArray.storage_ptr();

  fftw(backwardPlan_z, numPencilsInZDimSlab, in, 1, nx+PADDING,
       out, 1, nx+PADDING);
}

#else
void FT::pencilToUnitStrideForwardX(ndarray<Complex, 2> inArray,
                                    ndarray<Complex, 2> outArray) {
  fftw_complex *in = (fftw_complex*) inArray.storage_ptr();
  fftw_complex *out = (fftw_complex*) outArray.storage_ptr();

  fftw(forwardPlan_x, nz, in, nz+PADDING, 1, out, 1, nx+PADDING);
}

void FT::pencilToUnitStrideBackwardX(ndarray<Complex, 2> inArray,
                                     ndarray<Complex, 2> outArray) {
  fftw_complex *in = (fftw_complex*) inArray.storage_ptr();
  fftw_complex *out = (fftw_complex*) outArray.storage_ptr();

  fftw(backwardPlan_x, nx, in, nx+PADDING, 1, out, 1, ny+PADDING);
}

void FT::unitToUnitStrideForwardZ(ndarray<Complex, 1> inArray,
                                  ndarray<Complex, 1> outArray) {
  fftw_complex *in = (fftw_complex*) inArray.storage_ptr();
  fftw_complex *out = (fftw_complex*) outArray.storage_ptr();

  fftw(forwardPlan_z, 1, in, 1, nz+PADDING, out, 1, nz+PADDING);
}

void FT::unitToUnitStrideBackwardZ(ndarray<Complex, 1> inArray,
                                   ndarray<Complex, 1> outArray) {
  fftw_complex *in = (fftw_complex*) inArray.storage_ptr();
  fftw_complex *out = (fftw_complex*) outArray.storage_ptr();

  fftw(backwardPlan_z, 1, in, 1, nx+PADDING, out, 1, nx+PADDING);
}
#endif
