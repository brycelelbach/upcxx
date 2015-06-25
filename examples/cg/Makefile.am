include ../../$(GASNET_MAKEFILE)

bin_PROGRAMS = \
  cg

cg_SOURCES = CGDriver.cpp Random.cpp Vector.cpp LocalSparseMat.cpp \
  SparseMat.cpp MatGen.cpp Util.cpp

AM_CPPFLAGS = \
  -I$(top_srcdir)/include \
  $(GASNET_CPPFLAGS) 

AM_LDFLAGS = \
  $(GASNET_LDFLAGS) 

LDADD = $(top_builddir)/src/.libs/libupcxx.a $(GASNET_LIBS)
