include $(GASNET_MAKEFILE)

if UPCXX_MD_ARRAY
UPCXX_MD_ARRAY_BIN_FILES = \
  test_array \
  test_array_dist \
  test_reduce \
  test_safe_broadcast 
else
UPCXX_MD_ARRAY_BIN_FILES =
endif

bin_PROGRAMS = \
  hello \
  test_am_bcast \
  test_async \
  test_dynamic_finish \
  test_event \
  test_event2 \
  test_finish \
  test_global_ptr \
  test_remote_inc \
  test_shared_array \
  test_shared_array2 \
  test_shared_var \
  testperf2 \
  test_memberof \
  test_memberof2 \
	test_memberof3 $(UPCXX_MD_ARRAY_BIN_FILES)

hello_SOURCES = hello.cpp
test_am_bcast_SOURCES = test_am_bcast.cpp
test_async_SOURCES = test_async.cpp
test_dynamic_finish_SOURCES = test_dynamic_finish.cpp
test_event_SOURCES = test_event.cpp
test_event2_SOURCES = test_event2.cpp
test_finish_SOURCES = test_finish.cpp
test_global_ptr_SOURCES = test_global_ptr.cpp
test_remote_inc_SOURCES = test_remote_inc.cpp
test_shared_array_SOURCES = test_shared_array.cpp
test_shared_array2_SOURCES = test_shared_array2.cpp
test_shared_var_SOURCES = test_shared_var.cpp
testperf2_SOURCES = testperf2.cpp
test_memberof_SOURCES = test_memberof.cpp
test_memberof2_SOURCES = test_memberof2.cpp
test_memberof3_SOURCES = test_memberof3.cpp

if UPCXX_MD_ARRAY
test_array_SOURCES = test_array.cpp
test_array_dist_SOURCES = test_array_dist.cpp
test_reduce_SOURCES = test_reduce.cpp
test_safe_broadcast_SOURCES = test_safe_broadcast.cpp
endif

AM_CPPFLAGS = \
  -I$(top_srcdir)/include \
  $(GASNET_CPPFLAGS) $(GASNET_CFLAGS)

AM_LDFLAGS = \
  $(top_builddir)/src/.libs/libupcxx.a \
  $(GASNET_LDFLAGS) $(GASNET_LIBS)
