#include "upcxx.h"
#include "upcxx_internal.h"

// #define DEBUG

#ifdef UPCXX_USE_DMAPP
#include "dmapp_channel/dmapp_helper.h"
#endif

namespace upcxx
{
  int copy(global_ptr<void> src, global_ptr<void> dst, size_t nbytes)
  {
#ifdef DEBUG
    fprintf(stderr, "src id %d, src ptr %p, nbytes %llu, dst id %d, dst ptr %p\n",
            src.where(), src.raw_ptr(), nbytes, dst.where(), dst.raw_ptr());
#endif
    if (dst.where() == global_myrank()) {
      gasnet_get_bulk(dst.raw_ptr(), src.where(), src.raw_ptr(), nbytes);
    } else if (src.where() == global_myrank()) {
      gasnet_put_bulk(dst.where(), dst.raw_ptr(), src.raw_ptr(), nbytes);
    } else {
      void *buf;
      buf = malloc(nbytes);
      assert(buf != NULL);
      gasnet_get_bulk(buf, src.where(), src.raw_ptr(), nbytes);
      gasnet_put_bulk(dst.where(), dst.raw_ptr(), buf, nbytes);
      ::free(buf);
    }

    return UPCXX_SUCCESS;
  }

  int async_copy(global_ptr<void> src, global_ptr<void> dst, size_t nbytes,
                 event *e)
  {
    if (dst.where() != global_myrank() && src.where() != global_myrank()) {
      fprintf(stderr, "async_copy error: either the src pointer or the dst ptr needs to be local.\n");
      gasnet_exit(1);
    }
    if (e == &system_event) {
      // use implicit non-blocking copy for the global scope,
      // need to call gasnet_wait_syncnbi_all() to synchronize later
      if (dst.where() == global_myrank()) {
        gasnet_get_nbi_bulk(dst.raw_ptr(), src.where(), src.raw_ptr(), nbytes);
      } else { // src.where().islocal() == true
        gasnet_put_nbi_bulk(dst.where(), dst.raw_ptr(), src.raw_ptr(), nbytes);
      }
    } else { // e != &system_event)
      // explicit non-blocking copy, need event->wait()/test() to
      // synchronize later
      gasnet_handle_t h;
      if (dst.where() == global_myrank()) {
        h = gasnet_get_nb_bulk(dst.raw_ptr(), src.where(), src.raw_ptr(), nbytes);
      } else { // src.where() == global_myrank()
        h = gasnet_put_nb_bulk(dst.where(), dst.raw_ptr(), src.raw_ptr(), nbytes);
      }
      e->add_gasnet_handle(h);
    }
    return UPCXX_SUCCESS;
  }

  void set_flag(global_ptr<flag_t> flag_addr)
  {
    (*flag_addr) = UPCXX_FLAG_VAL_SET;
  }

  void unset_flag(global_ptr<flag_t> flag_addr)
  {
    (*flag_addr) = UPCXX_FLAG_VAL_UNSET;
  }

  void async_set_flag(global_ptr<flag_t> flag_addr, event *e)
  {
    async_copy(global_ptr<const flag_t>(&UPCXX_FLAG_VAL_SET), flag_addr, sizeof(flag_t), e);
  }

  void async_unset_flag(global_ptr<flag_t> flag_addr, event *e)
  {
    async_copy(global_ptr<const flag_t>(&UPCXX_FLAG_VAL_UNSET), flag_addr, sizeof(flag_t), e);
  }

  event **allocate_events(uint32_t num_events)
  {
    event **temp_events = (event **)malloc(sizeof(event *) * num_events);
    assert(temp_events != NULL);

    for (uint32_t i = 0; i < num_events; i++) {
      temp_events[i] = new event;
      assert(temp_events[i] != NULL);
    }

    return temp_events;
  }

  void deallocate_events(uint32_t num_events, event **events)
  {
    assert(events != NULL);
    for (uint32_t i = 0; i < num_events; i++) {
      assert(events[i] != NULL);
      delete events[i];
    }
    free(events);
  }

  GASNETT_INLINE(copy_and_set_reply_inner)
  void copy_and_set_reply_inner(gasnet_token_t token, void *done_ptr)
  {
    if (done_ptr != NULL) {
      event *done_event = (event *) done_ptr;
      done_event->decref();
    }
  }
  SHORT_HANDLER(copy_and_set_reply_inner, 1, 2,
                (token, UNPACK(a0)),
                (token, UNPACK2(a0, a1)));

