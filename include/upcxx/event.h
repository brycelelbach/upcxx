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

namespace upcxx
{
  struct async_task; // defined in async_gasnet.h

  // extern gasnet_hsl_t async_lock;
  // extern queue_t *async_task_queue;
  extern upcxx_mutex_t in_task_queue_lock;
  extern queue_t *in_task_queue;
  extern upcxx_mutex_t out_task_queue_lock;
  extern queue_t *out_task_queue;

#define MAX_NUM_DONE_CB 16

#define USE_EVENT_LOCK

  /**
   * \addtogroup asyncgroup Asynchronous task execution and Progress
   * @{
   * Events are used to notify asynch task completion and invoke callback functions.
   */
  struct event {
    volatile int _count; // outstanding number of tasks.
    int owner;
#if defined(UPCXX_THREAD_SAFE) || defined(GASNET_PAR)
    upcxx_mutex_t _mutex;
#endif
    std::vector<gasnet_handle_t> _gasnet_handles;
#ifdef UPCXX_USE_DMAPP
    std::vector<dmapp_syncid_handle_t> _dmapp_handles;
#endif
    int _num_done_cb;
    async_task *_done_cb[MAX_NUM_DONE_CB];  
    // std::vector<async_task &> _cont_tasks;

    inline event() : _count(0), _num_done_cb(0), owner(0)
    {
      upcxx_mutex_init(&_mutex);
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
    void decref(uint32_t c=1);

    void add_gasnet_handle(gasnet_handle_t h);

#ifdef UPCXX_USE_DMAPP
    void add_dmapp_handle(dmapp_syncid_handle_t h);
#endif

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

  void event_incref(event *e, uint32_t c=1);
  void event_decref(event *e, uint32_t c=1);

  typedef struct event future;

  inline
  std::ostream& operator<<(std::ostream& out, const event& e)
  {
    return out << "event: count " << e.count()
               << ", # of callback tasks " << e.num_done_cb()
               << "\n";
  }

  extern event *system_event; // defined in upcxx.cpp
  extern std::list<event *> *outstanding_events;
  extern upcxx_mutex_t outstanding_events_lock;

  /* event stack interface used by finish */
  void push_event(event *);
  void pop_event();
  event *peek_event();

  struct event_stack {
    std::vector<event *> stack;
    inline event_stack() {
      stack.push_back(system_event);
    }
  };
  extern event_stack *events;

  inline void async_wait(event *e)
  {
    if (e != NULL) {
      e->wait();
    }
  }

  void async_wait();

  int async_try(event *e = peek_event());

} // namespace upcxx
