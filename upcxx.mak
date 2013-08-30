ROOT=$(HOME)/Dropbox/degas/upcxx
#GASNET_PATH=$(HOME)/install/bupc_mic/opt
#GASNET_PATH=$(HOME)/install/gasnet_babbage
#GASNET_PATH=$(HOME)/gasnet_install
GASNET_PATH=$(HOME)/work/upc_runtime_install/opt
#include $(ROOT)/make_rules/icc-mic.mak
#include $(ROOT)/make_rules/icc-linux.mak
include $(ROOT)/make_rules/clang-macos.mak

BUILDDIR=$(ROOT)/build
VPATH = $(ROOT)/include:$(ROOT)/src:$(BUILDDIR)

## GASNet
GASNET_CONDUIT = mpi
GASNET_PAR_MODE = seq
GASNET_CONDUIT_MAKEFILE = $(GASNET_CONDUIT)-$(GASNET_PAR_MODE).mak
include $(GASNET_PATH)/include/$(GASNET_CONDUIT)-conduit/$(GASNET_CONDUIT_MAKEFILE)

## check if CC == $(GASNET_CC)?

## Update compiler and linker flags
UPCXX_FLAGS = -DUSE_GASNET_FAST_SEGMENT -DONLY_MSPACES 
INCLUDE = -I. -I$(ROOT)/include
CFLAGS += $(INCLUDE) $(GASNET_CFLAGS) $(UPCXX_FLAGS) # -Wall
CXXFLAGS += $(INCLUDE) $(GASNET_CPPFLAGS) -I $(ROOT) $(UPCXX_FLAGS)
LDFLAGS += $(GASNET_LDFLAGS)
LIBS += $(GASNET_LIBS)

# Rules of compiling different source file types
%.o : %.c 
	$(CC) $(CFLAGS) -o $@ -c $<

%.o : %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

%.o : %.cc
	$(CXX) $(CXXFLAGS) -o $@ -c $<

UPCXX_HEADERS = upcxx.h ptr_to_shared.h queue.h lock.h async.h event.h \
	shared_var.h gasnet_api.h forkjoin.h async_templates.h machine.h \
	active_coll.h shared_array.h group.h team.h collective.h  \
  allocate.h

UPCXX_CXX_SRC = lock.cpp machine.cpp async.cpp active_coll.cpp \
  upcxx_runtime.cpp event.cpp barrier.cpp ptr_to_shared.cpp team.cpp

UPCXX_C_SRC = dl_malloc.c

UPCXX_SRC = $(UPCXX_CXX_SRC) $(UPCXX_C_SRC)

UPCXX_OBJS = $(UPCXX_CXX_SRC:.cpp=.o) $(UPCXX_C_SRC:.c=.o) 

UPCXX_LIB = libupcxx.a

$(UPCXX_LIB): $(UPCXX_HEADERS) $(UPCXX_SRC)
	@mkdir -p $(BUILDDIR)
	@rm -f $(BUILDDIR)/$(UPCXX_LIB)
	cd $(BUILDDIR);	\
  make -f $(ROOT)/upcxx.mak $(UPCXX_OBJS); \
	ar rcs $@ $(UPCXX_OBJS);

