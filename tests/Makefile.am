## define test suite

TESTS_ENVIRONMENT = @UPCXX_TESTS_MPIRUN@

if UPCXX_MD_ARRAY
UPCXX_MD_ARRAY_TESTS = \
   ../examples/basic/test_acc_async \
   ../examples/basic/test_array \
   ../examples/basic/test_array_dist \
   ../examples/basic/test_array_ghost_copy \
   ../examples/basic/test_reduce \
   ../examples/basic/test_safe_broadcast
else
UPCXX_MD_ARRAY_TESTS =
endif

TESTS = \
  ../examples/basic/hello \
  ../examples/basic/test_active_message \
  ../examples/basic/test_am_bcast \
  ../examples/basic/test_asymmetric_partition \
  ../examples/basic/test_async \
  ../examples/basic/test_copy_closure \
  ../examples/basic/test_copy_and_signal \
  ../examples/basic/test_dynamic_finish \
  ../examples/basic/test_event \
  ../examples/basic/test_event2 \
  ../examples/basic/test_fetch_add \
  ../examples/basic/test_finish \
  ../examples/basic/test_global_ptr \
  ../examples/basic/test_lock \
  ../examples/basic/test_memberof \
  ../examples/basic/test_memberof2 \
  ../examples/basic/test_memberof3 \
  ../examples/basic/test_process_shared_mem \
  ../examples/basic/test_process_shared_mem2 \
  ../examples/basic/test_progress_thread \
  ../examples/basic/test_remote_inc \
  ../examples/basic/test_shared_array \
  ../examples/basic/test_shared_array2 \
  ../examples/basic/test_shared_var \
  ../examples/basic/test_team \
  ../examples/basic/testperf2 $(UPCXX_MD_ARRAY_TESTS)
