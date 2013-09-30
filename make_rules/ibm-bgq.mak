## GNU Makefile for building UPC++ on IBM BG/Q with XLC
## 

XLC_FLAGS = -O2 -qsmp=omp:noopt:noauto

# C++ compiler.  Preferably the same as the one used for GASNet
CXX = bgxlC_r $(XLC_FLAGS)
CXXFLAGS = $(INCLUDE) $(GASNET_CPPFLAGS) -I $(ROOT) -restrict # -Wall

# C compiler.  Preferably the same as the one used for GASNet
CC = $(GASNET_CC) 
CFLAGS = $(INCLUDE) $(GASNET_CFLAGS) 

# Linker
LINK = $(CXX) 
LFLAGS = 


