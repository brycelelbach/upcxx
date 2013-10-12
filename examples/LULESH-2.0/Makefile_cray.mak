#default build suggestion of MPI + OPENMP with gcc on Livermore machines you might have to change the compiler name

SHELL = /bin/sh
.SUFFIXES: .cc .o

#LULESH_EXEC = lulesh2.0 # lulesh2.0-upcxx
LULESH_EXEC = lulesh2.0-upcxx
all: $(LULESH_EXEC)

TOPDIR=../..
include $(TOPDIR)/upcxx.mak

MPI_INC = /opt/local/include/openmpi
MPI_LIB = /opt/local/lib

SERCXX = CC 
MPICXX = CC
#CXX = $(MPICXX) -DUSE_MPI=1 -DUSE_UPCXX=0 -DUSE_OMP=1
CXX = $(MPICXX) -fast -DUSE_OMP=1 -fopenmp

SOURCES2.0MPI = \
	lulesh.cc \
	lulesh-comm.cc \
	lulesh-viz.cc \
	lulesh-util.cc \
	lulesh-init.cc 
OBJECTS2.0MPI = $(SOURCES2.0MPI:.cc=.mpi.o)

SOURCES2.0UPCXX = \
	lulesh.cc \
	lulesh-comm-upcxx.cc \
	lulesh-viz.cc \
	lulesh-util.cc \
	lulesh-init.cc 
OBJECTS2.0UPCXX = $(SOURCES2.0UPCXX:.cc=.upcxx.o)

## -wd161 disable the openmp pragma warnings by Intel compilers
#OMP_FLAG = -wd161 #-fopenmp
#CXXFLAGS = -O3 $(OMP_FLAG)
#LDFLAGS = $(OMP_FLAG)

#common places you might find silo on the Livermore machines.
#SILO_INCDIR = /opt/local/include
#SILO_LIBDIR = /opt/local/lib
#SILO_INCDIR = /usr/gapps/silo/current/chaos_5_x86_64/include
#SILO_LIBDIR = /usr/gapps/silo/current/chaos_5_x86_64/lib

#below is and example of how to make with silo, hdf5 to get vizulization by default all this is turned off
#CXXFLAGS = -g -openmp -DVIZ_MESH -I${SILO_INCDIR} -Wall -Wno-pragmas
#LDFLAGS = -g -L${SILO_LIBDIR} -Wl,-rpath -Wl,${SILO_LIBDIR} -lsiloh5 -lhdf5

%.mpi.o: %.cc lulesh.h
	@echo "Building $<"
	$(CXX) -c $(CXXFLAGS) -DUSE_MPI=1 -DUSE_UPCXX=0 -o $@ $<

%.upcxx.o: %.cc lulesh.h
	@echo "Building $<"
	$(CXX) -c $(CXXFLAGS) -DUSE_MPI=0 -DUSE_UPCXX=1 -o $@ $<

lulesh2.0-mpi: $(OBJECTS2.0MPI) 
	@echo "Linking"
	$(CXX) $(LDFLAGS) $(OBJECTS2.0MPI) -lm -o $@

LIBS+=-L $(BUILDDIR)/ -lupcxx
lulesh2.0-upcxx: $(OBJECTS2.0UPCXX) $(UPCXX_LIB)
	@echo "Linking"
	$(CXX) $(OBJECTS2.0UPCXX) $(LDFLAGS) -lm $(LIBS) -o $@

clean:
	/bin/rm -f *.o *~ $(OBJECTS) $(LULESH_EXEC)
	/bin/rm -rf *.dSYM

tar: clean
	cd .. ; tar cvf lulesh-2.0.tar LULESH-2.0 ; mv lulesh-2.0.tar LULESH-2.0

