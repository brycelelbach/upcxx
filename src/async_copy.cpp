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
      e->add_handle(h);
    }
    return UPCXX_SUCCESS;
  }

  int async_copy_and_set(global_ptr<void> src,
                         global_ptr<void> dst,
                         size_t nbytes,
                         global_ptr<uint64_t> flag_addr,
                         uint64_t flag_val,
                         event *e)
  {
    // Either src or dst must be local
    if (dst.where() != myrank() && src.where() != myrank()) {
      fprintf(stderr, "async_copy error: either the src pointer or the dst ptr needs to be local.\n");
      gasnet_exit(1);
    }

    //dmapp_return_t rv;
#ifdef UPCXX_USE_DMAPP
    if (src.where() == myrank()) {
      assert(dst.where() == flag_addr.where());
      // dmapp_return_t rv;

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
                           flag_val);
      } else {
        // Explicit-handle NB
        dmapp_syncid_handle_t h;
        dmapp_put_flag_nb(dst.raw_ptr(), // target_addr,
                          get_dmapp_seg(dst.where()), // dmapp_seg_desc_t *target_seg,
                          dst.where(), // target_pe,
                          src.raw_ptr(), // source_addr,
                          nelems, // # of DMAPP type elements
                          dtype, // IN  dmapp_type_t     type,
                          flag_addr.raw_ptr(), //  target_flag
                          flag_val,
                          &h);
    } else {
      // for get
      gasnet_exit("async_copy_and_set for get (src pointer is remote) is not supported yet\n");
    }
#endif
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