  GASNETT_INLINE(copy_and_set_request_inner)
  void copy_and_set_request_inner(gasnet_token_t token,
                                  void *addr,
                                  size_t nbytes,
                                  void *target_addr,
                                  void *flag_addr,
                                  void *done_event)
  {
    assert(target_addr != NULL);
    assert(flag_addr != NULL);
    memcpy(target_addr, addr, nbytes);
    gasnett_local_wmb(); // make sure the flag is set after the data written
    *(flag_t*)flag_addr = UPCXX_FLAG_VAL_SET;
    if (done_event != NULL) {
      SHORT_SRP(1,2,(token, gasneti_handleridx(copy_and_set_reply), PACK(done_event)));
    }
  }
  MEDIUM_HANDLER(copy_and_set_request, 3, 6,
                 (token, addr, nbytes, UNPACK(a0),      UNPACK(a1),      UNPACK(a1)),
                 (token, addr, nbytes, UNPACK2(a0, a1), UNPACK2(a2, a3), UNPACK2(a4, a5)));

#ifdef UPCXX_USE_DMAPP
  int async_put_and_set_dmapp(global_ptr<void> src,
                                global_ptr<void> dst,
                                size_t nbytes,
                                global_ptr<flag_t> flag_addr,
                                event *e)
  {
    uint64_t nelems;
    dmapp_type_t dtype;
    bytes_to_dmapp_type(dst.raw_ptr(), src.raw_ptr(), nbytes,
                           &nelems, &dtype);

    // Implicit-handle NB
    if (e == &system_event) {
      dmapp_put_flag_nbi(dst.raw_ptr(), // target_addr,
                         get_dmapp_seg(dst.where()), // dmapp_seg_desc_t *target_seg
                         dst.where(), // target_pe,
                         src.raw_ptr(), // source_addr,
                         nelems, // # of DMAPP type elements
                         dtype, // dmapp_type_t type,
                         flag_addr.raw_ptr(), // target_flag
                         UPCXX_FLAG_VAL_SET);
    } else {
      // Explicit-handle NB
      dmapp_syncid_handle_t h;
      dmapp_put_flag_nb(dst.raw_ptr(), // target_addr,
                        get_dmapp_seg(dst.where()), // dmapp_seg_desc_t *target_seg
                        dst.where(), // target_pe
                        src.raw_ptr(), // source_addr
                        nelems, // # of DMAPP type elements
                        dtype, // dmapp_type_t type,
                        flag_addr.raw_ptr(), // target_flag
                        UPCXX_FLAG_VAL_SET,
                        &h);
      e->add_dmapp_handle(h);
    }

    return UPCXX_SUCCESS;
    // fprintf(stderr, "async_copy_and_set for get (src pointer is remote) is not supported yet\n");
  }
#endif

  int async_copy_and_set(global_ptr<void> src,
                         global_ptr<void> dst,
                         size_t nbytes,
                         global_ptr<flag_t> flag_addr,
                         event *e)
  {
    // Either src or dst must be local
    if (dst.where() != global_myrank() && src.where() != global_myrank()) {
      fprintf(stderr, "async_copy error: either the src pointer or the dst ptr needs to be local.\n");
      gasnet_exit(1);
    }

    if (dst.where() != flag_addr.where()) {
      fprintf(stderr, "async_copy error: dst and flag_addr should be on the same rank.\n");
      gasnet_exit(1);
    }

    // Use the the Cray DMAPP specialized version of put-and-set if available,
    // otherwise use the generic implementation
#ifdef UPCXX_USE_DMAPP
    if (env_use_dmapp) {
      if (src.where() == global_myrank()) {
        return async_put_and_set_dmapp(src, dst, nbytes, flag_addr, e);
      }
    }
#endif

    // implementation based on GASNet medium AM
    if (env_use_am_for_copy_and_set && nbytes < gasnet_AMMaxMedium()) {
      GASNET_SAFE(MEDIUM_REQ(2, 4, (dst.where(), COPY_AND_SET_AM,
                                    src.raw_ptr(), nbytes,
                                    PACK(dst.raw_ptr()),
                                    PACK(flag_addr.raw_ptr()))));
    } else {
      // async_copy_and_set implementation based on RDMA put and local async tasks
      event **temp_events = allocate_events(1);
      // start the async copy of the payload
      async_copy(src, dst, nbytes, temp_events[0]);
      // enqueue a local async task that will asynchronously set the remote flag via RMDA put
      async_after(global_myrank(), temp_events[0], e)(async_set_flag, flag_addr, e);
      // enqueue another local task that will clean up the temp_events after e is done
      async_after(global_myrank(), e)(deallocate_events, 1, temp_events);
    }

    return UPCXX_SUCCESS;
  }

  void async_copy_fence()
  {
    upcxx::async_wait();
  }

  int async_copy_try()
  {
    return upcxx::async_try();
  }
} // end of namespace upcxx
