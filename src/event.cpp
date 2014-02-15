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
  int event::test()
  {
    if (_h_flag) {
      if (gasnet_try_syncnb(_h) == GASNET_OK) {
        _h_flag = 0;
        decref();
      }
    }

    return isdone();
  }

  void event::wait()
  {
    while (!test()) {
      advance(1, 10);
      gasnett_sched_yield();
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
          if (task->_callee == my_node.id()) {
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
      stack.push_back(&default_event);
    }
  };

  static event_stack events;

  void push_event(event *e)
  {
    events.stack.push_back(e);
  }

  void pop_event()
  {
    events.stack.pop_back();
  }

  event *peek_event()
  {
    return events.stack.back();
  }
} // namespace upcxx
