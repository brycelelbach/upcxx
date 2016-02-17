/**
 * lock.h - global lock 
 *
 * This lock only provides mutual exclusion among processes (gasnet
 * nodes) but not for threads within the same process.  If
 * thread-level lock is needed, the client may use pthread_mutex_t.
 *
 */

#pragma once

#include "gasnet_api.h"
#include "event.h"

namespace upcxx
{
  /*
   * Todo: shared_lock variables can only be defined in the file
   * scope statically due to limitations in the current
   * implementation.
   */

  /**
   * @ingroup syncgroup
   * global address space lock implemented by active messages and local handler-safe locks.
   */
  struct  shared_lock {
  private:
    volatile int _locked; /**< lock state: 1 - locked, 0 - unlocked */
    rank_t _owner; /**< the owner of the lock */
    rank_t _holder; /**< the current holder of the lock */
    shared_lock *myself;

#if defined(UPCXX_THREAD_SAFE) || defined(GASNET_PAR)
    upcxx_mutex_t _mutex; /**< local handler-safe lock for thread/signal safety */
#endif

  public:
    inline shared_lock(rank_t owner=0)
    {
      _locked = 0;
      _owner = owner;
      _holder = _owner;

      myself = this;
#if defined(UPCXX_THREAD_SAFE)  || defined(GASNET_PAR)
      upcxx_mutex_init(&_mutex);
#endif
    }

    /**
     * Try to acquire the lock without blocking. return 1 if the
     * lock is acquired; return 0 otherwise.
     */
    int trylock();

    /**
     * Acquire the lock and would wait until succeed
     */
    void lock();

    /**
     * Release the lock
     */
    void unlock();

    // Make lock AM handler functions as member functions so that
    // they can access private data

    /// \cond SHOW_INTERNAL

    /**
     * @return 1 if the lock is hold by the calling process
     */
    int islocked();

    inline rank_t get_owner() { return _owner; }
    inline void set_owner(gasnet_node_t o) { _owner = o; }

    inline rank_t get_holder() { return _holder; }
    inline void set_holder(rank_t r) { _holder = r; }

    static void lock_am_handler(gasnet_token_t token, void *buf, size_t nbytes);
    static void lock_reply_handler(gasnet_token_t token, void *buf, size_t nbytes);
    static void unlock_am_handler(gasnet_token_t token, void *buf, size_t nbytes);
    static void unlock_reply_handler(gasnet_token_t token, void *buf, size_t nbytes);

    /// @endcond
  };

  struct lock_reply_t; // partial declaration of using its pointer type

  struct lock_rv_t {
    rank_t holder;
    int islocked;
  };

  struct lock_am_t {
    shared_lock *lock;
    lock_rv_t *rv_addr; // the address for the return value
    event *cb_event;
    int queryonly;
  };

  struct lock_reply_t {
    lock_rv_t *rv_addr;
    lock_rv_t rv;
    event *cb_event;
  };

  struct unlock_am_t {
    shared_lock *lock;
    event *cb_event;
  };

  struct unlock_reply_t {
    event *cb_event;
  };

} // namespace upcxx
