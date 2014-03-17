include $(GASNET_MAKEFILE)

bin_PROGRAMS = \
  sample_sort

##sfmt.o: SFMT/SFMT.c
##	$(CC) $(CFLAGS) -I ../SMFT -finline-functions -fomit-frame-pointer -DNDEBUG -fno-strict-aliasing -Wmissing-prototypes -Wall -std=c99 -DSFMT_MEXP=19937 -c -o sfmt.o SFMT/SFMT.c

sample_sort_SOURCES = sample_sort.cpp SFMT/SFMT.c 

AM_CPPFLAGS = \
  -I$(top_srcdir)/include \
  $(GASNET_CPPFLAGS) $(GASNET_CFLAGS)

AM_CFLAGS = \
  -I$(top_srcdir)/include \
  $(GASNET_CPPFLAGS) $(GASNET_CFLAGS) \
  -I SMFT -finline-functions -fomit-frame-pointer -DNDEBUG -fno-strict-aliasing -Wmissing-prototypes -Wall -std=c99 -DSFMT_MEXP=19937

AM_LDFLAGS = \
  $(top_builddir)/src/.libs/libupcxx.a \
  $(GASNET_LDFLAGS) $(GASNET_LIBS)
