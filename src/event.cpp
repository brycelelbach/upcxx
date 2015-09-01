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
#ifdef UPCXX_DEBUG1
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
        if (gasnet_try_syncnb_nopoll(*it) == GASNET_OK) {
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

  // Increment the reference counter for the event
  void event::_incref(uint32_t c)
  {
    uint32_t old = _count;
    _count += c;
#ifdef UPCXX_DEBUG
    fprintf(stderr, "P %u incref event %p, c = %d, old %u\n", global_myrank(), this, c, old);
#endif

    if (this != system_event && old == 0) {

#ifdef UPCXX_DEBUG
      fprintf(stderr, "P %u Add outstanding_event %p\n", global_myrank(), this);
#endif
      outstanding_events->push_back(this);
    }
  }

  // Decrement the reference counter for the event
  void event::_decref(uint32_t c)
  {
    _count -= c;
    if (_count < 0) {
      fprintf(stderr,
              "Fatal error: attempt to decrement an event (%p) to be a negative number of references!\n",
              this);
      gasnet_exit(1);
    }
    int tmp = _count; // obtain the value of count before unlock

    if (tmp == 0) {
      _enqueue_cb();
#ifdef UPCXX_DEBUG
        fprintf(stderr, "P %u Erase outstanding_event %p\n", global_myrank(), this);
#endif
        outstanding_events->remove(this);
    }
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
