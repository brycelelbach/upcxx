/**
 * UPCXX events
 */

#include <stdio.h>
#include <assert.h>

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
} // namespace upcxx
