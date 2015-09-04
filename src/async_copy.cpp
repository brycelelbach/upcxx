#include "upcxx.h"
#include "upcxx/upcxx_internal.h"

//#define UPCXX_DEBUG

namespace upcxx
{
  int copy(global_ptr<void> src, global_ptr<void> dst, size_t nbytes)
  {
#ifdef DEBUG
    fprintf(stderr, "src id %d, src ptr %p, nbytes %lu, dst id %d, dst ptr %p\n",
            src.where(), src.raw_ptr(), nbytes, dst.where(), dst.raw_ptr());
#endif
    if (dst.where() == global_myrank()) {
      UPCXX_CALL_GASNET(gasnet_get_bulk(dst.raw_ptr(), src.where(), src.raw_ptr(), nbytes));
    } else if (src.where() == global_myrank()) {
      UPCXX_CALL_GASNET(gasnet_put_bulk(dst.where(), dst.raw_ptr(), src.raw_ptr(), nbytes));
    } else {
      void *buf;
      buf = malloc(nbytes);
      assert(buf != NULL);
      UPCXX_CALL_GASNET(gasnet_get_bulk(buf, src.where(), src.raw_ptr(), nbytes));
      UPCXX_CALL_GASNET(gasnet_put_bulk(dst.where(), dst.raw_ptr(), buf, nbytes));
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
    if (e == system_event) {
      // use implicit non-blocking copy for the global scope,
      // need to call gasnet_wait_syncnbi_all() to synchronize later
      if (dst.where() == global_myrank()) {
        UPCXX_CALL_GASNET(gasnet_get_nbi_bulk(dst.raw_ptr(), src.where(), src.raw_ptr(), nbytes));
      } else { // src.where().islocal() == true
        UPCXX_CALL_GASNET(gasnet_put_nbi_bulk(dst.where(), dst.raw_ptr(), src.raw_ptr(), nbytes));
      }
    } else { // e != &system_event)
      // explicit non-blocking copy, need event->wait()/test() to
      // synchronize later
      gasnet_handle_t h;
      if (dst.where() == global_myrank()) {
        UPCXX_CALL_GASNET(h = gasnet_get_nb_bulk(dst.raw_ptr(), src.where(), src.raw_ptr(), nbytes));
      } else { // src.where() == global_myrank()
        UPCXX_CALL_GASNET(h = gasnet_put_nb_bulk(dst.where(), dst.raw_ptr(), src.raw_ptr(), nbytes));
      }
#ifdef UPCXX_DEBUG
      fprintf(stderr, "Rank %u async_copy to add handle %p to event %p\n", myrank(), h, e);
#endif
      e->add_gasnet_handle(h);
    }
    return UPCXX_SUCCESS;
  }

  GASNETT_INLINE(copy_and_set_reply_inner)
  void copy_and_signal_reply_inner(gasnet_token_t token, void *local_completion, void *remote_completion)
  {
#ifdef UPCXX_DEBUG
    printf("copy_and_signal_reply_inner: local_completion %p, remote_completion %p\n",
           local_completion, remote_completion);
#endif
    if (local_completion != NULL) {
      event *tmp = (event *) local_completion;
      tmp->decref();
    }
    if (remote_completion != NULL) {
      event *tmp = (event *) remote_completion;
      tmp->decref();
    }
  }
  SHORT_HANDLER(copy_and_signal_reply, 2, 4,
                (token, UNPACK(a0),      UNPACK(a1)),
                (token, UNPACK2(a0, a1), UNPACK2(a2, a3)));

  // Use the same handler for both Medium and Long AM requests.
  // Long and Medium handlers have the same signature.
  // See gasnet_handler.h for detail.
  GASNETT_INLINE(copy_and_signal_request_inner)
  void copy_and_signal_request_inner(gasnet_token_t token,
                                     void *addr,
                                     size_t nbytes,
                                     void *target_addr,
                                     void *signal_event,
                                     void *local_completion,
                                     void *remote_completion)
  {
#ifdef UPCXX_DEBUG
    printf("copy_and_signal_request_inner: target_addr %p, signal_event %p\n",
           target_addr, signal_event);
#endif

    // We are using the same AM handler for both Medium and Long AMs.
    // For a long AM request, the target_addr should be NULL.
    if (target_addr != NULL) {
      memcpy(target_addr, addr, nbytes);
      gasnett_local_wmb(); // make sure the flag is set after the data written
    }

    if (signal_event != NULL)
      ((event *)signal_event)->decref(); // signal the event on the destination

    if (local_completion != NULL || remote_completion != NULL) {
      // don't put UPCXX_CALL_GASNET here as it's already inside the lock through gasnet_AMPoll()
      GASNET_CHECK_RV(SHORT_REP(2,4,(token, COPY_AND_SIGNAL_REPLY, 
                                     PACK(local_completion), 
                                     PACK(remote_completion))));
    }
  }
  MEDIUM_HANDLER(copy_and_signal_request, 4, 8,
                 (token, addr, nbytes, UNPACK(a0),      UNPACK(a1),      UNPACK(a2),      UNPACK(a3)),
                 (token, addr, nbytes, UNPACK2(a0, a1), UNPACK2(a2, a3), UNPACK2(a4, a5), UNPACK2(a6, a7)));

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

  int async_copy_and_signal(global_ptr<void> src,
                            global_ptr<void> dst,
                            size_t nbytes,
                            event *signal_event,
                            event *local_completion,
                            event *remote_completion)
  {
#ifdef UPCXX_DEBUG
    std::cout << "Rank " << global_myrank() << " async_copy_and_signal src " << src
              << " dst " << dst << " nbytes " << nbytes
              << " signal_eventevent " << signal_event
              << " local_completion event " << local_completion
              << " remote_completion event " << remote_completion
              << "\n";
#endif

    // Either src or dst must be local
    if (dst.where() != global_myrank() && src.where() != global_myrank()) {
      fprintf(stderr, "async_copy error: either the src pointer or the dst ptr needs to be local.\n");
      gasnet_exit(1);
    }

    // implementation based on GASNet medium AM
    if (nbytes <= gasnet_AMMaxMedium()) {
      if (remote_completion!= NULL) remote_completion->incref();
      UPCXX_CALL_GASNET(
          GASNET_CHECK_RV(MEDIUM_REQ(4, 8, (dst.where(), COPY_AND_SIGNAL_REQUEST,
                                            src.raw_ptr(), nbytes,
                                            PACK(dst.raw_ptr()),
                                            PACK(signal_event),
                                            PACK(NULL), // no need to pass local_completion
                                            PACK(remote_completion)))));
    } else {
      // assert(nbytes <= gasnet_AMMaxLongRequest());
      /*
      if (local_completion!= NULL) local_completion->incref();
      if (remote_completion != NULL) remote_completion->incref();
      UPCXX_CALL_GASNET(LONGASYNC_REQ(4, 8, (dst.where(), COPY_AND_SIGNAL_REQUEST,
                                       src.raw_ptr(), nbytes, dst.raw_ptr(),
                                       PACK(NULL), // no need to copy for long AMs
                                       PACK(signal_event),
                                       PACK(local_completion),
                                       PACK(remote_completion)));
      */
      // async_copy_and_set implementation based on RDMA put and local async tasks
      event **temp_events = allocate_events(1);
      // start the async copy of the payload
      async_copy(src, dst, nbytes, temp_events[0]);

      if (local_completion != NULL) {
        local_completion->incref();
        async_after(global_myrank(), temp_events[0], NULL)(event_decref, local_completion, 1);
      }

      if (remote_completion != NULL) {
        remote_completion->incref();
        async_after(global_myrank(), temp_events[0], NULL)(event_decref, remote_completion, 1);
      }

      if (signal_event != NULL) {
        async_after(dst.where(), temp_events[0], NULL)(event_decref, signal_event, 1);
      }

      // enqueue another local task that will clean up the temp_events after e is done
      async_after(global_myrank(), temp_events[0], NULL)(deallocate_events, 1, temp_events);
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
