include $(GASNET_MAKEFILE)

bin_PROGRAMS = \
  hello \
  test_am_bcast \
  test_array \
  test_array_dist \
  test_async \
  test_dynamic_finish \
  test_event \
  test_finish \
  test_global_ptr \
  test_reduce \
  test_remote_inc \
  test_safe_broadcast \
  test_shared_array \
  test_shared_var \
  testperf2

hello_SOURCES = hello.cpp
test_am_bcast_SOURCES = test_am_bcast.cpp
test_array_SOURCES = test_array.cpp
test_array_dist_SOURCES = test_array_dist.cpp
test_async_SOURCES = test_async.cpp
test_dynamic_finish_SOURCES = test_dynamic_finish.cpp
test_event_SOURCES = test_event.cpp
test_finish_SOURCES = test_finish.cpp
test_global_ptr_SOURCES = test_global_ptr.cpp
test_reduce_SOURCES = test_reduce.cpp
test_remote_inc_SOURCES = test_remote_inc.cpp
test_safe_broadcast_SOURCES = test_safe_broadcast.cpp
test_shared_array_SOURCES = test_shared_array.cpp
test_shared_var_SOURCES = test_shared_var.cpp
testperf2_SOURCES = testperf2.cpp

AM_CPPFLAGS = \
  -I$(top_srcdir)/include \
  $(GASNET_CPPFLAGS) $(GASNET_CFLAGS)

AM_LDFLAGS = \
  $(top_builddir)/src/.libs/libupcxx.a \
  $(GASNET_LDFLAGS) $(GASNET_LIBS)
