include ./upcxx.mak

SUBDIRS = examples/basic \
	        examples/gups \
	        examples/LULESH-2.0 \
	        examples/fft2d 

.PHONY: all lib doc clean distclean

all: lib
	@for subdir in $(SUBDIRS) ; do \
		cd $(ROOT)/$$subdir; \
		make; \
		if [ $$? != 0 ]; then exit 1; fi; \
	done;

lib: $(UPCXX_LIB)

doc:
	doxygen doxygen_cfg

clean:
	-rm -f $(UPCXX_LIB) $(BUILDDIR)/*
	@for subdir in $(SUBDIRS) ; do \
		cd $(ROOT)/$$subdir; \
		make clean; \
		if [ $$? != 0 ]; then exit 1; fi; \
	done;

distclean: clean
	-rm -f *~ */*~ */*/*~
