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
  
  int async_copy_and_signal(global_ptr<void> src,
                            global_ptr<void> dst,
                            size_t nbytes,
                            event *singal_event,
                            event *done_event)
  {
#ifdef UPCXX_DEBUG
    std::cout << "Rank " << global_myrank() << " async_copy_and_signal src " << src
              << " dst " << dst << " nbytes " << nbytes
              << " singal_event " << singal_event << " done_event " << done_event
              << "\n";
#endif
    
    // Either src or dst must be local
    if (dst.where() != global_myrank() && src.where() != global_myrank()) {
      fprintf(stderr, "async_copy error: either the src pointer or the dst ptr needs to be local.\n");
      gasnet_exit(1);
    }

    assert(singal_event != NULL);
    if (done_event != NULL) done_event->incref();
    
    // implementation based on GASNet medium AM
    if (nbytes <= gasnet_AMMaxMedium()) {
      GASNET_SAFE(MEDIUM_REQ(3, 6, (dst.where(), COPY_AND_SIGNAL_REQUEST,
                                    src.raw_ptr(), nbytes,
                                    PACK(dst.raw_ptr()),
                                    PACK(singal_event),
                                    PACK(done_event))));
    } else {
      assert(nbytes <= gasnet_AMMaxLongRequest()); 
      GASNET_SAFE(LONGASYNC_REQ(3, 6, (dst.where(), COPY_AND_SIGNAL_REQUEST,
                                       src.raw_ptr(), nbytes, dst.raw_ptr(),
                                       PACK(NULL), // no need to copy for long AMs
                                       PACK(singal_event),
                                       PACK(done_event))));

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
