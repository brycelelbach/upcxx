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
    gasnet_hsl_t _lock;
    list<gasnet_handle_t> _h;
    int _num_done_cb;
    async_task *_done_cb[MAX_NUM_DONE_CB];  
    // vector<async_task &> _cont_tasks;

    inline event() : _count(0), _num_done_cb(0)
    {
      gasnet_hsl_init(&_lock);
    }

    inline int count() const { return _count; }

    void enqueue_cb();

    inline bool isdone() const
    {
      assert(_count >= 0);
      return (_count == 0);
    }
      
    // Increment the reference counter for the event
    inline int incref(uint32_t c=1)
    {
      gasnet_hsl_lock(&_lock);
      int tmp = _count += c;
      gasnet_hsl_unlock(&_lock);
      return tmp;
    }

    // Decrement the reference counter for the event
    inline void decref() 
    {
      gasnet_hsl_lock(&_lock);
      if (_count == 0) {
        fprintf(stderr,
                "Fatal error: attempt to decrement an event (%p) with 0 references!\n",
                this);
        gasnet_exit(1);
      }
        
      int tmp = --_count; // obtain the value of count before unlock
      gasnet_hsl_unlock(&_lock);

      if (tmp == 0) {
        enqueue_cb();
      }
    }

    inline void add_handle(gasnet_handle_t h)
    {
      _h.push_back(h);
      incref();
    }

    inline int num_done_cb() const { return _num_done_cb; }

    inline void add_done_cb(async_task *task)
    {
      gasnet_hsl_lock(&_lock);
      assert(_num_done_cb < MAX_NUM_DONE_CB);
      _done_cb[_num_done_cb] = task;
      _num_done_cb++;
      gasnet_hsl_unlock(&_lock);
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
