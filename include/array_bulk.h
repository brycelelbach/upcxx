#pragma once

#ifdef __cplusplus
extern "C" {
#endif
#include <gasnet.h>
#ifdef __cplusplus
} /* extern "C" */
#endif

/* AM-based scatter-gather primitives (for internal use in array
   library only) */

namespace upcxx {
  /* rectangular scatter/gather */
  void get_array(void *pack_method, void *copy_desc,
                 size_t copy_desc_size, uint32_t tgt_box,
                 void *buffer);
  void put_array(void *unpack_method, void *copy_desc,
                 size_t copy_desc_size, void *array_data,
                 size_t array_data_size, uint32_t tgt_box);

  /* sparse scatter/gather - transfer data back & forth between a
   * local, packed data list and the memory space of a remote proc
   * using a list of addresses on the remote proc
   * requires: sizeof(data_list) == num_elem * elem_sz
   *           sizeof(remote_addr_list) == num_elem * sizeof(void*)
   */
  void sparse_scatter(void **remote_addr_list,
                      void *src_data_list, uint32_t remote_box,
                      size_t num_elem, size_t elem_sz);
  void sparse_gather(void *tgt_data_list,
                     void **remote_addr_list,
                     uint32_t remote_box, size_t num_elem,
                     size_t elem_sz);
} // namespace upcxx
