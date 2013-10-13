EXES = fft2d

.PHONY: all clean 

all: clean
	make $(EXES)

clean:
	-rm -f *.o $(EXES)

TOPDIR=../..
include $(TOPDIR)/upcxx.mak

LIBS += -L $(BUILDDIR)/ -lupcxx 

EXES = matmul # matmul_hybrid

## module load mkl  

## The following MKL env variables are set by "module load" or manually.
#MKL_LIBDIR=/usr/common/usg/mkl/10.2.2.025/lib/em64t
#MKL_HOME=/usr/common/usg/mkl/10.2.2.025
#MKL_INC=/usr/common/usg/mkl/10.2.2.025/include
#MKL=-L/usr/common/usg/mkl/10.2.2.025/lib/em64t -lmkl_intel_lp64 -lmkl_sequential -lmkl_core -liomp5

MKL_INC ?= $(MKLROOT)/include
MKL ?= -L$(MKLROOT)/lib/em64t -lmkl_intel_lp64 -lmkl_sequential -lmkl_core # -liomp5
#MKL = -L$(MKL_LIBDIR) -lmkl_intel_lp64 -lmkl_intel_thread -lmkl_core -openmp -lpthread -lm

ESSL = -L/soft/apps/ESSL-4.4.1-1/lib -lesslsmpbg -L/soft/apps/ibmcmp/xlmass/bg/4.4/lib -lmass  -L/soft/apps/ibmcmp/xlf/bg/11.1/bglib -lxlf90_r -lxlopt -lxl -lxlfmath -L/soft/apps/ibmcmp/xlsmp/bg/1.7/bglib -lxlsmp -lpthread

#BLAS = $(ESSL)
#BLAS = $(MKL)

LDFLAGS +=  -framework Accelerate 

CUBLAS = -lcublas

#CXXFLAGS += -I$(MKL_INC)

matmul.o: matmul.cpp matrix.h verify.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

## need to link with Intel MKL or other BLAS/LAPACK library for
## functions in lapack_api.h
matmul: matmul.o $(UPCXX_LIBS) 
	$(LINK) $(LDFLAGS) -o $@ $^ $(LIBS) $(BLAS)
