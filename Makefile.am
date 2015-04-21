SUBDIRS = include src examples

ACLOCAL_AMFLAGS = -I m4

include_HEADERS =  upcxx_config.h

install-exec-hook:
	python $(srcdir)/scripts/set_upcxx_vars.py -f $(prefix)/include/upcxx.mak > $(prefix)/bin/upcxx_vars.sh
	python $(srcdir)/scripts/set_upcxx_vars.py -c -f $(prefix)/include/upcxx.mak > $(prefix)/bin/upcxx_vars.csh
	cp $(builddir)/scripts/upc++ $(prefix)/bin/
	chmod a+rx $(builddir)/scripts/upc++

