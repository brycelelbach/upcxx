/**
 * Multi-node global pointers and memory allocation and copy functions
 */

#pragma once

#include <iostream>

#include "global_ptr.h"

using namespace std;

namespace upcxx
{

  // **************************************************************************
  // Implement copy, allocate and free with ptr for GASNet backend
  // **************************************************************************

  int copy(global_ptr<void> src, global_ptr<void> dst, size_t nbytes);

  /**
   * \ingroup gasgroup
   * \brief copy data from cpu to cpu
   *
   * \tparam T type of the element
   * \param src the pointer of src data
   * \param dst the pointer of dst data
   * \param count the number of element to be copied
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
                 event *e = peek_event());

  inline void sync_nb(gasnet_handle_t h)
  {
    gasnet_wait_syncnb(h);
  }

  /**
   * \ingroup gasgroup
   * \brief Non-blocking copy data from cpu to cpu
   *
   * \tparam T type of the element
   * \param src the pointer of src data
   * \param dst the pointer of dst data
   * \param count the number of element to be copied
   * \param e the event which should be notified after the completion of the copy
   */
  template<typename T>
  int async_copy(global_ptr<T> src,
                 global_ptr<T> dst,
                 size_t count,
                 event *e = peek_event())
  {
    size_t nbytes = count * sizeof(T);
    return async_copy((global_ptr<void>)src,
                      (global_ptr<void>)dst,
                      nbytes,
                      e);
  }

  /**
   * \ingroup gasgroup
   * \brief Non-blocking copy-and-set, which first performs an async copy
   * and after the data transfer completes it sets a flag on remote rank
   *
   * The remote rank can wait on the flag to check if the corresponding
   * async_copy data have arrived.
   *
   * \param src the pointer of src data
   * \param dst the pointer of dst data
   * \param nbytes the number of bytes to be copied
   * \param flag_addr the pointer to the remote flag
   * \param flag_val the value to set in the remote flag
   * \param e the local event which should be notified after the completion of the copy
   */
  int async_copy_and_set(global_ptr<void> src,
                         global_ptr<void> dst,
                         size_t nbytes,
                         global_ptr<flag_t> flag_addr,
                         event *e = peek_event());

  /**
    * \ingroup gasgroup
    * \brief Non-blocking copy-and-set, which first performs an async copy
    * and after the data transfer completes it sets a flag on remote rank
    *
    * The remote rank can wait on the flag to check if the corresponding
    * async_copy data have arrived.
    *
    * \tparam T type of the element
    * \param src the pointer of src data
    * \param dst the pointer of dst data
    * \param count the number of element to be copied
    * \param flag_addr the pointer to the remote flag
    * \param e the event which should be notified after the completion of the copy
    */
   template<typename T>
   int async_copy_and_set(global_ptr<T> src,
                          global_ptr<T> dst,
                          size_t count,
                          global_ptr<flag_t> flag_addr,
                          event *e = peek_event())
   {
     size_t nbytes = count * sizeof(T);
     return async_copy_and_set((global_ptr<void>)src,
                               (global_ptr<void>)dst,
                               nbytes,
                               flag_addr,
                               e);
   }

   void async_set_flag(global_ptr<flag_t> flag_addr, event *e = NULL);

   void async_unset_flag(global_ptr<flag_t> flag_addr, event *e = NULL);

  /**
   * async_copy_fence is deprecated. Please use async_wait() instead.
   */
  void async_copy_fence();

  /**
   * async_copy_try is deprecated. Please async_try() instead.
   */
  int async_copy_try();

} // namespace upcxx
