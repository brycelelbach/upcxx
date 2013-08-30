# Intel C++ compiler.  Preferably the same as for building GASNet
CXX = icpc 
CXXFLAGS = -fopenmp # -g -restrict # -Wall

CC = icc
CFLAGS = -fopenmp # -g -restrict # -Wall

MPI_PATH = $(shell dirname `which mpicxx`)

# Linker
export MPICH_CC = icc -fopenmp
export MPICH_CXX = icpc -fopenmp
LINK = mpicxx 
LDFLAGS = -openmp
