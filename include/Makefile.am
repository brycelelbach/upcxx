if UPCXX_MD_ARRAY
UPCXX_MD_ARRAY_H_FILES = \
	array.h \
  array_bulk.h \
  array_bulk_internal.h \
  array_defs.h \
  broadcast.h \
  interfaces.h \
  interfaces_internal.h \
  reduce.h 
else
UPCXX_MD_ARRAY_H_FILES =
endif

include_HEADERS =  \
  active_coll.h \
  allocate.h \
  async.h \
  async_copy.h \
  async_impl.h \
  async_impl_templates1.h \
  async_impl_templates2.h \
  async_templates.h \
  atomic.h \
  coll_flags.h \
  collective.h \
  dl_malloc.h \
  event.h \
  finish.h \
  forkjoin.h \
  gasnet_api.h \
  global_ptr.h \
  global_ref.h \
  group.h \
  lock.h \
  queue.h \
  range.h \
  shared_array.h \
  shared_var.h \
  team.h \
  timer.h \
  upcxx.h \
  upcxx_runtime.h \
  upcxx_types.h \
  utils.h \
  upcxx.mak $(UPCXX_MD_ARRAY_H_FILES)

noinst_HEADERS = \
  upcxx_internal.h 

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
