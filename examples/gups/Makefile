EXES = gups

all: 
	make $(EXES)

clean:
	-rm -f *.o $(EXES)

TOPDIR=../..
include $(TOPDIR)/upcxx.mak

LIBS += -L $(BUILDDIR)/ -lupcxx

## Define make rules for the executables
define makerule4exe
$1: $1.o $(DEGAS_LIB)
	$(LINK) $(LDFLAGS) -o $$@ $$< $(LIBS)
endef

$(foreach e, $(EXES), $(eval $(call makerule4exe, $e)))

