include ../make_config/Make.gasnet.intel

EXES = axpy

all: 
	make $(DEGAS_LIBS)
	make $(EXES)

clean:
	-rm -f *.o $(EXES)

$(DEGAS_LIBS): $(DEGAS_HEADERS) Makefile

axpy: axpy.o  $(DEGAS_LIBS)
	$(LINK) $(LDFLAGS) -o $@ $^ $(LIBS) $(OMPLIBS)
