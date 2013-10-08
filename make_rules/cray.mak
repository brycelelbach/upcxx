# Use Cray Compiler wrappers
CXX = CC
CXXFLAGS = -DGASNETT_USE_GCC_ATTRIBUTE_MAYALIAS  ## for building gasnet with gcc

CC = cc
CFLAGS = 

# Linker
LINK = CC
LDFLAGS =
