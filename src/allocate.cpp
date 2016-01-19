#include "upcxx.h"
#include "upcxx/upcxx_internal.h"

namespace upcxx
{
  global_ptr<void> allocate(rank_t rank, size_t nbytes)
  {
    void *addr;

    if (rank == global_myrank()) {
#ifdef USE_GASNET_FAST_SEGMENT
      addr = gasnet_seg_alloc(nbytes);
#else
      addr = malloc(nbytes);
#endif
    } else {
      event e;
      e.incref();
      alloc_am_t am = { nbytes, &addr, &e };
      UPCXX_CALL_GASNET(gasnet_AMRequestMedium0(rank, ALLOC_CPU_AM, &am, sizeof(am)));
      e.wait();
    }
    global_ptr<void> ptr(addr, rank);

#ifdef DEBUG
    fprintf(stderr, "allocated %llu bytes at %p on node %d\n", nbytes, addr, rank);
#endif

    return ptr;
  }

  void deallocate(global_ptr<void> ptr)
  {
    if (ptr.where() == global_myrank()) {
#ifdef USE_GASNET_FAST_SEGMENT
      gasnet_seg_free(ptr.raw_ptr());
#else
      free(ptr.raw_ptr());
#endif
    } else {
      free_am_t am;
      am.ptr = ptr.raw_ptr();
      UPCXX_CALL_GASNET(gasnet_AMRequestMedium0(ptr.where(), FREE_CPU_AM, &am, sizeof(am)));
    }
  }

  void deallocate(void *ptr)
  {
#ifdef USE_GASNET_FAST_SEGMENT
    gasnet_seg_free(ptr);
#else
    free(ptr);
#endif
  } 

  void alloc_cpu_am_handler(gasnet_token_t token, void *buf, size_t nbytes)
  {
    assert(buf != NULL);
    assert(nbytes == sizeof(alloc_am_t));
    alloc_am_t *am = (alloc_am_t *)buf;

#ifdef UPCXX_DEBUG
    std::cerr << "Rank " << global_myrank() << " is inside alloc_cpu_am_handler.\n";
#endif

    alloc_reply_t reply;
    reply.ptr_addr = am->ptr_addr; // pass back the ptr_addr from the scr node
#ifdef USE_GASNET_FAST_SEGMENT
    reply.ptr = gasnet_seg_alloc(am->nbytes);
#else
    reply.ptr = malloc(am->nbytes);
#endif

#ifdef UPCXX_DEBUG
    assert(reply.ptr != NULL);
    std::cerr << "Rank " << global_myrank() << " allocated " << am->nbytes
              << " memory at " << reply.ptr << "\n";
#endif

    reply.cb_event = am->cb_event;
    GASNET_CHECK_RV(gasnet_AMReplyMedium0(token, ALLOC_REPLY, &reply, sizeof(reply)));
  }

  void alloc_reply_handler(gasnet_token_t token, void *buf, size_t nbytes)
  {
    // internal error checking
    assert(buf != NULL);
    assert(nbytes == sizeof(alloc_reply_t));
    // end of internal error checking

    alloc_reply_t *reply = (alloc_reply_t *)buf;

#ifdef UPCXX_DEBUG
    std::cerr << "Rank " << global_myrank() << " is in alloc_reply_handler. reply->ptr "
              << reply->ptr << "\n";
#endif

    *(reply->ptr_addr) = reply->ptr;
    reply->cb_event->decref();
  }

  void free_cpu_am_handler(gasnet_token_t token, void *buf, size_t nbytes)
  {
    assert(buf != NULL);
    assert(nbytes == sizeof(free_am_t));
    free_am_t *am = (free_am_t *)buf;
    if (am->ptr != NULL) {
#ifdef USE_GASNET_FAST_SEGMENT
      gasnet_seg_free(am->ptr);
#else
      free(am->ptr);
#endif
    }
  }
} // namespace upcxx
