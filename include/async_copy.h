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

  // YZ: Deprecated, should use async_wait() instead
  inline void async_copy_fence()
  {
    gasnet_wait_syncnbi_all();
  }

  // YZ: Deprecated, should async_try() instead
  inline int async_copy_try()
  {
    return gasnet_try_syncnbi_all();
  }
} // namespace upcxx
