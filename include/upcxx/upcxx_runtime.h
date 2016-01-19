/**
 * UPC++ runtime interface
 */

/**
 * \section intro Introduction
 */

#pragma once

#include "upcxx_types.h"
#include "queue.h"

namespace upcxx
{
  typedef uint32_t rank_t;

  /**
   * \defgroup initgroup Job initialization and query
   * This group of API is for initialization and self-query.
   */

  /**
   * \ingroup initgroup
   * Initialize the GASNet runtime, which should be called
   * only once at the beginning of the client program.
   *
   * \param argc the pointer to the number of arguments
   * \param argv the pointer to the values of the arguments
   *
   * \see hello.cpp
   */
  int init(int *argc=NULL, char ***argv=NULL);

  /**
   * \ingroup initgroup
   * Finalize the GASNet runtime, which should be called only
   * once at the end of the client program.
   */
  int finalize();

  /**
   * \ingroup initgroup
   *
   * @return true if upcxx is initialized or false otherwise
   */
  bool is_init();

  /**
   * \ingroup initgroup
   *
   * @return the total number of ranks in the whole parallel job
   */
  rank_t global_ranks();

  /**
   * \ingroup initgroup
   *
   * @return my rank id in the whole parallel job
   */
  rank_t global_myrank();

  extern queue_t *in_task_queue;

  extern queue_t *out_task_queue;

  /**
   * Default maximum number of task to dispatch for every time
   * the incoming task queue is polled/advanced.
   */
  #define MAX_DISPATCHED_IN   10

  /**
   * Default maximum number of task to dispatch for every time
   * the outgoing task queue is polled/advanced.
   */
  #define MAX_DISPATCHED_OUT  10

  /**
   * \ingroup asyncgroup
   * Advance both the incoming and outgoing task queues
   * Note that advance() shouldn't be be called in any GASNet AM
   * handlers because it calls gasnet_AMPoll() and may result in
   * a deadlock.
   *
   * \param max_in maximum number of incoming tasks to be processed before returning
   * \param max_out maximum number of outgoing tasks to be processed before returning
   *
   * \return the total number of tasks that have been processed in the incoming
   * and outgoing task queues
   */
  int advance(int max_in = MAX_DISPATCHED_IN, int max_out = MAX_DISPATCHED_OUT);

  // An alias of the advance function for backward compatibility.
  // The advance function is recommended.
  inline int progress() { return advance(); }

  // drain() is superseded by advance(), provided for backward compatibility.
  // The advance function is recommended.
  inline int drain(int max_dispatched = 0)
  {
    return advance(max_dispatched, max_dispatched);
  }

  /**
   * \ingroup asyncgroup
   * Poll the task queue, returning 0 if it is empty and 1 otherwise.
   * This function does not modify the task queue. However, like
   * progress(), peek() cannot be called in any GASNet AM handlers
   * because it calls gasnet_AMPoll().
   */
  int peek();

  /**
   * \defgroup syncgroup Synchronization primitives
   * Functions for node synchronization
   */

  /**
   * \ingroup syncgroup
   * Barrier synchronization of all ranks in the parallel job
   */
  int barrier();

  /**
   * \ingroup initgroup
   * Query the estimated maximum size of my global memory partition.
   * This function can be used before init() to help determine
   * the value for request_my_global_memory_size().
   * After init() is called, it's recommended to use
   * my_usable_global_memory_size() to query the actual available global
   * memory size.
   *
   * @return the maximum size of my global memory partition in bytes
   */
  size_t my_max_global_memory_size();

  /**
   * \ingroup initgroup
   * Query the current usable size of my global memory partition.
   * This function should be used after init().
   * Note that the usable size of global memory changes at runtime as
   * the user and the system allocate and deallocate global memory.
   * Due to memory fragmentation, it may not be possible to allocate a
   * contiguous chunk of memory equal to the size of current usable memory.
   *
   * @return the usable size of my global memory partition in bytes
   */
  size_t my_usable_global_memory_size();

  /**
   * \ingroup initgroup
   * Try to request the size of my global memory partition.
   * If this is not set, UPC++ will use the maximum size of global memory
   * supported by the system.
   * request_my_global_memory_size() should be called before init().
   *
   * @param request_size the desirable size of global memory of the calling rank
   * @return the actual global memory size that can be reserved, which may be smaller than the request_size
   */
  size_t request_my_global_memory_size(size_t request_size);

  /**
   * ingroup initgroup
   * Query the global memory size of a specific rank
   *
   * @param rank which rank of the global memory partition to query
   * @return the size of the global memory partition on that rank
   */
  size_t global_memory_size_on_rank(uint32_t rank);

} // namespace upcxx

#if defined(GASNET_PAR)
#define UPCXX_THREAD_SAFE     1
#define upcxx_mutex_lock      gasnet_hsl_lock
#define upcxx_mutex_trylock   gasnet_hsl_trylock
#define upcxx_mutex_unlock    gasnet_hsl_unlock
#define upcxx_mutex_init      gasnet_hsl_init
#define upcxx_mutex_t         gasnet_hsl_t
#define UPCXX_MUTEX_INITIALIZER GASNET_HSL_INITIALIZER

#elif defined(UPCXX_THREAD_SAFE)

#include <pthread.h>
#define upcxx_mutex_lock(X)   pthread_mutex_lock(X)
#define upcxx_mutex_trylock(X) pthread_mutex_trylock(X)
#define upcxx_mutex_unlock(X) pthread_mutex_unlock(X)
#define upcxx_mutex_init(X)   pthread_mutex_init(X, NULL)
#define upcxx_mutex_t         pthread_mutex_t
#define UPCXX_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER

#else // Not thread-safe

#define upcxx_mutex_lock(X)
#define upcxx_mutex_trylock(X) 0
#define upcxx_mutex_unlock(X)
#define upcxx_mutex_init(X)
#define upcxx_mutex_t int
#define UPCXX_MUTEX_INITIALIZER 0

#endif

#if defined(GASNET_PAR)

// GASNET is thread-safe by itself in PAR mode, no need to lock in UPC++ level
#define UPCXX_CALL_GASNET(fncall) fncall

#else

// protect gasnet calls if GASNET_PAR mode is not used
namespace upcxx {
  extern upcxx_mutex_t gasnet_call_lock;
}
#define UPCXX_CALL_GASNET(fncall) do {                                \
    upcxx_mutex_lock(&upcxx::gasnet_call_lock);                       \
    fncall;                                                           \
    upcxx_mutex_unlock(&upcxx::gasnet_call_lock);                     \
  } while(0)

#endif

#define GASNET_BARRIER do {                                           \
    UPCXX_CALL_GASNET(gasnet_barrier_notify(0, GASNET_BARRIERFLAG_UNNAMED));\
    UPCXX_CALL_GASNET(gasnet_barrier_wait(0, GASNET_BARRIERFLAG_UNNAMED));  \
  } while (0)

