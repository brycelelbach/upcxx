#include "upcxx.h"
#include "upcxx/upcxx_internal.h"

// #define UPCXX_DEBUG

namespace upcxx
{
  int copy(global_ptr<void> src, global_ptr<void> dst, size_t nbytes)
  {
#ifdef DEBUG
    fprintf(stderr, "src id %d, src ptr %p, nbytes %lu, dst id %d, dst ptr %p\n",
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
    if (e == system_event) {
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

  GASNETT_INLINE(copy_and_set_reply_inner)
  void copy_and_signal_reply_inner(gasnet_token_t token, void *done_ptr)
  {
    if (done_ptr != NULL) {
      event *done_event = (event *) done_ptr;
      done_event->decref();
    }
  }
  SHORT_HANDLER(copy_and_signal_reply, 1, 2,
                (token, UNPACK(a0)),
                (token, UNPACK2(a0, a1)));

  // Use the same handler for both Medium and Long AM requests.
  // Long and Medium handlers have the same signature.
  // See gasnet_handler.h for detail.
  GASNETT_INLINE(copy_and_signal_request_inner)
  void copy_and_signal_request_inner(gasnet_token_t token,
                                     void *addr,
                                     size_t nbytes,
                                     void *target_addr,
                                     void *signal_event,
                                     void *done_event)
  {
#ifdef UPCXX_DEBUG
    printf("copy_and_signal_request_inner: target_addr %p, signal_event %p\n",
           target_addr, signal_event);
#endif

    assert(signal_event != NULL);

    // We are using the same AM handler for both Medium and Long AMs.
    // For a long AM request, the target_addr should be NULL.
    if (target_addr != NULL) {
      memcpy(target_addr, addr, nbytes);
      gasnett_local_wmb(); // make sure the flag is set after the data written
    }
    
    ((event *)signal_event)->decref(); // signal the event on the destination
    
    if (done_event != NULL) {
      GASNET_SAFE(SHORT_REP(1,2,(token, COPY_AND_SIGNAL_REPLY, PACK(done_event))));
    }
  }
  MEDIUM_HANDLER(copy_and_signal_request, 3, 6,
                 (token, addr, nbytes, UNPACK(a0),      UNPACK(a1),      UNPACK(a1)),
                 (token, addr, nbytes, UNPACK2(a0, a1), UNPACK2(a2, a3), UNPACK2(a4, a5)));
  
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

    assert(signal_event != NULL);
    if (local_completion!= NULL) local_completion->incref();
    if (remote_completion!= NULL) remote_completion->incref();
    
    // implementation based on GASNet medium AM
    if (nbytes <= gasnet_AMMaxMedium()) {
      GASNET_SAFE(MEDIUM_REQ(3, 6, (dst.where(), COPY_AND_SIGNAL_REQUEST,
                                    src.raw_ptr(), nbytes,
                                    PACK(dst.raw_ptr()),
                                    PACK(signal_event),
                                    PACK(remote_completion))));
      local_completion->decref(); // send buffer is reusable immediately
    } else {
      /*
      assert(nbytes <= gasnet_AMMaxLongRequest()); 
      GASNET_SAFE(LONGASYNC_REQ(3, 6, (dst.where(), COPY_AND_SIGNAL_REQUEST,
                                       src.raw_ptr(), nbytes, dst.raw_ptr(),
                                       PACK(NULL), // no need to copy for long AMs
                                       PACK(signal_event),
                                       PACK(remote_completion))));
      */
      // async_copy_and_set implementation based on RDMA put and local async tasks
      event **temp_events = allocate_events(1);
      // start the async copy of the payload
      async_copy(src, dst, nbytes, temp_events[0]);
      // enqueue a local async task that will asynchronously set the remote flag via RMDA put
      if (local_completion != NULL)
        async_after(global_myrank(), temp_events[0])(event_decref, local_completion, 1);
      if (remote_completion != NULL)
        async_after(global_myrank(), temp_events[0])(event_decref, remote_completion, 1);
      async_after(dst.where(), temp_events[0])(event_decref, signal_event, 1);
      // enqueue another local task that will clean up the temp_events after e is done
      async_after(global_myrank(), temp_events[0])(deallocate_events, 1, temp_events);
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
