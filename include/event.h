/**
 * event.h - Event object 
 */

#pragma once

#include <iostream>
#include <cassert>
#include <vector>

#include <stdio.h>
#include <stdlib.h>

#include "gasnet_api.h"
#include "queue.h"

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
    gasnet_hsl_t _lock;
    int _h_flag; // wait on handle?
    gasnet_handle_t _h;
    int _num_done_cb;
    async_task *_done_cb[MAX_NUM_DONE_CB];  
    // vector<async_task &> _cont_tasks;

    inline event() : _count(0), _h_flag(0), _h(GASNET_INVALID_HANDLE),
                       _num_done_cb(0)
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
      _count += c;
      gasnet_hsl_unlock(&_lock);

      return _count;
    }

    // Decrement the reference counter for the event
    inline void decref() 
    {
      if (_count == 0) {
        fprintf(stderr,
                "Fatal error: attempt to decrement a event (%p) with 0 references!\n",
                this);
        exit(1);
      }
        
      gasnet_hsl_lock(&_lock);
      _count--;
      gasnet_hsl_unlock(&_lock);

      if (_count == 0) {
        enqueue_cb();
      }
    }

    inline void sethandle(gasnet_handle_t h)
    {
      _h = h;
      _h_flag = 1;
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
    int test();
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

  extern event default_event; // defined in upcxx.cpp

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

  inline void wait()
  {
    wait(&default_event);
  }

  inline int test(event *e = peek_event())
  {
    if (e == NULL) {
      return 1;
    }

    return e->test();
  }

} // namespace upcxx
