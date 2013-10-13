EXES = sample_sort

.PHONY: all clean 

all: clean
	make $(EXES)

clean:
	-rm -f *.o $(EXES)

TOPDIR=../..
include $(TOPDIR)/upcxx.mak

LIBS += -L $(BUILDDIR)/ -lupcxx 

CXXFLAGS += -O3 

sfmt.o: ../SFMT/SFMT.c
	$(CC) $(CFLAGS) -I ../SMFT -finline-functions -fomit-frame-pointer -DNDEBUG -fno-strict-aliasing -Wmissing-prototypes -Wall -std=c99 -DSFMT_MEXP=19937 -c -o sfmt.o ../SFMT/SFMT.c

sample_sort: sample_sort.o sfmt.o $(UPCXX_LIBS) 
	$(LINK) $(LDFLAGS) -o $@ $^  $(LIBS)


