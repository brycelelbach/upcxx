include ../$(GASNET_MAKEFILE)

lib_LTLIBRARIES = libupcxx.la

libupcxx_la_CPPFLAGS = \
  -I$(top_srcdir)/include \
  $(GASNET_CPPFLAGS)  -DUSE_GASNET_FAST_SEGMENT -DONLY_MSPACES 

if UPCXX_MD_ARRAY
UPCXX_MD_ARRAY_CPP_FILES = \
  array_bulk.cpp    
else
UPCXX_MD_ARRAY_CPP_FILES =   
endif

if UPCXX_USE_DMAPP
UPCXX_DMAPP_CPP_FILES = \
  dmapp_channel/dmapp_helper.cpp    
else
UPCXX_DMAPP_CPP_FILES =   
endif

libupcxx_la_SOURCES = \
  dl_malloc.c        \
  active_coll.cpp    \
  active_message.cpp \
  allocate.cpp       \
  async.cpp          \
  async_copy.cpp     \
  barrier.cpp        \
  collective.cpp     \
  event.cpp          \
  progress_thread.cpp\
  lock.cpp           \
  team.cpp           \
  upcxx_runtime.cpp $(UPCXX_DMAPP_CPP_FILES) $(UPCXX_MD_ARRAY_CPP_FILES)
