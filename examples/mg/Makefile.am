include ../../$(GASNET_MAKEFILE)

noinst_PROGRAMS = \
  mg

mg_SOURCES = MGDriver.cpp MG.cpp Grid.cpp Test.cpp

AM_CPPFLAGS = \
  -I$(top_srcdir)/include \
  $(GASNET_CPPFLAGS) 

AM_LDFLAGS = \
  $(GASNET_LDFLAGS) 

LDADD = $(top_builddir)/src/.libs/libupcxx.a $(GASNET_LIBS)
