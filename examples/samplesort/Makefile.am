include $(GASNET_MAKEFILE)

bin_PROGRAMS = \
  sample_sort

sample_sort_SOURCES = sample_sort.cpp 

AM_CPPFLAGS = \
  -I$(top_srcdir)/include \
  $(GASNET_CPPFLAGS) 

AM_LDFLAGS = \
  $(GASNET_LDFLAGS) -Wl,--whole-archive,$(top_builddir)/src/.libs/libupcxx.a,--no-whole-archive

LDADD = $(GASNET_LIBS)
