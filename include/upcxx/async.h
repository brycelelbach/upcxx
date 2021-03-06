/**
 * aysnc.h - asynchronous task execution
 */

#pragma once

#include "async_impl.h"

namespace upcxx
{

  /**
   * \ingroup asyncgroup
   *
   * Asynchronous function execution
   * Optionally signal the event "ack" for task completion
   *
   * ~~~~~~~~~~~~~~~{.cpp}
   * async(rank_t rank, event *ack)(function, arg1, arg2, ...);
   * ~~~~~~~~~~~~~~~
   * \see test_async.cpp
   *
   */
  inline gasnet_launcher<rank_t> async(rank_t rank,
                                       event *e = peek_event())
  {
    return gasnet_launcher<rank_t>(rank, e);
  }

  /**
   * \ingroup asyncgroup
   *
   * Asynchronous function execution
   *
   * ~~~~~~~~~~~~~~~{.cpp}
   * async(range ranks)(function, arg1, arg2, ...);
   * ~~~~~~~~~~~~~~~
   * \see test_am_bcast.cpp
   *
   */
  inline gasnet_launcher<range> async(range r,
                                      event *e = peek_event())
  {
    gasnet_launcher<range> launcher(r, e);
    launcher.set_group(group(r.count(), -1));
    return launcher;
  }

  /**
   * \ingroup asyncgroup
   *
   * Conditional asynchronous function execution
   * The task will be automatically enqueued for execution after
   * the event "after" is signaled.
   * Optionally signal the event "ack" for task completion
   *
   * ~~~~~~~~~~~~~~~{.cpp}
   * async_after(uint32_t rank, event *after, event *ack)(function, arg1, arg2, ...);
   * ~~~~~~~~~~~~~~~
   * \see test_async.cpp
   *
   */
  inline gasnet_launcher<rank_t> async_after(rank_t rank, event *after,
                                           event *ack = peek_event())
  {
    return gasnet_launcher<rank_t>(rank, ack, after);
  }

  
  /**
   * \ingroup asyncgroup
   *
   * async for Active Message style communication
   * ~~~~~~~~~~~~~~~{.cpp}
   * async(uint32_t rank, void *src, size_t nbytes, event *ack)
          (function, arg1, arg2, ...);
   * ~~~~~~~~~~~~~~~
   * \see test_async_am.cpp
   *
   */
  inline gasnet_launcher<rank_t> async(uint32_t rank,
                                       void *am_dst,
                                       void *am_src,
                                       size_t nbytes,
                                       event *ack)
  {
    return gasnet_launcher<rank_t>(rank, ack, NULL);
  }

  template<>
  void gasnet_launcher<rank_t>::launch(generic_fp fp,
                                       void *async_args,
                                       size_t arg_sz);

  template<>
  void gasnet_launcher<range>::launch(generic_fp fp,
                                      void *async_args,
                                      size_t arg_sz);

} // namespace upcxx
