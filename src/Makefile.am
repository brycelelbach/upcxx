include $(GASNET_MAKEFILE)

lib_LTLIBRARIES = libupcxx.la

libupcxx_la_CPPFLAGS = \
  -I$(top_srcdir)/include \
  $(GASNET_CPPFLAGS) 

if UPCXX_MD_ARRAY
UPCXX_MD_ARRAY_CPP_FILES = \
  array_bulk.cpp    
else
UPCXX_MD_ARRAY_CPP_FILES =   
endif

libupcxx_la_SOURCES = \
  dl_malloc.c        \
  active_coll.cpp    \
  allocate.cpp       \
  async.cpp          \
  async_copy.cpp     \
  barrier.cpp        \
  collective.cpp     \
  event.cpp          \
  lock.cpp           \
  team.cpp           \
  upcxx_runtime.cpp $(UPCXX_MD_ARRAY_CPP_FILES)
