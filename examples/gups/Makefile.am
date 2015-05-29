include $(GASNET_MAKEFILE)

bin_PROGRAMS = \
  gups

gups_SOURCES = gups.cpp

AM_CPPFLAGS = \
  -I$(top_srcdir)/include \
  $(GASNET_CPPFLAGS) 

AM_LDFLAGS = \
  $(top_builddir)/src/.libs/libupcxx.a \
  $(GASNET_LDFLAGS) $(GASNET_LIBS)
