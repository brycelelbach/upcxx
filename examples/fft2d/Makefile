EXES = fft2d

.PHONY: all clean 

all: clean
	make $(EXES)

clean:
	-rm -f *.o $(EXES)

TOPDIR=../..
include $(TOPDIR)/upcxx.mak

LIBS += -L $(BUILDDIR)/ -lupcxx 

## module load fftw-Intel/3.2.2 ## for Carver @ NERSC 
## FFTW_INC and FFTW_LIBDIR are defined by the module
## You would get linking errors if fftw3 is not available.

FFTW_INC ?= /usr/include
FFTW_LIBDIR ?= /usr/lib64

## The following MKL env variables are set by "module load" or manually.
MKLROOT = /opt/intel/mkl
MKL_LIBDIR = $(MKLROOT)/intel64
MKL_INC = $(MKLROOT)/include
MKL = -L$(MKL_LIBDIR) -lmkl_intel_lp64 -lmkl_intel_thread -lmkl_core -openmp -lpthread -lm


## Define make rules for the executables
# define makerule4exe
# $1: $1.o $(UPCXX_LIB)
# 	$(LINK) $(LDFLAGS) -o $$@ $$< $(LIBS)
# endef

# $(foreach e, $(EXES), $(eval $(call makerule4exe, $e)))

myfft_fftw3.o: myfft_fftw3.c myfft.h 
	$(CC) -DFFTW_MT -I $(FFTW_INC) -I. -c myfft_fftw3.c
#	$(CC) -mkl -I. -c myfft_fftw3.c

fft2d: fft2d.o  myfft_fftw3.o $(UPCXX_LIBS) 
  #module load fftw-Intel/3.2.2  
	$(LINK) $(LDFLAGS) -o $@ $^ $(LIBS) -L $(FFTW_LIBDIR) -lfftw3_threads -lfftw3
#	$(LINK) $(LDFLAGS) -o $@ $^ $(LIBS) -L $(FFTW_LIBDIR) $(MKL)

test_transpose: test_transpose.o $(UPCXX_LIB) 
	$(LINK) $(LDFLAGS) -o $@ $^ $(LIBS) 


