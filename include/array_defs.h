#pragma once

#include "machine.h"

#define UPCXXA_PROCS THREADS
#define UPCXXA_MYPROC MYTHREAD
#define UPCXXA_GLOBAL_PROCS GLOBAL_THREADS
#define UPCXXA_GLOBAL_MYPROC GLOBAL_MYTHREAD

#define UPCXXA_DEREF                            UPCXXA_DEREF_LOCAL
#define UPCXXA_ASSIGN                           UPCXXA_ASSIGN_LOCAL
#define UPCXXA_WEAK_ASSIGN                      UPCXXA_ASSIGN
#define UPCXXA_INDEX_GLOBAL                     UPCXXA_INDEX_LOCAL

#define UPCXXA_MALLOC                           upcxx::allocate
#define UPCXXA_FREE                             upcxx::deallocate
#define UPCXXA_SYNC() /* FIX */
#define UPCXXA_ARRAY_MALLOC                     upcxx::allocate
#define UPCXXA_ARRAY_FREE                       upcxx::deallocate
#define UPCXXA_GETENV_MASTER                    gasnet_getenv
#define UPCXXA_EVENT_T                          upcxx::event *
#define UPCXXA_DEFAULT_EVENT                    upcxx::peek_event()
#define UPCXXA_EVENT_NONE                       NULL

#if defined(GASNET_TRACE)
# define UPCXXA_TRACE
# define UPCXXA_TRACE_PRINTF(parenthesized_args)        \
  (GASNETT_TRACE_ENABLED ?                              \
   GASNETT_TRACE_PRINTF parenthesized_args :            \
   ((void)0))
#else
# define UPCXXA_TRACE_PRINTF(parenthesized_args) ((void)0)
#endif

#define UPCXXA_BULK_READ(d_addr, s_gaddr, size) do {    \
    gasnet_get_bulk(d_addr, UPCXXA_GET_BOXID(s_gaddr),  \
                    UPCXXA_TO_LOCAL(s_gaddr), size);    \
  } while(0)
#define UPCXXA_BULK_WRITE(d_gaddr, s_addr, size) do {           \
    gasnet_put_bulk(UPCXXA_GET_BOXID(d_gaddr),                  \
                    UPCXXA_TO_LOCAL(d_gaddr), s_addr, size);    \
  } while(0)
#define UPCXXA_ASSIGN_GLOBAL_ANONYMOUS_BULK(gptr, nitems, pval) \
  UPCXXA_BULK_WRITE(gptr, (void *)(pval),                       \
                    sizeof(*(pval)) * (nitems))

#define UPCXXA_LOCAL_TO_GLOBAL_COPY(local, global, nitems)      \
  UPCXXA_ASSIGN_GLOBAL_ANONYMOUS_BULK(global, nitems, local)
#define UPCXXA_GPTR_TO_T upcxx::global_ptr<T>
#define UPCXXA_GREF_TO_T upcxx::global_ref<T>
#define UPCXXA_GET_BOXID(gptr) (UPCXXA_TO_BOX(gptr)).id()
#define UPCXXA_BOX_T node
#define UPCXXA_TO_BOX(ptr) (ptr).where()
#define UPCXXA_TO_LOCAL(ptr) (ptr).raw_ptr()
#define UPCXXA_TO_GLOBALB(global, box, local)   \
  (global) = upcxx::global_ptr<T>(local, box)
#define UPCXXA_EQUAL_GLOBAL(p, q) ((p) == (q))
#define UPCXXA_MYBOX upcxx::my_node
#define UPCXXA_IS_DIRECTLY_ADDRESSABLE(ptr)     \
  ((ptr.where()) == upcxx::myrank())
#define UPCXXA_GLOBALIZE(global, local)                 \
  UPCXXA_TO_GLOBALB(global, UPCXXA_MYBOX, local)
#define UPCXXA_PROC_TO_BOXID_PROC(proc, boxid, bproc)           \
  upcxx::global_machine.cpu_id_global2local(proc, boxid, bproc)
#define UPCXXA_BOXID_TO_BOX(boxid)              \
  upcxx::global_machine.get_node(boxid)
#define UPCXXA_BOX_TO_FIRST_PROC(box)                   \
  upcxx::global_machine.cpu_id_local2global(box, 0)

#define UPCXXA_PUT_NB_BULK(event, destgaddr, srcaddr, nbytes)   \
  upcxx::async_copy(global_ptr<void>(srcaddr),                  \
                    global_ptr<void>(destgaddr),                \
                    nbytes, event)
#define UPCXXA_GET_NB_BULK(event, destaddr, srcgaddr, nbytes)   \
  upcxx::async_copy(global_ptr<void>(srcgaddr),                 \
                    global_ptr<void>(destaddr),                 \
                    nbytes, event)
#define UPCXXA_GET_ARRAY(pmethod, cdesc, cdescsz, target, buffer) \
  upcxx::get_array(pmethod, cdesc, cdescsz, target, buffer)
#define UPCXXA_PUT_ARRAY(umethod, cdesc, cdescsz, data, datasz, target, event) \
  upcxx::put_array(umethod, cdesc, cdescsz, data, datasz, target, event)
#define UPCXXA_SPARSE_SCATTER(dsts, src, target, num, elemsz) \
  upcxx::sparse_scatter(dsts, src, target, num, elemsz)
#define UPCXXA_SPARSE_GATHER(dst, srcs, target, num, elemsz) \
  upcxx::sparse_gather(dst, srcs, target, num, elemsz)

#define UPCXXA_BROADCAST_RAW(dst, sender, src, size)            \
  do {                                                          \
    gasnet_coll_broadcast(current_gasnet_team(), dst, sender,   \
			  src, size, UPCXX_GASNET_COLL_FLAG);   \
  } while (0)
#define UPCXXA_EXCHANGE_BULK(dst, src, bytes)                   \
  do {                                                          \
    gasnet_coll_gather_all(current_gasnet_team(), dst, src,     \
                           bytes, UPCXX_GASNET_COLL_FLAG);      \
  } while (0)
