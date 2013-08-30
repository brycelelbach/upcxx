/*
 * upcxx_internal.h
 */

#ifndef UPCXX_INTERNAL_H_
#define UPCXX_INTERNAL_H_

namespace upcxx {
  
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
}

#endif /* UPCXX_INTERNAL_H_ */
