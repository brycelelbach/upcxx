#include "upcxx.h"

int upcxx::copy(ptr_to_shared<void> src, ptr_to_shared<void> dst, size_t nbytes)
{
#ifdef DEBUG
  fprintf(stderr, "src id %d, src ptr %p, nbytes %llu, dst id %d, dst ptr %p\n",
          src.where().id(), src.raw_ptr(), nbytes, dst.where().id(), dst.raw_ptr());
#endif
  if (dst.where().islocal()) {
    gasnet_get(dst.raw_ptr(), src.where().node_id(), src.raw_ptr(), nbytes);
  } else if (src.where().islocal()) {
    gasnet_put(dst.where().node_id(), dst.raw_ptr(), src.raw_ptr(), nbytes);
  } else {
    void *buf;
    buf = malloc(nbytes);
    assert(buf != NULL);
    gasnet_get(buf, src.where().node_id(), src.raw_ptr(), nbytes);
    gasnet_put(dst.where().node_id(), dst.raw_ptr(), buf, nbytes);
    ::free(buf);
  }

  return UPCXX_SUCCESS;
}

int upcxx::async_copy(ptr_to_shared<void> src,
                      ptr_to_shared<void> dst,
                      size_t nbytes,
                      event *e)
{
  if (e == NULL) {
    // implicit non-blocking copy, need async_fence() to
    // synchronize later
    if (dst.where().islocal()) {
      gasnet_get_nbi_bulk(dst.raw_ptr(), src.where().node_id(),
                          src.raw_ptr(), nbytes);
    } else if (src.where().islocal()) {
      gasnet_put_nbi_bulk(dst.where().node_id(), dst.raw_ptr(),
                          src.raw_ptr(), nbytes);
    } else {
      fprintf(stderr,
              "memcpy_nb error: either the src pointer or the dst ptr needs to be local.\n");
      exit(1);
    }
  } else {
    // explicit non-blocking copy, need event->wait()/test() to
    // synchronize later
    if (dst.where().islocal()) {
      gasnet_handle_t h;
      h = gasnet_get_nb_bulk(dst.raw_ptr(),
                             src.where().node_id(),
                             src.raw_ptr(),
                             nbytes);
      e->sethandle(h);
    } else if (src.where().islocal()) {
      gasnet_handle_t h;
      h = gasnet_put_nb_bulk(dst.where().node_id(),
                             dst.raw_ptr(),
                             src.raw_ptr(),
                             nbytes);
      e->sethandle(h);
    } else {
      // Not implemented
      fprintf(stderr,
              "memcpy_nb error: either the src pointer or the dst ptr needs to be local.\n");
      exit(1);
    }
  }
  return UPCXX_SUCCESS;
}

gasnet_handle_t upcxx::async_copy2(ptr_to_shared<void> src,
                                   ptr_to_shared<void> dst,
                                   size_t nbytes)
{
  // explicit non-blocking copy, need event->wait()/test() to
  // synchronize later
  gasnet_handle_t h;
  if (dst.where().islocal()) {
    h = gasnet_get_nb_bulk(dst.raw_ptr(),
                           src.where().node_id(),
                           src.raw_ptr(),
                           nbytes);
    
  } else if (src.where().islocal()) {
    h = gasnet_put_nb_bulk(dst.where().node_id(),
                           dst.raw_ptr(),
                           src.raw_ptr(),
                           nbytes);
    
  } else {
    // Not implemented
    fprintf(stderr,
            "copy_nb error: either the src pointer or the dst ptr needs to be local.\n");
    exit(1);
  }
  return h;
}


