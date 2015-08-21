/**
 * Implement data transfer functions: copy, async_copy, async_copy_and_signal
 */

#pragma once

#include <iostream>

#include "global_ptr.h"

namespace upcxx
{
  /**
   * \ingroup gasgroup
   * \brief transfer data between processes
   *
   * \tparam T type of the element
   * \param src the pointer of src data
   * \param dst the pointer of dst data
   * \param nbytes the number of bytes to be transferred
   *
   */
  int copy(global_ptr<void> src, global_ptr<void> dst, size_t nbytes);

  /**
   * \ingroup gasgroup
   * \brief transfer data between processes
   *
   * \tparam T type of the element
   * \param src the pointer of src data
   * \param dst the pointer of dst data
   * \param count the number of element to be transferred
   *
   * \see test_global_ptr.cpp
   */
  template<typename T>
  int copy(global_ptr<T> src, global_ptr<T> dst, size_t count)
  {
    size_t nbytes = count * sizeof(T);
    return copy((global_ptr<void>)src, (global_ptr<void>)dst, nbytes);
  }

  template<typename T>
  inline int copy(global_ptr<T> src, T* dst, size_t count)
  {
    return copy(src, global_ptr<T>(dst), count);
  }

  template<typename T>
  inline int copy(T* src, global_ptr<T> dst, size_t count)
  {
    return copy(global_ptr<T>(src), dst, count);
  }

  int async_copy(global_ptr<void> src,
                 global_ptr<void> dst,
                 size_t bytes,
                 event *done_event = peek_event());

  /**
   * \ingroup gasgroup
   * \brief Non-blocking transfer data between processes
   *
   * \tparam T type of the element
   * \param src the pointer of src data
   * \param dst the pointer of dst data
   * \param count the number of element to be copied
   * \param done_event the event to be signaled after transfer completion 
   */
  template<typename T>
  int async_copy(global_ptr<T> src,
                 global_ptr<T> dst,
                 size_t count,
                 event *done_event = peek_event())
  {
    size_t nbytes = count * sizeof(T);
    return async_copy((global_ptr<void>)src,
                      (global_ptr<void>)dst,
                      nbytes,
                      done_event);
  }

  template<typename T>
  inline int async_copy(global_ptr<T> src, T* dst, size_t count,
                        event *done_event = peek_event())
  {
    return async_copy(src, global_ptr<T>(dst), count, done_event);
  }

  template<typename T>
  inline int async_copy(T* src, global_ptr<T> dst, size_t count,
                        event *done_event = peek_event())
  {
    return async_copy(global_ptr<T>(src), dst, count, done_event);
  }

  /**
   * \ingroup gasgroup
   * \brief Non-blocking signaling copy, which first performs an async copy
   * and then signal an event on the destination rank
   *
   * The remote rank can wait on the event to check if the corresponding
   * async_copy data have arrived.
   *
   * \tparam T type of the element
   * \param src the pointer of src data
   * \param dst the pointer of dst data
   * \param nbytes the number of bytes to be transferred
   * \param signal_event the event to be signaled on the dst rank after transfer
   * \param local_completion sender event when the local buffer can be reused
   * \param remote_completion sender event when the dst buffer is written
   */
  int async_copy_and_signal(global_ptr<void> src,
                            global_ptr<void> dst,
                            size_t nbytes,
                            event *signal_event,
                            event *local_completion,
                            event *remote_completion = peek_event());

  /**
   * \ingroup gasgroup
   * \brief Non-blocking signaling copy, which first performs an async copy
   * and then signal an event on the destination rank
   *
   * The remote rank can wait on the event to check if the corresponding
   * async_copy data have arrived.
   *
   * The remote rank can wait on the flag to check if the corresponding
   * async_copy data have arrived.
   *
   * \tparam T type of the element
   * \param src the pointer of src data
   * \param dst the pointer of dst data
   * \param count the number of element to be transferred
   * \param signal_event the event to be signaled on the dst rank
   * \param done_event the event to be signaled after copy completion
   */
  template<typename T>
  int async_copy_and_signal(global_ptr<T> src,
                            global_ptr<T> dst,
                            size_t count,
                            event *signal_event,
                            event *local_completion,
                            event *remote_completion = peek_event())
  {
    size_t nbytes = count * sizeof(T);
    return async_copy_and_signal((global_ptr<void>)src,
                                 (global_ptr<void>)dst,
                                 nbytes,
                                 signal_event,
                                 local_completion,
                                 remote_completion);
  }

  /**
   * async_copy_fence is deprecated. Please use async_wait() instead.
   */
  void async_copy_fence();

  /**
   * async_copy_try is deprecated. Please async_try() instead.
   */
  int async_copy_try();

} // namespace upcxx
