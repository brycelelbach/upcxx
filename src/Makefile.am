include /Users/mbdriscoll/opt/gasnet-1.20.2/include/smp-conduit/smp-seq.mak

lib_LTLIBRARIES = libupcxx.la

libupcxx_la_CPPFLAGS = \
  -I$(top_srcdir)/include \
  $(GASNET_CPPFLAGS) \
  $(GASNET_CFLAGS)

libupcxx_la_SOURCES = \
  dl_malloc.c        \
  active_coll.cpp    \
  async.cpp          \
  barrier.cpp        \
  event.cpp          \
  lock.cpp           \
  machine.cpp        \
  ptr_to_shared.cpp  \
  team.cpp           \
  upcxx_runtime.cpp
