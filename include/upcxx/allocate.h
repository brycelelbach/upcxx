/**
 * allocate.h - provide memory allocation service for the GASNet segment
 */

#pragma once

#include "global_ptr.h"

namespace upcxx
{
  /**
   * @ingroup gasgroup
   * @brief allocate memory in the global address space of a rank (process)
   *
   * @return the pointer to the allocated pace
   * @param nbytes the number of element to be copied
   * @param rank the rank_t where the memory space should be allocated
   */
  global_ptr<void> allocate(rank_t rank, size_t nbytes);

  static inline void* allocate(size_t nbytes) 
  {
    return allocate(global_myrank(), nbytes).raw_ptr();
  }

  /**
   * @ingroup gasgroup
   * @brief allocate memory in the global address space at a specific rank_t
   *
   * \tparam T type of the element
   *
   * @return the pointer to the allocated pace
   * @param count the number of element to be copied
   * @param rank the rank_t where the memory space should be allocated
   */
  template<typename T>
  global_ptr<T> allocate(rank_t rank, size_t count)
  {
    size_t nbytes = count * sizeof(T);
    return global_ptr<T>(allocate(rank, nbytes));
  }

  template<typename T>
  T* allocate(size_t count)
  {
    size_t nbytes = count * sizeof(T);
    return (T*)allocate(nbytes);
  }

  void deallocate(global_ptr<void> ptr);

  void deallocate(void *ptr);

  /**
   * @ingroup gasgroup
   * @brief free memory in the global address space
   *
   * \tparam T type of the element
   *
   * @param ptr the pointer to which the memory space should be freed
   */
  template<typename T>
  void deallocate(global_ptr<T> ptr)
  {
    deallocate(global_ptr<void>(ptr));
  }
} // namespace upcxx
