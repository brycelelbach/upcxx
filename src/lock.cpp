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
     
    if (!am->queryonly) {
      upcxx_mutex_lock(&lock->_mutex);
#ifdef UPCXX_DEBUG
      printf("lock_am_handler() rank %u, lock %p, _locked %d, _holder %u, _owner %u\n",
             global_myrank(), lock, lock->_locked, lock->_holder, lock->_owner);
#endif

      if (lock->_locked == 0) {
        lock->_holder = srcnode;
        lock->_locked = 1;
      } else if (lock->_holder == srcnode) {
        // \Todo Need to check the lock sequence carefully to avoid deadlocks!

        fprintf(stderr, "node %d is trying to lock a lock that it is holding!\n",
        srcnode);
        gasnet_exit(1);
      }
      upcxx_mutex_unlock(&lock->_mutex);
    }
    lock_reply_t reply;
    reply.rv.holder = lock->_holder;
    reply.rv.islocked = lock->_locked;
    reply.rv_addr = am->rv_addr;
    reply.cb_event = am->cb_event;

    GASNET_SAFE(gasnet_AMReplyMedium0(token, LOCK_REPLY, &reply, sizeof(reply)));
  }

  void shared_lock::lock_reply_handler(gasnet_token_t token,
                                       void *buf,
                                       size_t nbytes)
  {
      
    lock_reply_t *reply = (lock_reply_t *)buf;      
    assert(nbytes == sizeof(lock_reply_t));

    reply->rv_addr->holder = reply->rv.holder;
    reply->rv_addr->islocked = reply->rv.islocked;
    reply->cb_event->decref();
  }

  void shared_lock::unlock_am_handler(gasnet_token_t token,
                                      void *buf,
                                      size_t nbytes)
  {
    unlock_am_t *am = (unlock_am_t *)buf;
    assert(nbytes == sizeof(unlock_am_t));
    assert(am->lock->_locked);

    gasnet_node_t srcnode;
    GASNET_SAFE(gasnet_AMGetMsgSource(token, &srcnode));

    upcxx_mutex_lock(&am->lock->_mutex);
    if (am->lock->_holder != srcnode) {
      fprintf(stderr, "unlock_am_handler error: srcnode %u attempts to unlock a lock held by %u!\n",
              srcnode, am->lock->_holder);
      upcxx_mutex_unlock(&am->lock->_mutex);
      gasnet_exit(1);
    }
    am->lock->_holder = global_myrank();
    am->lock->_locked = 0;
    upcxx_mutex_unlock(&am->lock->_mutex);
  }

  int shared_lock::trylock()
  {
    event e;
    lock_rv_t rv;
    lock_am_t am;
    am.lock = myself;
    am.queryonly = 0;
    am.rv_addr = &rv;
    e.incref();
    am.cb_event = &e;

#ifdef UPCXX_DEBUG
    printf("trylock() rank %u, lock %p, _locked %d, _holder %u, _owner %u\n",
            global_myrank(), this, _locked, _holder, _owner);
#endif

    GASNET_SAFE(gasnet_AMRequestMedium0(get_owner(), LOCK_AM, &am, sizeof(am)));
    e.wait();

    if (rv.holder == global_myrank() && rv.islocked) {
      return 1;
    }

    return 0;
  }

  void shared_lock::lock()
  {
    GASNET_BLOCKUNTIL(trylock() == 1);
  }

  void shared_lock::unlock()
  {
    unlock_am_t am;
    am.lock = myself;
    GASNET_SAFE(gasnet_AMRequestMedium0(get_owner(), UNLOCK_AM, &am, sizeof(am)));

    // If Active Messages may be delivered out-of-order, we need
    // to wait for a reply from the lock master node (0) before
    // proceeding.  Otherwise, a later lock AM may fail if it is
    // delivered to the master node ahead of a preceding unlock AM
    // from the same source node.  We would need to add a reply
    // event in the unlock AM struct.

  }

  int shared_lock::islocked()
  {
    event e;
    lock_rv_t rv;
    lock_am_t am;
    am.lock = this;
    am.queryonly = 1;
    am.rv_addr = &rv;
    e.incref();
    am.cb_event = &e;

    GASNET_SAFE(gasnet_AMRequestMedium0(get_owner(), LOCK_AM, &am, sizeof(am)));
    e.wait();

    return rv.islocked;
  }
} // namespace upcxx


