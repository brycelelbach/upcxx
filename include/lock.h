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
   * \ingroup syncgroup
   * global address space lock implemented by active messages and local handler-safe locks.
   */
  struct  shared_lock {
  private:
    volatile int _locked; /**< lock state: 1 - locked, 0 - unlocked */
    volatile gasnet_node_t _owner; /**< current owner of the lock */
    int _init_flag;
#ifdef UPCXX_THREAD_SAFE
    pthread_mutex_t _mutex; /**< local handler-safe lock for thread/signal safety */
#endif

  public:
    inline shared_lock()
    {
      _locked = 0;
      _owner = 0;
      _init_flag = 0;
#ifdef UPCXX_THREAD_SAFE
      pthread_mutex_init(&_mutex, NULL);
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
     * \return 1 if the lock is hold by the calling process
     */
    inline int islocked() { return _locked; }

    inline gasnet_node_t getowner() { return _owner; }
    inline void setowner(gasnet_node_t o) { _owner = o; }

    static void lock_am_handler(gasnet_token_t token, void *buf, size_t nbytes);
    static void lock_reply_handler(gasnet_token_t token, void *buf, size_t nbytes);
    static void unlock_am_handler(gasnet_token_t token, void *buf, size_t nbytes);

    /// \endcond
  };

  struct lock_am_t {
    int id;
    shared_lock *lock;
    event *cb_event;
  };

  struct lock_reply_t {
    shared_lock *lock;
    gasnet_node_t lock_owner;
    event *cb_event;
  };

  struct unlock_am_t {
    int id;
    shared_lock *lock;
  };
} // namespace upcxx
