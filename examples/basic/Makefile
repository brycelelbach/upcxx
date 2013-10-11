## Makefile for building examples

EXES = hello test_shared_var test_ptr_to_shared testperf2 test_async \
	test_event test_am_bcast test_shared_array test_reduce test_remote_inc
	 # test_team
  #test_domain

#CXX11_EXES = test_async2 

all: 
	make $(EXES) $(CXX11_EXES)

clean:
	-rm -f *.o $(EXES) $(CXX11_EXES)

TOPDIR=../..
include $(TOPDIR)/upcxx.mak

LIBS += -L $(BUILDDIR)/ -lupcxx

## Define make rules for the executables

define makerule4exe
$1: $1.o $(DEGAS_LIB)
	$(LINK) $(LDFLAGS) -o $$@ $$< $(LIBS)
endef

CXX11FLAGS = $(CXXFLAGS) -std=c++11

define makerule4cxx11exe
$1.o: $1.cpp $(DEGAS_LIB)
	$(CXX) $(CXX11FLAGS) -o $$@ -c $$< 

$1: $1.o $(DEGAS_LIB)
	$(LINK) $(LDFLAGS) -o $$@ $$< $(LIBS)
endef

$(foreach e, $(EXES), $(eval $(call makerule4exe, $e)))

$(foreach e, $(CXX11_EXES), $(eval $(call makerule4cxx11exe, $e)))
