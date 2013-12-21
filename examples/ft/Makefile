EXES = ft

.PHONY: all clean 

all: clean
	make $(EXES)

clean:
	-rm -f *.o $(EXES)


UPCXX_INSTALL_DIR ?= /tmp/upcxx_install
include $(UPCXX_INSTALL_DIR)/include/upcxx.mak

VPATH=$(UPCXX_INSTALL_DIR)/include

## module load fftw-Intel/3.2.2 ## for Carver @ NERSC 
## FFTW_INC and FFTW_LIBDIR are defined by the module
## You would get linking errors if fftw3 is not available.

FFTW_DIR ?= $(HOME)/install/fftw-3.3.3
FFTW_INC ?= $(FFTW_DIR)/include
FFTW_LIBDIR ?= $(FFTW_DIR)/lib

FTInt.o: FTInt.cpp
	$(CXX) $(UPCXX_CXXFLAGS) -I $(FFTW_INC) -I. -o FTInt.o -c FTInt.cpp

%.o: %.cpp 
	$(CXX) $(UPCXX_CXXFLAGS) -o $@ -c $<

ft: FT.o FTDriver.o FTInt.o Random.o Test.o 
	$(CXX) $(LDFLAGS) $(UPCXX_LDFLAGS) -o $@ $^ $(UPCXX_LDLIBS)  -L $(FFTW_LIBDIR) -lfftw3_threads -lfftw3
