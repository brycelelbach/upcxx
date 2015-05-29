EXES = fft2d

.PHONY: all clean 

all: clean
	make $(EXES)

clean:
	-rm -f *.o $(EXES)

CC ?= cc
CXX ?= mpicxx

UPCXX_INSTALL_DIR ?= /tmp/upcxx_install
include $(UPCXX_INSTALL_DIR)/include/upcxx.mak

VPATH=$(UPCXX_INSTALL_DIR)/include

## module load fftw-Intel/3.2.2 ## for Carver @ NERSC 
## FFTW_INC and FFTW_LIBDIR are defined by the module
## You would get linking errors if fftw3 is not available.

FFTW_INC ?= $(HOME)/install/fftw-3.3.3/include
FFTW_LIBDIR ?= $(HOME)/install/fftw-3.3.3/lib

## The following MKL env variables are set by "module load" or manually.
MKLROOT = /opt/intel/mkl
MKL_LIBDIR = $(MKLROOT)/intel64
MKL_INC = $(MKLROOT)/include
MKL = -L$(MKL_LIBDIR) -lmkl_intel_lp64 -lmkl_intel_thread -lmkl_core -openmp -lpthread -lm

myfft_fftw3.o: myfft_fftw3.c myfft.h 
	$(CC) -DFFTW_MT -I $(FFTW_INC) -I. -c myfft_fftw3.c
#	$(CC) -mkl -I. -c myfft_fftw3.c

%.o: %.cpp 
	$(CXX) $(UPCXX_CXXFLAGS) -o $@ -c $<

fft2d: fft2d.o  myfft_fftw3.o 
	$(CXX) $(LDFLAGS) $(UPCXX_LDFLAGS) -o $@ $^ $(UPCXX_LDLIBS)  -L $(FFTW_LIBDIR) -lfftw3_threads -lfftw3
  ## Use MKL
  ## $(CXX) $(LDFLAGS) $(UPCXX_LDFLAGS) -o $@ $^ $(UPCXX_LDLIBS)  -L $(FFTW_LIBDIR) $(MKL)
