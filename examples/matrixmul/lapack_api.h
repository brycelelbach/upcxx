/**
 * lapack_api.hpp -- export the BLAS/LAPACK FORTRAN interface in C/C++
 * The application should link the BLAS/LAPACK library (e.g., MKL, ACML, ESSL).
 * Note: these function prototypes are not type-correct.
 * see http://www.netlib.org/lapack/ for details
 */

#pragma once

extern "C"
{
  // Dense matrix routines
  void dgemm_ ( void *, void *, void *, void *, void *, void *, void *, 
                void *, void *, void *, void *, void *, void *);
  void sgemm_ ( void *, void *, void *, void *, void *, void *, void *, 
                void *, void *, void *, void *, void *, void *);
  void zgemm_ ( void *, void *, void *, void *, void *, void *, void *, 
                void *, void *, void *, void *, void *, void *);
  void cgemm_ ( void *, void *, void *, void *, void *, void *, void *, 
                void *, void *, void *, void *, void *, void *);
  
  void dgetrf_ (void *, void *, void *, void *, void *, void *);
  void sgetrf_ (void *, void *, void *, void *, void *, void *);
  void zgetrf_ (void *, void *, void *, void *, void *, void *);
  void cgetrf_ (void *, void *, void *, void *, void *, void *);
    
  void dgetrs_ (void *, void *, void *, void *, void *, void *, void *, 
                void *, void *);
  void sgetrs_ (void *, void *, void *, void *, void *, void *, void *, 
                void *, void *);
  void zgetrs_ (void *, void *, void *, void *, void *, void *, void *, 
                void *, void *);
  void cgetrs_ (void *, void *, void *, void *, void *, void *, void *, 
                void *, void *);
    
  void dgesv_ (void *, void *, void *, void *, void *, void *, void *, 
               void *);
  void sgesv_ (void *, void *, void *, void *, void *, void *, void *, 
               void *);
  void zgesv_ (void *, void *, void *, void *, void *, void *, void *, 
               void *);
  void cgesv_ (void *, void *, void *, void *, void *, void *, void *, 
               void *);

  // Banded matrix routines
  void dgbtrf_ (void *, void *, void *, void *, void *, void *, void *, 
                void *);
  void sgbtrf_ (void *, void *, void *, void *, void *, void *, void *, 
                void *);
  void zgbtrf_ (void *, void *, void *, void *, void *, void *, void *, 
                void *);
  void cgbtrf_ (void *, void *, void *, void *, void *, void *, void *, 
                void *);

  void dgbtrs_ (void *, void *, void *, void *, void *, void *, void *, 
                void *, void *, void *, void *);
  void sgbtrs_ (void *, void *, void *, void *, void *, void *, void *, 
                void *, void *, void *, void *);
  void zgbtrs_ (void *, void *, void *, void *, void *, void *, void *, 
                void *, void *, void *, void *);
  void cgbtrs_ (void *, void *, void *, void *, void *, void *, void *, 
                void *, void *, void *, void *);
  
  void dgbsv_ (void *, void *, void *, void *, void *, void *, void *, 
               void *, void *, void *);
  void sgbsv_ (void *, void *, void *, void *, void *, void *, void *, 
               void *, void *, void *);
  void zgbsv_ (void *, void *, void *, void *, void *, void *, void *, 
               void *, void *, void *);
  void cgbsv_ (void *, void *, void *, void *, void *, void *, void *, 
               void *, void *, void *);
}

