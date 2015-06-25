/**
 * UPCXX events
 */

#include <stdio.h>
#include <assert.h>
#include <vector>

#define UPCXX_NO_INITIALIZER_OBJECT

#include "upcxx.h"

// #define DEBUG

using namespace std;

namespace upcxx
{
  int event::async_try()
  {
    if (upcxx_mutex_trylock(&_mutex) != 0) {
      return 0; // somebody else is holding the lock, so return not done yet
    }

    // Acquired the lock

    // check outstanding gasnet handles
    if (!_gasnet_handles.empty()) {
     // for (auto it = _h.begin(); it != _h.end()) { // UPCXX_HAVE_CXX11
     for (std::vector<gasnet_handle_t>::iterator it = _gasnet_handles.begin();
           it != _gasnet_handles.end();) {
        if (gasnet_try_syncnb(*it) == GASNET_OK) {
          _gasnet_handles.erase(it); // erase increase it automatically
          decref();
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
          decref();
        } else {
          ++it;
        }
      }
    }
#endif

    upcxx_mutex_unlock(&_mutex); // trylock must be successful to get here
    return isdone();
  }

  void event::wait()
  {
    while (!async_try()) {
      advance(1, 10);
      // YZ: don't need to yield if CPUs are not over-subscribed.
      // gasnett_sched_yield();
    }
  }

  void event::enqueue_cb()
  {
    assert (_count == 0);

    upcxx_mutex_lock(&_mutex);
    // add done_cb to the task queue
    if (_num_done_cb > 0) {
      for (int i=0; i<_num_done_cb; i++) {
        if (_done_cb[i] != NULL) {
          async_task *task = _done_cb[i];
          if (task->_callee == global_myrank()) {
            // local task
            assert(in_task_queue != NULL);
            gasnet_hsl_lock(&in_task_queue_lock);
            queue_enqueue(in_task_queue, task);
            gasnet_hsl_unlock(&in_task_queue_lock);
          } else {
            // remote task
            assert(out_task_queue != NULL);
            gasnet_hsl_lock(&out_task_queue_lock);
            queue_enqueue(out_task_queue, task);
            gasnet_hsl_unlock(&out_task_queue_lock);
          }
        }
      }
      _num_done_cb = 0;
    }
    upcxx_mutex_unlock(&_mutex);
  }

  // Increment the reference counter for the event
  void event::incref(uint32_t c)
  {
    upcxx_mutex_lock(&_mutex);
    int old = _count;
    _count += c;
    if (this != system_event && old == 0) {
      gasnet_hsl_lock(&outstanding_events_lock);
      // fprintf(stderr, "P %u   Add outstanding_event %p\n", global_myrank(), this);
      outstanding_events->push_back(this);
      gasnet_hsl_unlock(&outstanding_events_lock);
    }
    upcxx_mutex_unlock(&_mutex);
  }

  // Decrement the reference counter for the event
  void event::decref()
  {
    upcxx_mutex_lock(&_mutex);
    if (_count == 0) {
      fprintf(stderr,
              "Fatal error: attempt to decrement an event (%p) with 0 references!\n",
              this);
      gasnet_exit(1);
    }

    int tmp = --_count; // obtain the value of count before unlock
    upcxx_mutex_unlock(&_mutex);

    if (tmp == 0) {
      enqueue_cb();
      if (this != system_event) {
        gasnet_hsl_lock(&outstanding_events_lock);
        // fprintf(stderr, "P %u Erase outstanding_event %p\n", global_myrank(), this);
        outstanding_events->remove(this);
        gasnet_hsl_unlock(&outstanding_events_lock);
      }
    }
  }

  event_stack *events = NULL;

  void event::add_gasnet_handle(gasnet_handle_t h)
  {
    upcxx_mutex_lock(&_mutex);
    _gasnet_handles.push_back(h);
    upcxx_mutex_unlock(&_mutex);
    incref();
  }

#ifdef UPCXX_USE_DMAPP
  void event::add_dmapp_handle(dmapp_syncid_handle_t h)
  {
    upcxx_mutex_lock(&_mutex);
    _dmapp_handles.push_back(h);
    upcxx_mutex_unlock(&_mutex);
    incref();
  }

#endif

  void push_event(event *e)
  {
    events->stack.push_back(e);
  }

  void pop_event()
  {
    assert(events->stack.back() != system_event);
    events->stack.pop_back();
  }

  event *peek_event()
  {
    return events->stack.back();
  }

  void async_wait()
  {
    while (!outstanding_events->empty()) {
      upcxx::advance(1,10);
    }
    system_event->wait();
    gasnet_wait_syncnbi_all();
  }

  int async_try(event *e)
  {
    // There is at least the default event.
    assert (e != NULL);

    int rv = e->async_try();

    if (e == system_event) {
      rv = rv && gasnet_try_syncnbi_all();
    }
    return rv;
  }

} // namespace upcxx
