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
      for (auto it=_h.begin(); it!=_h.end(); ++it) {
#else
      for (std::list<gasnet_handle_t>::iterator it=_h.begin(); it!=_h.end(); ++it) {
#endif
        if (gasnet_try_syncnb(*it) == GASNET_OK) {
          it = _h.erase(it);
          decref();
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

  struct event_stack {
    vector<event *> stack;
    event_stack() {
      stack.push_back(&system_event);
    }
  };

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
