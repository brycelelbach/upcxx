#pragma once

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
void get_array(void *pack_method, void *copy_desc,
               size_t copy_desc_size, int tgt_box, void *buffer,
               int atomicelements);
void put_array(void *unpack_method, void *copy_desc,
               size_t copy_desc_size, void *array_data,
               size_t array_data_size, int tgt_box);

/* sparse scatter/gather - transfer data back & forth between a 
 * local, packed data list and the memory space of a remote proc
 * using a list of addresses on the remote proc
 * requires: sizeof(data_list) == num_elem * elem_sz
 *           sizeof(remote_addr_list) == num_elem * sizeof(void*)
 */
void sparse_scatter(void **remote_addr_list, void *src_data_list,
                    int remote_box, size_t num_elem, size_t elem_sz,
                    int atomic_elements);
void sparse_gather(void *tgt_data_list, void **remote_addr_list, 
                   int remote_box, size_t num_elem, size_t elem_sz,
                   int atomic_elements);

/* handler declarations */
  typedef struct {
    void *addr;
  } array_delete_am_t;
  void array_delete_am(gasnet_token_t token, void *ambuf,
                       size_t nbytes);

  typedef struct {
    size_t size; /* size is in bytes. */
    void *destptr;
  } array_alloc_am_t;
  void array_alloc_am(gasnet_token_t token, void *ambuf,
                      size_t nbytes);

  typedef struct {
    void *buf;
    void *destptr;
  } array_alloc_reply_t;
  void array_alloc_reply(gasnet_token_t token, void *ambuf,
                         size_t nbytes);

  typedef struct {
    void *copy_desc;
    size_t copy_desc_size;
    void *_packMethod;
    void *remoteBuffer;
    void *pack_info_ptr;
    int atomicelements;
  } array_strided_pack_am_t;
  void array_strided_pack_am(gasnet_token_t token, void *ambuf,
                             size_t nbytes);

  typedef struct {
    void *array_data;
    size_t array_data_size;
    void *localBuffer;
    void *remoteBuf;
    size_t dataSz;
    void *pack_info_ptr;
  } array_strided_pack_reply_t;
  void array_strided_pack_reply(gasnet_token_t token, void *ambuf,
                                size_t nbytes);

  typedef struct {
    void *packedArrayData;
    size_t nBytes;
    void *_unpackMethod;
    size_t copyDescSize;
    void *unpack_spin_ptr;
  } array_strided_unpackAll_am_t;
  void array_strided_unpackAll_am(gasnet_token_t token, void *ambuf,
                                  size_t nbytes);

  typedef struct {
    void *unpack_spin_ptr;
  } array_strided_unpack_reply_t;
  void array_strided_unpack_reply(gasnet_token_t token, void *ambuf,
                                  size_t nbytes);

  typedef struct {
    void *copyDesc;
    size_t copyDescSize;
    void *bufAddr;
    void *_unpackMethod;
    void *unpack_spin_ptr;
  } array_strided_unpackOnly_am_t;
  void array_strided_unpackOnly_am(gasnet_token_t token, void *ambuf,
                                   size_t nbytes);

MEDIUM_HANDLER_DECL(sparse_simpleScatter_request, 3, 4);
SHORT_HANDLER_DECL(sparse_done_reply, 1, 2);
SHORT_HANDLER_DECL(sparse_generalScatter_request, 4, 6);
MEDIUM_HANDLER_DECL(sparse_simpleGather_request, 5, 7);
MEDIUM_HANDLER_DECL(sparse_simpleGather_reply, 4, 6);
SHORT_HANDLER_DECL(sparse_generalGather_request, 4, 6);

#ifdef __cplusplus
} /* extern "C" */
#endif
