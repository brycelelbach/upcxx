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
    const int poll_times = 1;

    // Don't poll the network if the event is already done
    if (isdone()) {
      run_cb();
      return 1;
    }

    for (int i=0; i<poll_times; i++) {
      gasnet_AMPoll();
      progress();
    }

    if (_h_flag) {
      if (gasnet_try_syncnb(_h) == GASNET_OK)
        decref();
    }

    return isdone();
  }

  void event::wait()
  {
    while (!test()) {
      gasnett_sched_yield();
    }
  }

  struct event_stack {
    vector<event *> stack;
    event_stack() {
      stack.push_back(&default_event);
    }
  };

#if __cplusplus < 201103L
  static event_stack events;
#else
  static thread_local event_stack events;
#endif

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
