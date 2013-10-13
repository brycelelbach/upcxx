# Clang compilers.  Preferably the same as for building GASNet
CXX = clang++
CXXFLAGS = -O3 # -g #-std=c++11 -stdlib=libc++ -Wreserved-user-defined-literal # -restrict -Wall

CC = clang
CFLAGS = -O3 -g # -restrict -Wall

MPI_PATH = /Users/yzheng/work/mpich2-1.5_install/bin

# Linker
LINK = $(MPI_PATH)/mpicxx 
#LDFLAGS = -std=c++11 -stdlib=libc++
#LFLAGS = -fopenmp

FFTW_INC = /opt/local/include
FFTW_LIBDIR = /opt/local/lib