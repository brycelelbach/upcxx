/*
 * lock.cpp - implement global lock
 */

#include <assert.h>

#include "upcxx.h"
#include "upcxx/upcxx_internal.h"

/// #define UPCXX_DEBUG

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
    GASNET_CHECK_RV(gasnet_AMGetMsgSource(token, &srcnode));

    if (!am->queryonly) {
      upcxx_mutex_lock(&lock->_mutex);
#ifdef UPCXX_DEBUG
      fprintf(stderr, "lock_am_handler() rank %u, lock %p, _locked %d, _holder %u, _owner %u\n",
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

    GASNET_CHECK_RV(gasnet_AMReplyMedium0(token, LOCK_REPLY, &reply, sizeof(reply)));
  }

  void shared_lock::lock_reply_handler(gasnet_token_t token,
                                       void *buf,
                                       size_t nbytes)
  {

    lock_reply_t *reply = (lock_reply_t *)buf;
    assert(nbytes == sizeof(lock_reply_t));

    reply->rv_addr->holder = reply->rv.holder;
    reply->rv_addr->islocked = reply->rv.islocked;
    gasnett_local_wmb();
    reply->cb_event->decref();

#ifdef UPCXX_DEBUG
      fprintf(stderr, "lock_reply_handler() rank %u, rv_addr %p, rv.holder %u,  rv.islocked %d\n",
              global_myrank(), reply->rv_addr, reply->rv_addr->holder, reply->rv_addr->islocked);
#endif
  }

  void shared_lock::unlock_am_handler(gasnet_token_t token,
                                      void *buf,
                                      size_t nbytes)
  {
    unlock_am_t *am = (unlock_am_t *)buf;
    assert(nbytes == sizeof(unlock_am_t));
    assert(am->lock->_locked);

    gasnet_node_t srcnode;
    GASNET_CHECK_RV(gasnet_AMGetMsgSource(token, &srcnode));

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

    unlock_reply_t reply;
    reply.cb_event = am->cb_event;
    GASNET_CHECK_RV(gasnet_AMReplyMedium0(token, UNLOCK_REPLY, &reply, sizeof(reply)));
  }

  void shared_lock::unlock_reply_handler(gasnet_token_t token,
                                         void *buf,
                                         size_t nbytes)
  {

    unlock_reply_t *reply = (unlock_reply_t *)buf;
    assert(nbytes == sizeof(unlock_reply_t));
    reply->cb_event->decref();
#ifdef UPCXX_DEBUG
    fprintf(stderr,"%u Unlock Reply handler\n", myrank());
#endif
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
    fprintf(stderr, "trylock() rank %u, lock %p, _locked %d, _holder %u, _owner %u\n",
            global_myrank(), this, _locked, _holder, _owner);
#endif

    GASNET_CHECK_RV(gasnet_AMRequestMedium0(get_owner(), LOCK_AM, &am, sizeof(am)));
    e.wait();

#ifdef UPCXX_DEBUG
    fprintf(stderr, "trylock() rank %u, lock %p, rv.holder %u, rv.islocked %u\n",
            global_myrank(), this, rv.holder, rv.islocked);
#endif


    if (rv.holder == global_myrank() && rv.islocked) {
      return 1;
    }

    return 0;
  }

  void shared_lock::lock()
  {
    GASNET_BLOCKUNTIL(trylock() == 1);
#ifdef UPCXX_DEBUG
    fprintf(stderr, "Rank %u gets the lock successfully.\n", myrank());
#endif
  }

  void shared_lock::unlock()
  {
    event e;
    unlock_am_t am;
    am.lock = myself;
    e.incref();
    am.cb_event = &e;
    GASNET_CHECK_RV(gasnet_AMRequestMedium0(get_owner(), UNLOCK_AM, &am, sizeof(am)));
    e.wait();
#ifdef UPCXX_DEBUG
    fprintf(stderr, "Rank %u releases the lock successfully.\n", myrank());
#endif
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

    GASNET_CHECK_RV(gasnet_AMRequestMedium0(get_owner(), LOCK_AM, &am, sizeof(am)));
    e.wait();

    return rv.islocked;
  }
} // namespace upcxx
