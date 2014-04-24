include $(GASNET_MAKEFILE)

bin_PROGRAMS = \
  gsrb

gsrb_SOURCES = amr_level.cpp amr_test.cpp execution_log.cpp

AM_CPPFLAGS = \
  -I$(top_srcdir)/include \
  $(GASNET_CPPFLAGS) $(GASNET_CFLAGS)

AM_LDFLAGS = \
  $(top_builddir)/src/.libs/libupcxx.a \
  $(GASNET_LDFLAGS) $(GASNET_LIBS)
