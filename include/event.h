/**
 * event.h - Event object 
 */

#pragma once

#include <iostream>
#include <cassert>

#include <stdio.h>
#include <stdlib.h>


#include "gasnet_api.h"
#include "queue.h"

using namespace std;

namespace upcxx
{
  struct async_task; // defined in async_gasnet.h
  extern gasnet_hsl_t async_lock;
  extern queue_t *async_task_queue;

#define MAX_NUM_DONE_CB 16

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

    inline event() : _count(0), _h_flag(0), _h(GASNET_INVALID_HANDLE),
                       _num_done_cb(0)
    { 
      gasnet_hsl_init(&_lock);
    }

    inline int count() const { return _count; }

    inline void run_cb()
    {
      assert (_count == 0);

      gasnet_hsl_lock(&_lock);
      // add done_cb to the task queue
      if (_num_done_cb > 0) {
        gasnet_hsl_lock(&async_lock);
        for (int i=0; i<_num_done_cb; i++) {
          if (_done_cb[i] != NULL)
            queue_enqueue(async_task_queue, _done_cb[i]);
        }
        gasnet_hsl_unlock(&async_lock);
      }
      gasnet_hsl_unlock(&_lock);
    }

    inline bool isdone() const { return (_count == 0); }
      
    // Increment the reference counter for the event
    inline  int incref(uint32_t c=1) { return (_count += c); }

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

      if (isdone()) {
        run_cb();
      }
    }

    inline void sethandle(gasnet_handle_t h)
    {
      _h = h;
      _h_flag = 1;
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

  typedef event * async_event; // use the same type name as phalanx v2

  extern event default_event; // defined in upcxx.cpp

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

  inline int test(event *e = &default_event)
  {
    if (e == NULL) {
      return 1;
    }

    return e->test();
  }

} // namespace upcxx
