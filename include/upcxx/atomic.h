/**
 * atomic.h - atomic operations
 *
 * This is currently an experimental feature for preview.
 */

#pragma once

#include "gasnet_api.h"
#include "event.h"
#include "global_ptr.h"

#include <iostream>


namespace upcxx
{
  template<typename T>
  struct atomic {
  public:
    inline atomic(const T& data) : _data(data)
    { }

    inline atomic() : _data()
    { }

    inline T load() { return _data; }
    inline void store(const T& val) { _data = val; }

    inline T fetch_add(const T& add_val)
    {
      // no threading
      T old = _data;
      _data += add_val;
      return old;
    }
    
  private:
    T _data;
    // mutex
  };

  template<typename T>
  struct fetch_add_am_t {
    atomic<T> *obj;
    T add_val;
    T* old_val_addr;
    event* cb_event;
  };

  template<typename T>
  struct fetch_add_reply_t {
    T old_val;
    T* old_val_addr;
    event* cb_event;
  };

  #ifdef UPCXX_HAVE_CXX11
  template<typename T = uint64_t>
  #else
  template<typename T>
  #endif
  static void fetch_add_am_handler(gasnet_token_t token, void *buf, size_t nbytes)
  {
    fetch_add_am_t<T> *am = (fetch_add_am_t<T> *)buf;
    fetch_add_reply_t<T> reply;

    // if no threading
    reply.old_val = am->obj->fetch_add(am->add_val);
    reply.old_val_addr = am->old_val_addr;
    reply.cb_event = am->cb_event; // callback event on the src rank
    // YZ: should use a different GASNet AM handler id for different type T
    GASNET_SAFE(gasnet_AMReplyMedium0(token, FETCH_ADD_U64_REPLY,
                                      &reply, sizeof(reply)));
  }

  #ifdef UPCXX_HAVE_CXX11
  template<typename T = uint64_t>
  #else
  template<typename T>
  #endif
  static void fetch_add_reply_handler(gasnet_token_t token, void *buf, size_t nbytes)
  {
    fetch_add_reply_t<T> *reply = (fetch_add_reply_t<T> *)buf;

    *reply->old_val_addr = reply->old_val;
    reply->cb_event->decref();
  }

  /**
   * Atomic fetch-and-add: atomically add val to the atomic obj and
   * then return the old value of the atomic object
   */
  inline
  uint64_t fetch_add(global_ptr<atomic<uint64_t> > obj, uint64_t add_val)
  {
    event e;
    uint64_t old_val;
    fetch_add_am_t<uint64_t> am;

    gasnet_AMPoll(); // process other AMs to be fair

    am.obj = obj.raw_ptr();
    am.add_val = add_val;
    am.old_val_addr = &old_val;
    am.cb_event = &e;
    e.incref();
    
    // YZ: should use a different GASNet AM handler id for different type T
    GASNET_SAFE(gasnet_AMRequestMedium0(obj.where(), FETCH_ADD_U64_AM, &am, sizeof(am)));
    e.wait();

    gasnet_AMPoll(); // process other AMs to be fair

    return old_val;
  }

#if GASNETC_GNI_FETCHOP
  inline uint64_t my_fetch_add(global_ptr<uint64_t> obj, uint64_t add_val)
  {
    uint64_t old_val;

    old_val = gasnetX_fetchadd_u64_val(obj.where(), obj.raw_ptr(), add_val);

    return old_val;
  }
#endif
} // end of upcxx
