#ifndef _include_newbulk_h_
#define _include_newbulk_h_

#ifdef __cplusplus
extern "C" {
#endif

#include <gasnet.h>
#include <gasnet_handler.h>

/*
  initialize prealloc and pipelining by reading console environment variables
*/
void gather_init();

/* AM-based scatter-gather primitives (for internal use in ti_array.c only) */

/* rectangular scatter/gather */
void get_array(void *pack_method, void *copy_desc, int copy_desc_size, 
               int tgt_box, void *buffer, int atomicelements);
void put_array(void *unpack_method, void *copy_desc, int copy_desc_size,
               void *array_data, size_t array_data_size, int tgt_box);

/* sparse scatter/gather - transfer data back & forth between a 
 * local, packed data list and the memory space of a remote proc
 * using a list of addresses on the remote proc
 * requires: sizeof(data_list) == num_elem * elem_sz
 *           sizeof(remote_addr_list) == num_elem * sizeof(void*)
 */
void sparse_scatter(void **remote_addr_list, void *src_data_list, 
                    int remote_box, size_t num_elem, size_t elem_sz, int atomic_elements);
void sparse_gather(void *tgt_data_list, void **remote_addr_list, 
                   int remote_box, size_t num_elem, size_t elem_sz, int atomic_elements);


/* handler declarations */

SHORT_HANDLER_DECL(misc_null_reply, 0, 0);
SHORT_HANDLER_DECL(misc_delete_request, 1, 2);
SHORT_HANDLER_DECL(misc_alloc_request, 2, 3);
SHORT_HANDLER_DECL(misc_alloc_reply, 2, 4);

MEDIUM_HANDLER_DECL(strided_pack_request, 4, 7);
MEDIUM_HANDLER_DECL(strided_pack_reply, 4, 7);
MEDIUM_HANDLER_DECL(strided_unpackAll_request, 3, 5);
SHORT_HANDLER_DECL(strided_unpack_reply, 1, 2);
MEDIUM_HANDLER_DECL(strided_unpackOnly_request, 3, 6);

MEDIUM_HANDLER_DECL(sparse_simpleScatter_request, 3, 4);
SHORT_HANDLER_DECL(sparse_done_reply, 1, 2);
SHORT_HANDLER_DECL(sparse_generalScatter_request, 4, 6);
MEDIUM_HANDLER_DECL(sparse_simpleGather_request, 5, 7);
MEDIUM_HANDLER_DECL(sparse_simpleGather_reply, 4, 6);
SHORT_HANDLER_DECL(sparse_generalGather_request, 4, 6);
LONG_HANDLER_DECL(sparse_largeGather_request, 5, 7);
LONG_HANDLER_DECL(sparse_largeScatterNoDelete_request, 3, 4);
LONG_HANDLER_DECL(sparse_largeGather_reply, 1, 2);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* !_include_newbulk_h_ */




