include $(GASNET_MAKEFILE)

bin_PROGRAMS = \
  pps id_test

pps_SOURCES = execution_log.cpp nodal_infdmn.cpp pmg.cpp \
  my_math.cpp nodal_multigrid.cpp pmg_test.cpp

id_test_SOURCES = execution_log.cpp id_test.cc my_math.cpp \
  nodal_infdmn.cpp nodal_multigrid.cpp

AM_CPPFLAGS = \
  -I$(top_srcdir)/include \
  $(GASNET_CPPFLAGS) $(GASNET_CFLAGS)

AM_LDFLAGS = \
  $(top_builddir)/src/.libs/libupcxx.a \
  $(GASNET_LDFLAGS) $(GASNET_LIBS)
