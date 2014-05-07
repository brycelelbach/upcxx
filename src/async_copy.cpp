#include "upcxx.h"

// AK: should this be using bulk puts/gets?
int upcxx::copy(global_ptr<void> src, global_ptr<void> dst, size_t nbytes)
{
#ifdef DEBUG
  fprintf(stderr, "src id %d, src ptr %p, nbytes %llu, dst id %d, dst ptr %p\n",
          src.where().id(), src.raw_ptr(), nbytes, dst.where().id(), dst.raw_ptr());
#endif
  if (dst.where() == myrank()) {
    gasnet_get(dst.raw_ptr(), src.where(), src.raw_ptr(), nbytes);
  } else if (src.where() == myrank()) {
    gasnet_put(dst.where(), dst.raw_ptr(), src.raw_ptr(), nbytes);
  } else {
    void *buf;
    buf = malloc(nbytes);
    assert(buf != NULL);
    gasnet_get(buf, src.where(), src.raw_ptr(), nbytes);
    gasnet_put(dst.where(), dst.raw_ptr(), buf, nbytes);
    ::free(buf);
  }

  return UPCXX_SUCCESS;
}

int upcxx::async_copy(global_ptr<void> src,
                      global_ptr<void> dst,
                      size_t nbytes,
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
    outstanding_events.push_back(e);
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


