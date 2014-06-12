/**
 * event.h - Event object 
 */

#pragma once

#include <iostream>
#include <cassert>
#include <vector>
#include <list>

#include <stdio.h>
#include <stdlib.h>

#include "gasnet_api.h"
#include "queue.h"
#include "upcxx_runtime.h"

using namespace std;

namespace upcxx
{
  struct async_task; // defined in async_gasnet.h

  // extern gasnet_hsl_t async_lock;
  // extern queue_t *async_task_queue;
  extern gasnet_hsl_t in_task_queue_lock;
  extern queue_t *in_task_queue;
  extern gasnet_hsl_t out_task_queue_lock;
  extern queue_t *out_task_queue;

#define MAX_NUM_DONE_CB 16

#define USE_EVENT_LOCK

  /**
   * \addtogroup asyncgroup Asynchronous task execution and Progress
   * @{
   * Events are used to notify asych task completion and invoke callback functions.
   */
  struct event {
    volatile int _count; // outstanding number of tasks.
    int owner;
#ifdef UPCXX_THREAD_SAFE
    pthread_mutex_t _mutex;
#endif
    list<gasnet_handle_t> _h;
    int _num_done_cb;
    async_task *_done_cb[MAX_NUM_DONE_CB];  
    // vector<async_task &> _cont_tasks;

    inline event()
    {
      _count = 0;
      _num_done_cb = 0;
#ifdef UPCXX_THREAD_SAFE
      pthread_mutex_init(&_mutex, NULL);
#endif
    }

    inline ~event()
    {
      this->wait();
    }

    inline int count() const { return _count; }

    void enqueue_cb();

    inline bool isdone() const
    {
      assert(_count >= 0);
      return (_count == 0);
    }
      
    // Increment the reference counter for the event
    void incref(uint32_t c=1);

    // Decrement the reference counter for the event
    void decref();

    void add_handle(gasnet_handle_t h);

    void remove_handle(gasnet_handle_t h);

    inline void lock() { upcxx_mutex_lock(&_mutex); }

    inline void unlock() { upcxx_mutex_unlock(&_mutex); }

    inline int num_done_cb() const { return _num_done_cb; }

    inline void add_done_cb(async_task *task)
    {
      // _lock should be held already
      assert(_num_done_cb < MAX_NUM_DONE_CB);
      _done_cb[_num_done_cb] = task;
      _num_done_cb++;
    }

    /**
     * Wait for the asynchronous event to complete
     */
    void wait(); 

    /**
     * Return 1 if the task is done; return 0 if not
     */
    int async_try();

    inline int test() { return async_try(); }
  };
  /// @}

  typedef struct event future;

  inline
  std::ostream& operator<<(std::ostream& out, const event& e)
  {
    return out << "gasnet event: count " << e.count()
               << ", # of callback tasks " << e.num_done_cb()
               << "\n";
  }

  extern event system_event; // defined in upcxx.cpp
  extern std::list<event *> outstanding_events;
  extern gasnet_hsl_t outstanding_events_lock;

  /* event stack interface used by finish */
  void push_event(event *);
  void pop_event();
  event *peek_event();

  inline void wait(event *e)
  {
    if (e != NULL) {
      e->wait();
    }
  }

  inline void async_wait()
  {
    while (!outstanding_events.empty()) {
      upcxx::advance(1,10);
    }
    system_event.wait();
    gasnet_wait_syncnbi_all();
  }

  inline void wait() { async_wait(); } // for backward compatibility

  inline int async_try(event *e = peek_event())
  {
    // There is at least the default event.
    assert (e != NULL);

    int rv = e->async_try();

    if (e == &system_event) {
      rv = rv && gasnet_try_syncnbi_all();
    }
    return rv;
  }

} // namespace upcxx
