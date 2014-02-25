include $(GASNET_MAKEFILE)

lib_LTLIBRARIES = libupcxx.la

libupcxx_la_CPPFLAGS = \
  -I$(top_srcdir)/include \
  $(GASNET_CPPFLAGS) \
  $(GASNET_CFLAGS)

libupcxx_la_SOURCES = \
  array_bulk.c       \
  dl_malloc.c        \
  active_coll.cpp    \
  async.cpp          \
  barrier.cpp        \
  collective.cpp     \
  event.cpp          \
  lock.cpp           \
  machine.cpp        \
  global_ptr.cpp     \
  team.cpp           \
  upcxx_runtime.cpp
