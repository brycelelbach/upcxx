# GCC compilers.  Preferably the same as for building GASNet
CXX = g++
CXXFLAGS = -fopenmp -g # -restrict -Wall

CC = gcc
CFLAGS = -fopenmp -g # -restrict -Wall

# Linker
LINK = mpicxx 
LDFLAGS = -fopenmp
