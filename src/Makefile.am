include @GASNET_CONDUIT_INCLUDE_FILE@
inlcude ../common.mk

CONDUIT_LIBS = @GASNET_CONDUIT_LIBS@ $(CONDUIT_LIBS)

hpgas_SOURCES = lock.cpp \
                machine.cpp \
                async.cpp \
                active_coll.cpp \
                degas.cpp \
                domain.cpp

DEGAS_HEADERS = degas.h global_ptr.h queue.h lock.h async.h event.h \
global_var.h gasnet_api.h startup.h async_template.h machine.h \
active_coll.h global_array.h



# ROOT ?= $(HOME)/Dropbox/degas/degas-ps/hpgas

# # search path
# VPATH = $(ROOT)/include:$(ROOT)/src:.

# # GASNet
# #GASNET_PATH ?= $(HOME)/gasnet_gpu_install
# GASNET_PATH ?= $(HOME)/gasnet_install
# # select a network conduit for your system
# # smp, mpi, udp, ibv
# GASNET_CONDUIT = mpi
# # select a parallel mode for gasnet: seq - not thread safe but fastest; parsync - thread safe (via locks and atomics) but thread agnostic; par - thread aware
# GASNET_PAR_MODE = seq
# GASNET_CONDUIT_MAKEFILE = $(GASNET_CONDUIT)-$(GASNET_PAR_MODE).mak

# include $(GASNET_PATH)/include/$(GASNET_CONDUIT)-conduit/$(GASNET_CONDUIT_MAKEFILE)

# INCLUDE = -I. -I$(ROOT)/include

# # Rules of compiling different source file types
# %.o : %.c 
# 	$(CC) $(CFLAGS) -o $@ -c $<

# %.o : %.cpp
# 	$(CXX) $(CXXFLAGS) -o $@ -c $<

# %.o : %.cxx
# 	$(CXX) $(CXXFLAGS) -o $@ -c $<

# %.o : %.cu 
# 	$(NVCC) $(NVCCFLAGS) -o $@ -c $<

# %.cubin : %.cu 
# 	$(NVCC) $(NVCCFLAGS) -o $@ -cubin $<

# %.ptx : %.cu
# 	$(NVCC)  $(NVCCFLAGS) -o $@ -ptx $<
