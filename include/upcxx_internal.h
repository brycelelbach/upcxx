#pragma once

/*
 * upcxx_internal.h
 */

namespace upcxx {
  /**
   * \ingroup internal
   * Advance the incoming task queue by processing local tasks
   *
   * Note that some local tasks may take
   *
   * \param max_dispatched the maximum number of tasks to be processed
   *
   * \return the number of tasks that have been processed
   */
  int advance_out_task_queue(queue_t *outq, int max_dispatched);

  inline int advance_out_task(int max_dispatched = MAX_DISPATCHED_OUT)
  {
    return advance_out_task_queue(out_task_queue, max_dispatched);
  }

  /*
   * \ingroup internal
   * Advance the outgoing task queue by sending out remote task requests
   *
   * Note that advance_out_task_queue() shouldn't be be called in
   * any GASNet AM handlers because it calls gasnet_AMPoll() and
   * may result in a deadlock.
   *
   * \param max_dispatched the maximum number of tasks to send
   *
   * \return the number of tasks that have been sent
   */
  int advance_in_task_queue(queue_t *inq, int max_dispatched);

  inline int advance_in_task(int max_dispatched = MAX_DISPATCHED_IN)
  {
    return advance_in_task_queue(in_task_queue, max_dispatched);
  }

  // Internal function for the master node to signal worker nodes to exit
  void signal_exit();
  
  // Internal function for worker nodes to wait for requests from the master node
  void wait_for_incoming_tasks();
  
  // AM handler functions
  void async_am_handler(gasnet_token_t token, void *am, size_t nbytes);
  void async_done_am_handler(gasnet_token_t token, void *am, size_t nbytes);
  void alloc_cpu_am_handler(gasnet_token_t token, void *am, size_t nbytes);
  void alloc_gpu_am_handler(gasnet_token_t token, void *am, size_t nbytes);
  void alloc_reply_handler(gasnet_token_t token, void *reply, size_t nbytes);
  void free_cpu_am_handler(gasnet_token_t token, void *am, size_t nbytes);
  void free_gpu_am_handler(gasnet_token_t token, void *am, size_t nbytes);
  void inc_am_handler(gasnet_token_t token, void *am, size_t nbytes);
}
