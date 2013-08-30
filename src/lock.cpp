/*
 * lock.cpp - implement global lock 
 */

#include <assert.h>

#include "upcxx.h"
#include "upcxx_internal.h"

namespace upcxx
{
  void shared_lock::lock_am_handler(gasnet_token_t token,
                                    void *buf,
                                    size_t nbytes)
  {
    lock_am_t *am = (lock_am_t *)buf;
    assert(nbytes == sizeof(lock_am_t));
    shared_lock *lock = am->lock;

    gasnet_node_t srcnode;
    GASNET_SAFE(gasnet_AMGetMsgSource(token, &srcnode));

    assert(my_node.id() == 0);
     
    gasnet_hsl_lock(&lock->_hsl);
    if (lock->_locked == 0) {
      lock->_owner = srcnode; 
      lock->_locked = 1;
    } else if (lock->_owner == srcnode) {
      // \Todo Need to check the lock sequence carefully to avoid deadlocks!
      /*
        fprintf(stderr, "node %d is trying to lock a lock that it is holding!\n",
        srcnode);
      */
      //exit(1);
    }
    gasnet_hsl_unlock(&lock->_hsl);

    //(gasnet_hsl_trylock(&lock1) == GASNET_OK);

    lock_reply_t reply;
    reply.lock_owner = lock->_owner;
    reply.lock = lock;
    reply.cb_event = am->cb_event;
    GASNET_SAFE(gasnet_AMReplyMedium0(token, LOCK_REPLY, &reply, sizeof(reply)));
  }

  void shared_lock::lock_reply_handler(gasnet_token_t token,
                                       void *buf,
                                       size_t nbytes)
  {
      
    lock_reply_t *reply = (lock_reply_t *)buf;      
    assert(nbytes == sizeof(lock_reply_t));

    gasnet_node_t srcnode;
    GASNET_SAFE(gasnet_AMGetMsgSource(token, &srcnode));
    assert(srcnode == 0);
    assert(my_node.id() != 0);

    reply->lock->_owner = reply->lock_owner;
    reply->cb_event->decref();
  }

  void shared_lock::unlock_am_handler(gasnet_token_t token,
                                      void *buf,
                                      size_t nbytes)
  {
    unlock_am_t *am = (unlock_am_t *)buf;
    assert(nbytes == sizeof(unlock_am_t));

    gasnet_node_t srcnode;
    GASNET_SAFE(gasnet_AMGetMsgSource(token, &srcnode));

    if (am->lock->_owner != srcnode) {
      fprintf(stderr, "unlock_am_handler error: srcnode %u attempts to unlock a lock owned by %u!\n",
              srcnode, am->lock->_owner);
      exit(1);
    }

    gasnet_hsl_lock(&am->lock->_hsl);
    am->lock->_owner = 0;
    am->lock->_locked = 0;
    gasnet_hsl_unlock(&am->lock->_hsl);
  }

  int shared_lock::trylock()
  {
    // First try to acquire the remote lock and then lock the
    // local lock
      
    // gasneti_assert(locked == 0);
    int replied;

    if (my_node.id() == 0) {
      gasnet_hsl_lock(&_hsl);
      if (_locked == 0) {
        _locked = 1;
        _owner = 0;
        gasnet_hsl_unlock(&_hsl);
        return 1;
      } else {
        gasnet_hsl_unlock(&_hsl);
        return 0;
      }
    } else {
      gasnet_hsl_lock(&_hsl);
      event e;
      lock_am_t am;
      am.id = my_node.id();
      am.lock = this;
      e.incref();
      am.cb_event = &e;
      
      GASNET_SAFE(gasnet_AMRequestMedium0(0, LOCK_AM, &am, sizeof(am)));
      e.wait();

      if (_owner == my_node.id()) {
        _locked = 1;
        gasnet_hsl_unlock(&_hsl);
        return 1;
      }

      gasnet_hsl_unlock(&_hsl);
      return 0;
    }
  }

  void shared_lock::lock()
  {
    GASNET_BLOCKUNTIL(trylock() == 1);
  }

  void shared_lock::unlock()
  {
    assert(_locked);

    if (my_node.id() != 0) {
      unlock_am_t am;
      am.id = my_node.id();
      am.lock = this;
      GASNET_SAFE(gasnet_AMRequestMedium0(0, UNLOCK_AM, &am, sizeof(am)));
      // If Active Messages may be delivered out-of-order, we need
      // to wait for a reply from the lock master node (0) before
      // proceeding.  Otherwise, a later lock AM may fail if it is
      // delivered to the master node ahead of a preceding unlock AM
      // from the same source node.  We would need to add a reply
      // event in the unlock AM struct.
    } 
      
    gasnet_hsl_lock(&_hsl);
    _locked = 0;
    _owner = 0;
    gasnet_hsl_unlock(&_hsl);
  }
} // namespace upcxx


