include_HEADERS =  \
	upcxx.h \
  upcxx.mak

if UPCXX_MD_ARRAY
UPCXX_MD_ARRAY_H_FILES = \
	upcxx/array.h \
  upcxx/array_bulk.h \
  upcxx/array_bulk_internal.h \
  upcxx/array_defs.h
else
UPCXX_MD_ARRAY_H_FILES =
endif

upcxxincludedir = $(includedir)/upcxx
upcxxinclude_HEADERS =  \
  upcxx/active_coll.h \
  upcxx/allocate.h \
  upcxx/async.h \
  upcxx/async_copy.h \
  upcxx/async_impl.h \
  upcxx/async_impl_templates1.h \
  upcxx/async_impl_templates2.h \
  upcxx/async_templates.h \
  upcxx/atomic.h \
  upcxx/broadcast.h \
  upcxx/coll_flags.h \
  upcxx/collective.h \
  upcxx/dl_malloc.h \
  upcxx/event.h \
  upcxx/finish.h \
  upcxx/forkjoin.h \
  upcxx/gasnet_api.h \
  upcxx/global_ptr.h \
  upcxx/global_ref.h \
  upcxx/group.h \
  upcxx/interfaces.h \
  upcxx/interfaces_internal.h \
  upcxx/lock.h \
  upcxx/progress_thread.h \
  upcxx/queue.h \
  upcxx/range.h \
  upcxx/reduce.h \
  upcxx/shared_array.h \
  upcxx/shared_var.h \
  upcxx/team.h \
  upcxx/timer.h \
  upcxx/upcxx.h \
  upcxx/upcxx_runtime.h \
  upcxx/upcxx_types.h \
  upcxx/utils.h  $(UPCXX_MD_ARRAY_H_FILES)

noinst_HEADERS = \
  upcxx/upcxx_internal.h

if UPCXX_USE_DMAPP
dmapp_includedir = $(includedir)/dmapp_channel
dmapp_include_HEADERS = dmapp_channel/dmapp_helper.h
endif

if UPCXX_MD_ARRAY
arrayincludedir = $(includedir)/upcxx-arrays
arrayinclude_HEADERS =  \
  upcxx-arrays/array.h \
  upcxx-arrays/array_addr.h \
  upcxx-arrays/array_impl.h \
  upcxx-arrays/array_impl.tpp \
  upcxx-arrays/array_undefs.h \
  upcxx-arrays/array_wrapper.h \
  upcxx-arrays/array_wrapper_specializations.h \
  upcxx-arrays/arrayflag_tools.h \
  upcxx-arrays/defs.h \
  upcxx-arrays/domain.h \
  upcxx-arrays/domain.tpp \
  upcxx-arrays/domain_undefs.h \
  upcxx-arrays/foreach.h \
  upcxx-arrays/iter.h \
  upcxx-arrays/point.h \
  upcxx-arrays/point.tpp \
  upcxx-arrays/point_aux.h \
  upcxx-arrays/rdomain.h \
  upcxx-arrays/rdomain.tpp \
  upcxx-arrays/utils.h \
  upcxx-arrays/wrapper_tools.h \
  upcxx-arrays/wrapper_tools_specializations.h
endif
