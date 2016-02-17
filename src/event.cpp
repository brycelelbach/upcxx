/**
 * UPCXX events
 */

#include <stdio.h>
#include <assert.h>
#include <vector>

#include "upcxx.h"

//#define UPCXX_DEBUG

using namespace std;

namespace upcxx
{
  int event::_async_try()
  {
    // Acquired the lock
#ifdef UPCXX_DEBUG
    fprintf(stderr, "Rank %u is calling async_try with event %p\n", myrank(), this);
#endif

    // check outstanding gasnet handles
    if (!_gasnet_handles.empty()) {
      // for (auto it = _h.begin(); it != _h.end()) { // UPCXX_HAVE_CXX11
      for (std::vector<gasnet_handle_t>::iterator it = _gasnet_handles.begin();
           it != _gasnet_handles.end();) {
#ifdef UPCXX_DEBUG
        fprintf(stderr, "Rank %u event %p is checking handle %p in async_try.\n",
                myrank(), this, *it);
#endif
        // It's very important Not to poll GASNet while in async_try otherwise deadlocks may happen!
        int rv;
        UPCXX_CALL_GASNET(rv = gasnet_try_syncnb_nopoll(*it));
        if (rv == GASNET_OK) {
          _gasnet_handles.erase(it); // erase increase it automatically
          _decref(1);
        } else {
          ++it;
        }
      }
    }

#ifdef UPCXX_USE_DMAPP
    // check outstanding dmapp handles
    if (!_dmapp_handles.empty()) {
     // for (auto it = _h.begin(); it != _h.end()) { // UPCXX_HAVE_CXX11
     for (std::vector<dmapp_syncid_handle_t>::iterator it = _dmapp_handles.begin();
           it != _dmapp_handles.end();) {
       int flag;
       DMAPP_SAFE(dmapp_syncid_test(*it, &flag);
       if (flag) {
          _dmapp_handles.erase(it); // erase increase it automatically
          _decref(1);
        } else {
          ++it;
        }
      }
    }
#endif

    return isdone();
  }

  void event::wait()
  {
    while (!async_try()) {
      advance();
      // YZ: don't need to yield if CPUs are not over-subscribed.
      // gasnett_sched_yield();
    }
  }

  void event::_enqueue_cb()
  {
    assert (_count == 0);

    // add done_cb to the task queue
    if (_num_done_cb > 0) {
      for (int i=0; i<_num_done_cb; i++) {
        if (_done_cb[i] != NULL) {
          async_task *task = _done_cb[i];
          if (task->_callee == global_myrank()) {
            // local task
            assert(in_task_queue != NULL);
            upcxx_mutex_lock(&in_task_queue_lock);
            queue_enqueue(in_task_queue, task);
            upcxx_mutex_unlock(&in_task_queue_lock);
          } else {
            // remote task
            assert(out_task_queue != NULL);
            upcxx_mutex_lock(&out_task_queue_lock);
            queue_enqueue(out_task_queue, task);
            upcxx_mutex_unlock(&out_task_queue_lock);
          }
        }
      }
      _num_done_cb = 0;
    }
  }

  int num_inc_system_event = 0;
  int num_dec_system_event = 0;

  // upcxx_mutex_t extra_event_lock = UPCXX_MUTEX_INITIALIZER;

  // Increment the reference counter for the event
  void event::_incref(uint32_t c)
  {
    // upcxx_mutex_lock(&extra_event_lock);
    uint32_t old = _count;
    _count += c;
#ifdef UPCXX_DEBUG
    fprintf(stderr, "P %u _incref event %p, c = %d, old %u\n", global_myrank(), this, c, old);
#endif
    
    // some sanity check
    if (this == system_event) {
      num_inc_system_event += c;
      if (num_inc_system_event - num_dec_system_event != _count) {
        fprintf(stderr, "Fatal error in _incref: Rank %u race condition happened for event %p, _count %d, num_inc %d, num_dec %d.\n",
                myrank(), this, _count, num_inc_system_event, num_dec_system_event);
        gasnet_exit(1);
      }
    }

    if (this != system_event && old == 0) {

#ifdef UPCXX_DEBUG
      fprintf(stderr, "P %u Add outstanding_event %p\n", global_myrank(), this);
#endif
      outstanding_events->push_back(this);
    }
    // upcxx_mutex_unlock(&extra_event_lock);
  }

  // Decrement the reference counter for the event
  void event::_decref(uint32_t c)
  {
#ifdef UPCXX_DEBUG
    fprintf(stderr, "P %u _decref event %p, c = %d, old %u\n", global_myrank(), this, c, _count);
#endif
    // upcxx_mutex_lock(&extra_event_lock);

    _count -= c;

    if (this == system_event) {
      num_dec_system_event += c;
      if (num_inc_system_event - num_dec_system_event != _count) {
        fprintf(stderr, "Fatal error in _decref: Rank %u race condition happened for event %p, _count %d, num_inc %d, num_dec %d.\n",
                myrank(), this, _count, num_inc_system_event, num_dec_system_event);
        gasnet_exit(1);
      }
    }

    if (_count < 0) {
      fprintf(stderr,
              "Fatal error: Rank %u attempt to decrement an event (%p) to be a negative number of references!\n",
              global_myrank(), this);
      fprintf(stderr,
              "Fatal error: Rank %u this event %p, _count %d, c %u, system_event %p, num_inc %d, num_dec %d.\n",
              global_myrank(), this, _count, c, system_event, num_inc_system_event, num_dec_system_event);
      gasnet_exit(1);
    }
    if (_count == 0  && this != system_event) {
      _enqueue_cb();
#ifdef UPCXX_DEBUG
        fprintf(stderr, "P %u Erase outstanding_event %p\n", global_myrank(), this);
#endif
        outstanding_events->remove(this);
    }
    // upcxx_mutex_unlock(&extra_event_lock);
  }

  event_stack *events = NULL;

  void event::_add_gasnet_handle(gasnet_handle_t h)
  {
#ifdef UPCXX_DEBUG
        fprintf(stderr, "Rank %u adds handles %p to event %p\n",
                myrank(), h, this);
#endif
    _gasnet_handles.push_back(h);
    _incref();
  }

#ifdef UPCXX_USE_DMAPP
  void event::add_dmapp_handle(dmapp_syncid_handle_t h)
  {
    _dmapp_handles.push_back(h);
    _incref();
  }

#endif

  void event::_add_done_cb(async_task *task)
  {
    assert(_num_done_cb < MAX_NUM_DONE_CB);
    assert(this != system_event);
    _done_cb[_num_done_cb] = task;
    _num_done_cb++;
  }

  upcxx_mutex_t events_stack_lock = UPCXX_MUTEX_INITIALIZER;

  void push_event(event *e)
  {
    upcxx_mutex_lock(&events_stack_lock);
    events->stack.push_back(e);
    upcxx_mutex_unlock(&events_stack_lock);
  }

  void pop_event()
  {
    upcxx_mutex_lock(&events_stack_lock);
    assert(events->stack.back() != system_event);
    events->stack.pop_back();
    upcxx_mutex_unlock(&events_stack_lock);
  }

  event *peek_event()
  {
    return events->stack.back();
  }

  void async_wait()
  {
    while (!outstanding_events->empty()) {
      upcxx::advance(10,10);
    }
    system_event->wait();
    UPCXX_CALL_GASNET(gasnet_wait_syncnbi_all());
  }

  int async_try(event *e)
  {
    // There is at least the default event.
    assert (e != NULL);

    int rv = e->async_try();

    if (e == system_event) {
      int rv1;
      UPCXX_CALL_GASNET(rv1 = gasnet_try_syncnbi_all());
      rv = rv && rv1;
    }
    return rv;
  }

  void event_incref(event *e, uint32_t c)
  {
    assert(e != NULL);
    e->incref(c);
  }

  void event_decref(event *e, uint32_t c)
  {
    assert(e != NULL);
    e->decref(c);
  }

} // namespace upcxx
