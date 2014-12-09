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

    assert(myrank() == 0);
     
    upcxx_mutex_lock(&lock->_mutex);

    if (lock->_locked == 0) {
      lock->_owner = srcnode; 
      lock->_locked = 1;
    } else if (lock->_owner == srcnode) {
      // \Todo Need to check the lock sequence carefully to avoid deadlocks!
      /*
        fprintf(stderr, "node %d is trying to lock a lock that it is holding!\n",
        srcnode);
      */
      //gasnet_exit(1);
    }
    upcxx_mutex_unlock(&lock->_mutex);

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
    assert(myrank() != 0);

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

    upcxx_mutex_lock(&am->lock->_mutex);
    if (am->lock->_owner != srcnode) {
      fprintf(stderr, "unlock_am_handler error: srcnode %u attempts to unlock a lock owned by %u!\n",
              srcnode, am->lock->_owner);
      upcxx_mutex_unlock(&am->lock->_mutex);
      gasnet_exit(1);
    }
    am->lock->_owner = 0;
    am->lock->_locked = 0;
    upcxx_mutex_unlock(&am->lock->_mutex);
  }

  int shared_lock::trylock()
  {
    // First try to acquire the remote lock and then lock the
    // local lock
      
    // gasneti_assert(locked == 0);
    if (myrank() == 0) {
      upcxx_mutex_lock(&_mutex);
      if (_locked == 0) {
        _locked = 1;
        _owner = 0;
        upcxx_mutex_unlock(&_mutex);
        return 1;
      } else {
        upcxx_mutex_unlock(&_mutex);
        return 0;
      }
    } else {
      upcxx_mutex_lock(&_mutex);
      event e;
      lock_am_t am;
      am.id = myrank();
      am.lock = this;
      e.incref();
      am.cb_event = &e;
      
      GASNET_SAFE(gasnet_AMRequestMedium0(0, LOCK_AM, &am, sizeof(am)));
      e.wait();

      if (_owner == myrank()) {
        _locked = 1;
        upcxx_mutex_unlock(&_mutex);
        return 1;
      }
      upcxx_mutex_unlock(&_mutex);

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

    if (myrank() != 0) {
      unlock_am_t am;
      am.id = myrank();
      am.lock = this;
      GASNET_SAFE(gasnet_AMRequestMedium0(0, UNLOCK_AM, &am, sizeof(am)));
      // If Active Messages may be delivered out-of-order, we need
      // to wait for a reply from the lock master node (0) before
      // proceeding.  Otherwise, a later lock AM may fail if it is
      // delivered to the master node ahead of a preceding unlock AM
      // from the same source node.  We would need to add a reply
      // event in the unlock AM struct.
    } 
      
    upcxx_mutex_lock(*_mutex);
    _locked = 0;
    _owner = 0;
    upcxx_mutex_unlock(&_mutex);
  }
} // namespace upcxx


