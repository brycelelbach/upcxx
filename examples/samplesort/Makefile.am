include $(GASNET_MAKEFILE)

bin_PROGRAMS = \
  sample_sort

sample_sort_SOURCES = sample_sort.cpp 

AM_CPPFLAGS = \
  -I$(top_srcdir)/include \
  $(GASNET_CPPFLAGS) $(GASNET_CFLAGS)

AM_CFLAGS = \
  -I$(top_srcdir)/include \
  $(GASNET_CPPFLAGS) $(GASNET_CFLAGS) 

AM_LDFLAGS = \
  $(top_builddir)/src/.libs/libupcxx.a \
  $(GASNET_LDFLAGS) $(GASNET_LIBS)
