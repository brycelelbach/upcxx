include ../../$(GASNET_MAKEFILE)

if UPCXX_MD_ARRAY
UPCXX_MD_ARRAY_BIN_FILES = \
  test_acc_async \
  test_array \
  test_array_dist \
  test_array_ghost_copy \
  test_reduce \
  test_safe_broadcast 
else
UPCXX_MD_ARRAY_BIN_FILES =
endif

bin_PROGRAMS = \
  hello \
  test_am_bcast \
  test_async \
  test_copy_closure \
  test_copy_and_signal \
  test_dynamic_finish \
  test_event \
  test_event2 \
  test_fetch_add \
  test_finish \
  test_global_ptr \
  test_lock \
  test_memberof \
  test_memberof2 \
  test_memberof3 \
  test_process_shared_mem \
	test_progress_thread \
  test_remote_inc \
  test_shared_array \
  test_shared_array2 \
  test_shared_var \
	test_team \
  testperf2 $(UPCXX_MD_ARRAY_BIN_FILES)

hello_SOURCES = hello.cpp
test_am_bcast_SOURCES = test_am_bcast.cpp
test_async_SOURCES = test_async.cpp
test_copy_closure_SOURCES = test_copy_closure.cpp
test_copy_and_signal_SOURCES = test_copy_and_signal.cpp
test_dynamic_finish_SOURCES = test_dynamic_finish.cpp
test_event_SOURCES = test_event.cpp
test_event2_SOURCES = test_event2.cpp
test_fetch_add_SOURCES = test_fetch_add.cpp
test_finish_SOURCES = test_finish.cpp
test_global_ptr_SOURCES = test_global_ptr.cpp
test_lock_SOURCES = test_lock.cpp
test_memberof_SOURCES = test_memberof.cpp
test_memberof2_SOURCES = test_memberof2.cpp
test_memberof3_SOURCES = test_memberof3.cpp
test_process_shared_mem_SOURCES = test_process_shared_mem.cpp
test_progress_thread_SOURCES = test_progress_thread.cpp
test_remote_inc_SOURCES = test_remote_inc.cpp
test_shared_array_SOURCES = test_shared_array.cpp
test_shared_array2_SOURCES = test_shared_array2.cpp
test_shared_var_SOURCES = test_shared_var.cpp
test_team_SOURCES = test_team.cpp
testperf2_SOURCES = testperf2.cpp

if UPCXX_MD_ARRAY
test_acc_async_SOURCES = test_acc_async.cpp
test_array_SOURCES = test_array.cpp
test_array_dist_SOURCES = test_array_dist.cpp
test_array_ghost_copy_SOURCES = test_array_ghost_copy.cpp
test_reduce_SOURCES = test_reduce.cpp
test_safe_broadcast_SOURCES = test_safe_broadcast.cpp
endif

AM_CPPFLAGS = \
  -I$(top_srcdir)/include \
  $(GASNET_CPPFLAGS)

AM_LDFLAGS = \
  $(GASNET_LDFLAGS) 

LDADD = $(top_builddir)/src/.libs/libupcxx.a $(GASNET_LIBS)

test_progress_thread_LDFLAGS = $(GASNET_LDFLAGS) -pthread
