# Intel C++ compiler.  Preferably the same as for building GASNet
CXX = icpc -mmic -openmp
#CXXFLAGS = -g -restrict # -Wall

CC = icc -mmic -openmp
#CFLAGS =  -g -restrict # -Wall

MPICXX = mpiicpc -mmic

# Linker
LINK = $(MPICXX) -openmp
LDFLAGS = 
