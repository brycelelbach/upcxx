include $(GASNET_MAKEFILE)

bin_PROGRAMS = \
  stencil

stencil_SOURCES = stencil.cpp

AM_CPPFLAGS = \
  -I$(top_srcdir)/include \
  $(GASNET_CPPFLAGS) 

AM_LDFLAGS = \
  $(GASNET_LDFLAGS) -Wl,--whole-archive,$(top_builddir)/src/.libs/libupcxx.a,--no-whole-archive

LDADD = $(GASNET_LIBS)
