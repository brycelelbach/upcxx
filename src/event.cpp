/**
 * UPCXX events
 */

#include <stdio.h>
#include <assert.h>
#include <vector>

#include "upcxx.h"

// #define DEBUG

using namespace std;

namespace upcxx
{
  int event::async_try()
  {
    if (!_h.empty()) {
#if UPCXX_HAVE_CXX11
      for (auto it = _h.begin(); it != _h.end(); ++it) {
#else
        for (std::list<gasnet_handle_t>::iterator it = _h.begin(); it != _h.end(); ++it) {
#endif
          if (gasnet_try_syncnb(*it) == GASNET_OK) {
            remove_handle(*it);
            break;
          }
        }
      }
      return isdone();
    }

    void event::wait()
    {
      while (!test()) {
        advance(1, 10);
        // YZ: don't need to yield if CPUs are not over-subscribed.
        // gasnett_sched_yield();
      }
    }

    void event::enqueue_cb()
    {
      assert (_count == 0);

      gasnet_hsl_lock(&_lock);
      // add done_cb to the task queue
      if (_num_done_cb > 0) {
        for (int i=0; i<_num_done_cb; i++) {
          if (_done_cb[i] != NULL) {
            async_task *task = _done_cb[i];
            if (task->_callee == myrank()) {
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
      gasnet_hsl_unlock(&_lock);
    }

    // Increment the reference counter for the event
    void event::incref(uint32_t c)
    {
      gasnet_hsl_lock(&_lock);
      int old = _count;
      _count += c;
      if (this != &system_event && old == 0) {
        gasnet_hsl_lock(&outstanding_events_lock);
        // fprintf(stderr, "P %u   Add outstanding_event %p\n", myrank(), this);
        outstanding_events.push_back(this);
        gasnet_hsl_unlock(&out_task_queue_lock);
      }
      gasnet_hsl_unlock(&_lock);
    }

    // Decrement the reference counter for the event
    void event::decref()
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
        if (this != &system_event) {
          gasnet_hsl_lock(&outstanding_events_lock);
          // fprintf(stderr, "P %u Erase outstanding_event %p\n", myrank(), this);
          outstanding_events.remove(this);
          gasnet_hsl_unlock(&outstanding_events_lock);
        }
      }
    }

    struct event_stack {
      vector<event *> stack;
      event_stack() {
        stack.push_back(&system_event);
      }
    };

    void event::add_handle(gasnet_handle_t h)
    {
      gasnet_hsl_lock(_lock);
      _h.push_back(h);
      gasnet_hsl_unlock(_lock);
      incref();
    }

    void event::remove_handle(gasnet_handle_t h)
    {
      gasnet_hsl_lock(_lock);
      _h.remove(h);
      gasnet_hsl_unlock(_lock);
      decref();
    }

    static event_stack events;

    void push_event(event *e)
    {
      events.stack.push_back(e);
    }

    void pop_event()
    {
      assert(events.stack.back() != &system_event);
      events.stack.pop_back();
    }

    event *peek_event()
    {
      return events.stack.back();
    }
  } // namespace upcxx
