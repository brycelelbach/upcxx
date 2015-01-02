#include "upcxx.h"

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
    if (dst.where() == myrank()) {
      gasnet_get_bulk(dst.raw_ptr(), src.where(), src.raw_ptr(), nbytes);
    } else if (src.where() == myrank()) {
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
    if (dst.where() != myrank() && src.where() != myrank()) {
      fprintf(stderr, "async_copy error: either the src pointer or the dst ptr needs to be local.\n");
      gasnet_exit(1);
    }
    if (e == &system_event) {
      // use implicit non-blocking copy for the global scope,
      // need to call gasnet_wait_syncnbi_all() to synchronize later
      if (dst.where() == myrank()) {
        gasnet_get_nbi_bulk(dst.raw_ptr(), src.where(), src.raw_ptr(), nbytes);
      } else { // src.where().islocal() == true
        gasnet_put_nbi_bulk(dst.where(), dst.raw_ptr(), src.raw_ptr(), nbytes);
      }
    } else { // e != &system_event)
      // explicit non-blocking copy, need event->wait()/test() to
      // synchronize later
      gasnet_handle_t h;
      if (dst.where() == myrank()) {
        h = gasnet_get_nb_bulk(dst.raw_ptr(), src.where(), src.raw_ptr(), nbytes);
      } else { // src.where() == myrank()
        h = gasnet_put_nb_bulk(dst.where(), dst.raw_ptr(), src.raw_ptr(), nbytes);
      }
      e->add_gasnet_handle(h);
    }
    return UPCXX_SUCCESS;
  }

  void set_flag(flag_t *flag_addr)
  {
    (*flag_addr) = UPCXX_FLAG_VAL_SET;
  }

  void unset_flag(flag_t *flag_addr)
  {
    (*flag_addr) = UPCXX_FLAG_VAL_UNSET;
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

#ifdef UPCXX_USE_DMAPP
  int async_put_and_set_w_dmapp(global_ptr<void> src,
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
                         get_dmapp_seg(dst.where()), // dmapp_seg_desc_t *target_seg,
                         dst.where(), // target_pe,
                         src.raw_ptr(), // source_addr,
                         nelems, // # of DMAPP type elements
                         dtype, // IN  dmapp_type_t     type,
                         flag_addr.raw_ptr(), //  target_flag
                         UPCXX_FLAG_VAL_SET);
    } else {
      // Explicit-handle NB
      dmapp_syncid_handle_t h;
      dmapp_put_flag_nb(dst.raw_ptr(), // target_addr,
                        get_dmapp_seg(dst.where()), // dmapp_seg_desc_t *target_seg,
                        dst.where(), // target_pe,
                        src.raw_ptr(), // source_addr
                        nelems, // # of DMAPP type elements
                        dtype, // IN  dmapp_type_t     type,
                        flag_addr.raw_ptr(), //  target_flag
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
    if (dst.where() != myrank() && src.where() != myrank()) {
      fprintf(stderr, "async_copy error: either the src pointer or the dst ptr needs to be local.\n");
      gasnet_exit(1);
    }

    // Use the the Cray DMAPP specialized version of put-and-set if available,
    // otherwise use the generic implementation
#ifdef UPCXX_USE_DMAPP
    if (src.where() == myrank()) {
      assert(dst.where() == flag_addr.where());
      return async_put_and_set_w_dmapp(src, dst, nbytes, flag_addr, e);
    }
#endif

    event **temp_events = allocate_events(2);
    async_copy(src, dst, nbytes, temp_events[0]);
    async_after(dst.where(), temp_events[0], temp_events[1])(set_flag, flag_addr.raw_ptr());
    async_after(myrank(), temp_events[1], e)(deallocate_events, 2, temp_events);
    //dmapp_return_t rv;



    return UPCXX_SUCCESS;
  }

  void upcxx::async_copy_fence()
  {
    upcxx::async_wait();
  }

  int upcxx::async_copy_try()
  {
    return upcxx::async_try();
  }
} // end of namespace upcxx
