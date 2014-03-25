#include <stdlib.h>
#include <assert.h>
#include <array_bulk.h>
#include "gasnet_api.h"
#include <portable_inttypes.h>
#include "allocate.h"

/* No multithreading support for now */
#define BOXES       gasnet_nodes() /* TODO: use UPC++ global_machine.node_count() */
#define MYBOXPROCS  1              /* TODO: use UPC++ my_node.processor_count() */
#define MYBOXPROC   0              /* TODO: use UPC++ my_processor.local_id() */

#define SUNPACK(a0) ((size_t)UNPACK(a0))
#define SUNPACK2(a0, a1) ((size_t)UNPACK2(a0, a1))

#define upcxxa_malloc_handlersafe(s) gasnet_seg_alloc(s)
#define upcxxa_free_handlersafe(p)   gasnet_seg_free(p)
#define upcxxa_malloc_atomic_uncollectable(s) gasnet_seg_alloc(s)
#define upcxxa_malloc_atomic_huge(s) gasnet_seg_alloc(s)
#define upcxxa_free(p) gasnet_seg_free(p)

/* look into how to sync puts in UPC++ */
#define upcxxa_write_sync()

/* FIX THIS VVV */
#define UPCXXA_TRANSLATE_FUNCTION_ADDR(localaddr, targetbox) \
  (localaddr)

/**************************
 * INITIALIZATION/STATICS *
 *************************/

/*
  preallocate buffer size in bytes
*/
static int upcxxa_prealloc;

/*
  pipelining is 1 if using pipelining
*/
static int upcxxa_pipelining;

/*
   struct definition to hold information about each remote buffer
*/
struct Buffer{
  void *ptr;
  int size;
};

/*
  holds information about each remote buffer
  each thread has its own array
  array has the same length as the number of nodes
*/
static struct Buffer **bufferArray;

/*
  initialize data structure for buffer reuse
  assumes only one thread in the box will run this code
  so it does not acquire any locks
*/
void buffer_init(){
  int i, j;
  bufferArray = (struct Buffer **)
    upcxxa_malloc_atomic_uncollectable(sizeof(struct Buffer *)*
                                       MYBOXPROCS);
  if (!bufferArray) {
    fprintf(stderr, "failed to allocate %zu bytes in %s\n",
            (sizeof(struct Buffer *)*MYBOXPROCS),
            "array bulk initialization");
    abort();
  }

  for (i = 0; i < MYBOXPROCS; i++){
    struct Buffer *tmp;
    bufferArray[i] = (struct Buffer *)
      upcxxa_malloc_atomic_uncollectable(sizeof(struct Buffer)*
                                         BOXES);
    tmp = bufferArray[i];
    if (!tmp) {
      fprintf(stderr, "failed to allocate %zu bytes in %s\n",
              (sizeof(struct Buffer *)*BOXES),
              "array bulk initialization");
      abort();
    }
    for (j = 0; j < BOXES; j++){
	tmp->size = -1;
	tmp->ptr = NULL;
	tmp++;
    }
  }
}

/*
  in the case of preallocate buffer, return a pointer to the buffer of
  size datasz or greater for node remote_box
*/
void *get_remote_buffer(size_t datasz, uint32_t remote_box){
  void * volatile remoteAllocBuf = NULL;
  struct Buffer *myBuffers = bufferArray[MYBOXPROC];
  struct Buffer *buffer = myBuffers+remote_box;

  /* Allocate a buffer to hold the array data on the remote sideif
     there is no preallocated buffer of this size or greater. */
  if (buffer->size >= datasz){
      remoteAllocBuf = buffer->ptr;
  }
  else if (buffer->size == -1){
    SHORT_REQ(2,3,(remote_box,
                   gasneti_handleridx(misc_alloc_request),
                   datasz, PACK(&remoteAllocBuf)));
    GASNET_BLOCKUNTIL(remoteAllocBuf);
    assert((((uintptr_t)remoteAllocBuf) % sizeof(void *) == 0));
    buffer->size = datasz;
    buffer->ptr = remoteAllocBuf;
  }
  else{
    /* first delete old buffer that is too small */
    SHORT_REQ(1,2,(remote_box,
                   gasneti_handleridx(misc_delete_request),
                   PACK(buffer->ptr)));
    /* allocate new buffer with size datasz */
    SHORT_REQ(2,4,(remote_box, gasneti_handleridx(misc_alloc_request),
                   PACK(datasz), PACK(&remoteAllocBuf)));
    GASNET_BLOCKUNTIL(remoteAllocBuf);
    assert((((uintptr_t)remoteAllocBuf) % sizeof(void *) == 0));
    buffer->size = datasz;
    buffer->ptr = remoteAllocBuf;
  }
  return remoteAllocBuf;
}

typedef void *(*pack_method_t)(void *, size_t *, void *);
typedef void *(*unpack_method_t)(void *, void *);

/* ------------------------------------------------------------------------------------ */
/***************************
 * Miscellaneous handlers. *
 ***************************/

GASNETT_INLINE(misc_delete_request_inner)
void misc_delete_request_inner(gasnet_token_t token, void *addr) {
  upcxxa_free_handlersafe(addr);
}
SHORT_HANDLER(misc_delete_request, 1, 2,
              (token, UNPACK(a0)     ),
              (token, UNPACK2(a0, a1)));
/* ------------------------------------------------------------------------------------ */
/* Size is in bytes. */
GASNETT_INLINE(misc_alloc_request_inner)
void misc_alloc_request_inner(gasnet_token_t token, size_t size,
                              void *destptr) {
  void *buf = upcxxa_malloc_handlersafe(size);
  if (!buf) {
    fprintf(stderr, "failed to allocate %zu bytes in %s\n",
            size, "array alloc AM handler");
    abort();
  }
  SHORT_REP(2,4,(token, gasneti_handleridx(misc_alloc_reply),
                 PACK(buf), PACK(destptr)));
}
SHORT_HANDLER(misc_alloc_request, 2, 4,
              (token, SUNPACK(a0),      UNPACK(a1)     ),
              (token, SUNPACK2(a0, a1), UNPACK2(a2, a3)));
/* ------------------------------------------------------------------------------------ */
GASNETT_INLINE(misc_alloc_reply_inner)
void misc_alloc_reply_inner(gasnet_token_t token, void *buf,
                            void *destptr) {
  void ** premoteAllocBuf = (void **)destptr;
  (*premoteAllocBuf) = (void *)buf;
}
SHORT_HANDLER(misc_alloc_reply, 2, 4,
              (token, UNPACK(a0),      UNPACK(a1)     ),
              (token, UNPACK2(a0, a1), UNPACK2(a2, a3)));
/* ------------------------------------------------------------------------------------ *
 *  Rectangular strided copy
 * ------------------------------------------------------------------------------------ */
/************************************
 * Get an array from a remote node. *
 ***********************************/

/* Call _packMethod to pack the array with copy_desc as its "copy
   descriptor", returning the packed data to remote address
   remoteBuffer. */
GASNETT_INLINE(strided_pack_request_inner)
void strided_pack_request_inner(gasnet_token_t token, void *copy_desc,
                                size_t copy_desc_size,
                                void *_packMethod, void *remoteBuffer,
                                void *pack_info_ptr,
                                int atomicelements) {
  size_t array_data_len;
  void *array_data;
  pack_method_t packMethod = (pack_method_t)_packMethod;
  /* A NULL buffer pointer tells the packer to allocate its own
   * buffer. (void*)-1 tells it to use handlersafe, uncollectable
   * memory so it doesn't disappear on us
   */
  array_data = (*packMethod)(copy_desc, &array_data_len, (void*)-1);

  if (array_data_len <= gasnet_AMMaxMedium()) {
    /* Common, fast case. Send back the packed data with the reply. */
    MEDIUM_REP(4,7,(token, gasneti_handleridx(strided_pack_reply),
                    array_data, array_data_len, PACK(remoteBuffer),
                    PACK(0), 0, PACK(pack_info_ptr)));
    upcxxa_free_handlersafe(array_data);
  }
  else {
#if 0
    gasnett_local_wmb(); /* ensure data is committed before signalling initiator to pull data */
#endif
    /* Slower case. Why doesn't the darned AM_ReplyXfer() work? */
    MEDIUM_REP(4,7,(token, gasneti_handleridx(strided_pack_reply),
                    array_data, gasnet_AMMaxMedium(),
                    PACK(remoteBuffer), PACK(array_data),
                    array_data_len, PACK(pack_info_ptr)));
  }
}
MEDIUM_HANDLER(strided_pack_request, 4, 7,
               (token,addr,nbytes, UNPACK(a0),      UNPACK(a1),
                UNPACK(a2),      a3),
               (token,addr,nbytes, UNPACK2(a0, a1), UNPACK2(a2, a3),
                UNPACK2(a4, a5), a6));

/* ------------------------------------------------------------------------------------ */
/* The reply handler needs to pass this information back to the
   caller. */
typedef struct {
  volatile int pack_spin; /* 0 while waiting, 1 if completed, 2 if using fragmentation */
  void * volatile remoteBuffer;
  volatile size_t dataSize;
} pack_return_info_t;

/* Sets the spin flag to 1 if the array fit in the message reply and
   we're done; otherwise it sets the spinlock variable to be the
   remote address of the packed data, which must be sent over with a
   libtic bulk transfer. */
GASNETT_INLINE(strided_pack_reply_inner)
void strided_pack_reply_inner(gasnet_token_t token, void *array_data,
                              size_t array_data_size,
                              void *localBuffer, void *remoteBuf,
                              int dataSz, void *pack_info_ptr) {
  int moreWaiting = (remoteBuf != 0 && dataSz != 0);
  pack_return_info_t* pinfo = (pack_return_info_t*)pack_info_ptr;
  memcpy((void *)localBuffer, array_data, array_data_size);
  if (!moreWaiting) { /* got it all in a single chunk */
    gasnett_local_wmb(); /* ensure data is committed before signalling */
    pinfo->pack_spin = 1;
  }
  else { /* need more chunks */
    assert(array_data_size == gasnet_AMMaxMedium()); /* first chunk always max medium in size */
    pinfo->remoteBuffer = (uint8_t *)remoteBuf;
    pinfo->dataSize = dataSz;
    gasnett_local_wmb(); /* ensure data is committed before signalling */
    pinfo->pack_spin = 2;
  }
}
MEDIUM_HANDLER(strided_pack_reply, 4, 7,
               (token,addr,nbytes, UNPACK(a0),      UNPACK(a1),
                a2, UNPACK(a3)     ),
               (token,addr,nbytes, UNPACK2(a0, a1), UNPACK2(a2, a3),
                a4, UNPACK2(a5, a6)));
/* ------------------------------------------------------------------------------------ */
extern void get_array(void *pack_method, void *copy_desc,
                      size_t copy_desc_size, uint32_t tgt_box,
                      void *buffer, int atomicelements) {
  pack_return_info_t info;
  info.pack_spin = 0;
  /* A copy_desc is an object that contains the array descriptor. */
  assert(copy_desc && copy_desc_size > 0 &&
         copy_desc_size <= gasnet_AMMaxMedium());
  MEDIUM_REQ(4,7,(tgt_box, gasneti_handleridx(strided_pack_request),
                  copy_desc, copy_desc_size,
                  PACK(UPCXXA_TRANSLATE_FUNCTION_ADDR(pack_method,
                                                      tgt_box)),
                  PACK(buffer),
                  PACK(&info), atomicelements));
  GASNET_BLOCKUNTIL(info.pack_spin);
  /* If the data was too big to be sent in the reply, use a Tic call
     to do the job. */
  if (info.pack_spin != 1) {
    /* pull the data to use using a bulk copy (first fragment has
       already been copied) */
    gasnet_get_bulk((void *)((uintptr_t)buffer+gasnet_AMMaxMedium()),
                    tgt_box, ((uint8_t *)info.remoteBuffer) +
                    gasnet_AMMaxMedium(),
                    info.dataSize - gasnet_AMMaxMedium());
    /* now tell remote to delete temp space */
    SHORT_REQ(1,2,(tgt_box, gasneti_handleridx(misc_delete_request),
                   PACK(info.remoteBuffer)));
  }
}

/* ------------------------------------------------------------------------------------ */
/************************************
 * Write an array to a remote node. *
 ***********************************/

/* The fastest way of doing things....take the data given and unpack
   it. */
GASNETT_INLINE(strided_unpackAll_request_inner)
void strided_unpackAll_request_inner(gasnet_token_t token,
                                     void *packedArrayData,
                                     size_t nBytes,
                                     void *_unpackMethod,
                                     int copyDescSize,
                                     void *unpack_spin_ptr) {
  void *temp = NULL;
  if (((uintptr_t)packedArrayData) % 8 != 0) {
   /* it seems that some crappy AM implementations (ahem.. the NOW)
    * fail to double-word align the medium-sized message buffer
    * fix that problem if it occurs
    */
    temp = upcxxa_malloc_handlersafe(nBytes+8);
    if (!temp) {
      fprintf(stderr, "failed to allocate %zu bytes in %s\n",
              (nBytes+8), "array unpack AM handler");
      abort();
    }
    memcpy(temp, packedArrayData, nBytes);
    packedArrayData = temp;
  }
  {
    unpack_method_t unpackMethod = (unpack_method_t)_unpackMethod;
    void *copyDesc = packedArrayData;
    void *arrayData =
      (void *)((uintptr_t)packedArrayData + copyDescSize);
    assert(((uintptr_t)copyDesc) % 8 == 0 &&
           ((uintptr_t)arrayData) % 8 == 0);
    (*unpackMethod)(copyDesc, arrayData);
#if 0
    gasnett_local_wmb(); /* ensure data is committed before signalling completion */
#endif
    SHORT_REP(1,2,(token, gasneti_handleridx(strided_unpack_reply),
                   PACK(unpack_spin_ptr)));
  }
  if (temp) upcxxa_free_handlersafe(temp);
}
MEDIUM_HANDLER(strided_unpackAll_request, 3, 5,
               (token,addr,nbytes, UNPACK(a0),      a1,
                UNPACK(a2)     ),
               (token,addr,nbytes, UNPACK2(a0, a1), a2,
                UNPACK2(a3, a4)));
/* ------------------------------------------------------------------------------------ */
GASNETT_INLINE(strided_unpack_reply_inner)
void strided_unpack_reply_inner(gasnet_token_t token,
                                void *unpack_spin_ptr) {
  int *punpack_spin = (int *)unpack_spin_ptr;
  *(punpack_spin) = 1;
}
SHORT_HANDLER(strided_unpack_reply, 1, 2,
              (token, UNPACK(a0)     ),
              (token, UNPACK2(a0, a1)));
/* ------------------------------------------------------------------------------------ */
/* The data was provided by previous calls; just unpack it with the
   given descriptor. */
GASNETT_INLINE(strided_unpackOnly_request_inner)
void strided_unpackOnly_request_inner(gasnet_token_t token,
                                      void *copyDesc,
                                      size_t copyDescSize,
                                      void *bufAddr,
                                      void *_unpackMethod,
                                      void *unpack_spin_ptr) {
  unpack_method_t unpackMethod = (unpack_method_t)_unpackMethod;
  (*unpackMethod)(copyDesc, (void *)bufAddr);
  upcxxa_free_handlersafe((void *)bufAddr);
#if 0
  gasnett_local_wmb(); /* ensure data is committed before signalling completion */
#endif
  SHORT_REP(1,2,(token, gasneti_handleridx(strided_unpack_reply),
                 PACK(unpack_spin_ptr)));
}
MEDIUM_HANDLER(strided_unpackOnly_request, 3, 6,
               (token,addr,nbytes, UNPACK(a0),      UNPACK(a1),
                UNPACK(a2)     ),
               (token,addr,nbytes, UNPACK2(a0, a1), UNPACK2(a2, a3),
                UNPACK2(a4, a5)));
/* ------------------------------------------------------------------------------------ */
/* Send a contiguous array of data to a node, and have it get unpacked
   into a Titanium array. */
extern void put_array(void *unpack_method, void *copy_desc,
                      size_t copy_desc_size, void *array_data,
                      size_t array_data_size, uint32_t tgt_box) {
  void *data;
  /* ensure double-word alignment for array data */
  size_t copy_desc_size_padded = ((copy_desc_size-1)/8 + 1) * 8;
  size_t data_size = copy_desc_size_padded + array_data_size;
  volatile int unpack_spin = 0;

  /* Fast(er), hopefully common case. */
  if (data_size <= gasnet_AMMaxMedium()) {
    /* The remote end needs the descriptor and the array data. Since
       the data is small, copying the array data will be faster than
       sending two messages. */
    data = (void *) upcxxa_malloc_atomic_huge(data_size);
    if (!data) {
      fprintf(stderr, "failed to allocate %zu bytes in %s\n",
              data_size, "array put AM handler");
      abort();
    }
    assert(data && copy_desc_size_padded % 8 == 0);
    memcpy(data, copy_desc, copy_desc_size);
    /* All math is done in bytes. sizeof(void*) should be irrelevant. */
    memcpy((void *)((uintptr_t)data + copy_desc_size_padded),
           array_data, array_data_size);
    MEDIUM_REQ(3,5,(tgt_box,
                    gasneti_handleridx(strided_unpackAll_request),
                    data, data_size,
                    PACK(UPCXXA_TRANSLATE_FUNCTION_ADDR(unpack_method,
                                                        tgt_box)),
                    copy_desc_size_padded,
                    PACK(&unpack_spin)));
    GASNET_BLOCKUNTIL(unpack_spin);
    upcxxa_free(data);
  }
  else { /* Slow case. */
    /* Allocate a buffer to hold the array data on the remote side. */
    void * volatile remoteAllocBuf = NULL;
    SHORT_REQ(2,4,(tgt_box, gasneti_handleridx(misc_alloc_request),
                   PACK(array_data_size), PACK(&remoteAllocBuf)));
    GASNET_BLOCKUNTIL(remoteAllocBuf);
    /* Transfer the data to the buffer with libtic. */
    gasnet_put_bulk(tgt_box, remoteAllocBuf, array_data,
                    array_data_size);
    /* Tell the remote side to unpack the data. */
    assert(copy_desc_size <= gasnet_AMMaxMedium());
    MEDIUM_REQ(3,6,(tgt_box,
                    gasneti_handleridx(strided_unpackOnly_request),
                    copy_desc, copy_desc_size,
                    PACK(remoteAllocBuf),
                    PACK(UPCXXA_TRANSLATE_FUNCTION_ADDR(unpack_method,
                                                        tgt_box)),
                    PACK(&unpack_spin)));
    GASNET_BLOCKUNTIL(unpack_spin);
  }
}
/* ------------------------------------------------------------------------------------ *
 *  Sparse scatter-gather
 * ------------------------------------------------------------------------------------ */
/* use for packing in gather handlers */
#define FAST_PACK(elem_sz, data, addr_list) do {                \
  switch(elem_sz){                                              \
  case sizeof(uint8_t):                                         \
    {                                                           \
      uint8_t *ubyteData = (uint8_t *) data;                    \
      uint8_t **ubyteAddrList = (uint8_t **) addr_list;         \
      for (i = 0; i < num_elem; i++) {                          \
	ubyteData[i] = *(ubyteAddrList[i]);                     \
      }                                                         \
    }                                                           \
    break;                                                      \
  case sizeof(uint16_t):                                        \
    {                                                           \
      uint16_t *charData = (uint16_t *) data;                   \
      uint16_t **charAddrList = (uint16_t **) addr_list;        \
      for (i = 0; i < num_elem; i++) {                          \
	charData[i] = *(charAddrList[i]);                       \
      }                                                         \
    }                                                           \
    break;                                                      \
  case sizeof(uint32_t):                                        \
    {                                                           \
      uint32_t *uintData = (uint32_t *) data;                   \
      uint32_t **uintAddrList = (uint32_t **) addr_list;        \
      for (i = 0; i < num_elem; i++) {                          \
	uintData[i] = *(uintAddrList[i]);                       \
      }                                                         \
    }                                                           \
    break;                                                      \
  case sizeof(uint64_t):                                        \
    {                                                           \
      uint64_t *ulongData = (uint64_t *) data;                  \
      uint64_t **ulongAddrList = (uint64_t **) addr_list;       \
      for (i = 0; i < num_elem; i++) {                          \
	ulongData[i] = *(ulongAddrList[i]);                     \
      }                                                         \
    }                                                           \
    break;                                                      \
  default:                                                      \
    for (i = 0; i < num_elem; i++) {                            \
      memcpy(data + i*elem_sz, addr_list[i], elem_sz);          \
    }                                                           \
  }} while(0)
/* use for unpacking in scatter handlers */
#define FAST_UNPACK(elem_sz, data, addr_list) do {              \
  switch(elem_sz){                                              \
  case sizeof(uint8_t):                                         \
    {                                                           \
      uint8_t *ubyteData = (uint8_t *) data;                    \
      uint8_t **ubyteAddrList = (uint8_t **) addr_list;         \
      for (i = 0; i < num_elem; i++) {                          \
	*(ubyteAddrList[i]) = ubyteData[i];                     \
      }                                                         \
    }                                                           \
    break;                                                      \
  case sizeof(uint16_t):                                        \
    {                                                           \
      uint16_t *charData = (uint16_t *) data;                   \
      uint16_t **charAddrList = (uint16_t **) addr_list;        \
      for (i = 0; i < num_elem; i++) {                          \
	*(charAddrList[i]) = charData[i];                       \
      }                                                         \
    }                                                           \
    break;                                                      \
  case sizeof(uint32_t):                                        \
    {                                                           \
      uint32_t *uintData = (uint32_t *) data;                   \
      uint32_t **uintAddrList = (uint32_t **) addr_list;        \
      for (i = 0; i < num_elem; i++) {                          \
	*(uintAddrList[i]) = uintData[i];                       \
      }                                                         \
    }                                                           \
    break;                                                      \
  case sizeof(uint64_t):                                        \
    {                                                           \
      uint64_t *ulongData = (uint64_t *) data;                  \
      uint64_t **ulongAddrList = (uint64_t **) addr_list;       \
      for (i = 0; i < num_elem; i++) {                          \
	*(ulongAddrList[i]) = ulongData[i];                     \
      }                                                         \
    }                                                           \
    break;                                                      \
  default:                                                      \
    for (i = 0; i < num_elem; i++) {                            \
      memcpy(addr_list[i], data + i*elem_sz, elem_sz);          \
    }                                                           \
  }} while(0)
/* this macro requires dest and src have alignment of at least length */
#define FAST_MEMCPY(dest, src, length) do {           \
  switch(length) {                                    \
    case sizeof(uint8_t):                             \
      *((uint8_t *)(dest)) = *((uint8_t *)(src));     \
      break;                                          \
    case sizeof(uint16_t):                            \
      *((uint16_t *)(dest)) = *((uint16_t *)(src));   \
      break;                                          \
    case sizeof(uint32_t):                            \
      *((uint32_t *)(dest)) = *((uint32_t *)(src));   \
      break;                                          \
    case sizeof(uint64_t):                            \
      *((uint64_t *)(dest)) = *((uint64_t *)(src));   \
      break;                                          \
    default:                                          \
      memcpy(dest, src, length);                      \
  } } while(0)

extern void sparse_scatter_serial(void **remote_addr_list,
                                  void *src_data_list,
                                  uint32_t remote_box,
                                  size_t num_elem,
                                  size_t elem_sz,
                                  int atomic_elements) {
  volatile int done_ctr = 0;
  size_t datasz;
  size_t offset;

  assert(src_data_list && remote_addr_list &&
         remote_box >= 0 && remote_box < BOXES &&
         num_elem > 0 && elem_sz > 0);

  offset = num_elem * sizeof(void *);
  datasz = offset + num_elem * elem_sz;
  if (datasz <= gasnet_AMMaxMedium()) {
    /* fast case - everything fits in a medium msg */
    char *data = upcxxa_malloc_atomic_huge(datasz);
    if (!data) {
      fprintf(stderr, "failed to allocate %zu bytes in %s\n",
              datasz, "array scatter AM handler");
      abort();
    }
    memcpy(data, remote_addr_list, offset);
    memcpy(data+offset, src_data_list, num_elem * elem_sz);
    MEDIUM_REQ(3,4,(remote_box,
                    gasneti_handleridx(sparse_simpleScatter_request),
                    data, datasz, num_elem, elem_sz,
                    PACK(&done_ctr)));
    GASNET_BLOCKUNTIL(done_ctr);
    upcxxa_free(data);
  } else {  /* slower case - need multiple messages */
    void * volatile remoteAllocBuf = NULL;

    if (datasz <= upcxxa_prealloc){
      /* use preallocate buffer if one is available */
      remoteAllocBuf = get_remote_buffer(datasz, remote_box);
    }
    else{
      /* Allocate a buffer to hold the array data on the remote side. */
      SHORT_REQ(2,4,(remote_box,
                     gasneti_handleridx(misc_alloc_request),
                     PACK(datasz), PACK(&remoteAllocBuf)));
      GASNET_BLOCKUNTIL(remoteAllocBuf);
    }

    /* Transfer the addresses and data to the buffer with libtic (this could be optimized somewhat) */
    gasnet_put_nbi(remote_box, remoteAllocBuf, remote_addr_list,
                   offset);
    gasnet_put_nbi(remote_box, ((char *)remoteAllocBuf)+offset,
                   src_data_list, num_elem * elem_sz);
    upcxxa_write_sync();

    /* Tell the remote side to scatter the data. */
    SHORT_REQ(4,6,(remote_box,
                   gasneti_handleridx(sparse_generalScatter_request),
                   PACK(remoteAllocBuf), num_elem, elem_sz,
                   PACK(&done_ctr)));
    GASNET_BLOCKUNTIL(done_ctr);
  }
}
/* ------------------------------------------------------------------------------------ */
extern void sparse_scatter_pipeline(void **remote_addr_list,
                                    void *src_data_list,
                                    uint32_t remote_box,
                                    size_t num_elem,
                                    size_t elem_sz,
                                    int atomic_elements) {
  volatile int done_ctr = 0;
  size_t datasz;
  size_t offset;

  assert(src_data_list && remote_addr_list &&
         remote_box >= 0 && remote_box < BOXES &&
         num_elem > 0 && elem_sz > 0);

  offset = num_elem * sizeof(void *);
  datasz = offset + num_elem * elem_sz;
  if (datasz <= gasnet_AMMaxMedium()) {
    /* fast case - everything fits in a medium msg */
    char *data = upcxxa_malloc_atomic_huge(datasz);
    if (!data) {
      fprintf(stderr, "failed to allocate %zu bytes in %s\n",
              datasz, "array scatter AM handler");
      abort();
    }
    memcpy(data, remote_addr_list, offset);
    memcpy(data+offset, src_data_list, num_elem * elem_sz);
    MEDIUM_REQ(3,4,(remote_box,
                    gasneti_handleridx(sparse_simpleScatter_request),
                    data, datasz, num_elem, elem_sz,
                    PACK(&done_ctr)));
    GASNET_BLOCKUNTIL(done_ctr);
    upcxxa_free(data);
  } else {  /* slower case - need multiple messages */
    size_t total = num_elem;
    int loadSize;
    int count = 0;
    int messageCount;
    int pairSize = sizeof(void *)+elem_sz;
    char *localBuffer;
    char *localCurBuf;
    size_t bufferSize;
    int loadSizeInBytes;
    int bufferPadding;
    int lastLoadSize;
    int i;
    int addrLoadSize;
    int dataLoadSize;

    loadSize = gasnet_AMMaxMedium() / pairSize;
    addrLoadSize = loadSize*sizeof(void *);
    dataLoadSize = loadSize*elem_sz;
    loadSizeInBytes = addrLoadSize+dataLoadSize;
    bufferPadding = loadSizeInBytes % sizeof(void *);
    if (bufferPadding != 0){
      bufferPadding = sizeof(void *) - bufferPadding;
    }
    loadSizeInBytes += bufferPadding;
    lastLoadSize = (int) (num_elem % loadSize);
    if (lastLoadSize == 0){
      messageCount = (int) (num_elem / loadSize);
      bufferSize =
        ((size_t) loadSizeInBytes) * ((size_t) messageCount);
      /*
      combine the address list and data list into
      a local buffer before sending
      */
      localBuffer = upcxxa_malloc_atomic_huge(bufferSize);
      if (!localBuffer) {
        fprintf(stderr, "failed to allocate %zu bytes in %s\n",
                bufferSize, "array scatter AM handler");
        abort();
      }
      localCurBuf = localBuffer;
      for (i = 0; i < messageCount; i++){
	memcpy(localCurBuf, remote_addr_list+i*loadSize,
               addrLoadSize);
	memcpy(localCurBuf+addrLoadSize,
               ((char *)src_data_list+i*dataLoadSize), dataLoadSize);
	localCurBuf += loadSizeInBytes;
      }
    }
    else{
      messageCount = (int) (num_elem / loadSize) + 1;
      bufferSize = loadSizeInBytes * (messageCount - 1) +
        lastLoadSize * pairSize;
      /*
      combine the address list and data list into
      a buffer before sending
      */
      localBuffer = upcxxa_malloc_atomic_huge(bufferSize);
      if (!localBuffer) {
        fprintf(stderr, "failed to allocate %zu bytes in %s\n",
                bufferSize, "array scatter AM handler");
        abort();
      }
      localCurBuf = localBuffer;
      for (i = 0; i < messageCount-1; i++){
	memcpy(localCurBuf, remote_addr_list+i*loadSize,
               addrLoadSize);
	memcpy(localCurBuf+addrLoadSize,
               ((char *)src_data_list+i*dataLoadSize), dataLoadSize);
	localCurBuf += loadSizeInBytes;
      }
      memcpy(localCurBuf, remote_addr_list+(messageCount-1)*loadSize,
             lastLoadSize*sizeof(void *));
      memcpy(localCurBuf+lastLoadSize*sizeof(void *),
             ((char *)src_data_list+(messageCount-1)*dataLoadSize),
             lastLoadSize*elem_sz);
    }
    localCurBuf = localBuffer;

    {
      int volatile * done_ctr_array = (int *)
        upcxxa_malloc_atomic_uncollectable(sizeof(int) *
                                           messageCount);
      int *curCtr;
      if (!done_ctr_array) {
        fprintf(stderr, "failed to allocate %zu bytes in %s\n",
                (sizeof(int) * messageCount),
                "array scatter AM handler");
        abort();
      }

      for (i = 0; i < messageCount; i++){
	done_ctr_array[i] = 0;
      }

      while (total > 0){
	int currentLoadSize;
	int currentLoadSizeInBytes;

	if (total >= loadSize){
	  currentLoadSize = loadSize;
	  currentLoadSizeInBytes = loadSizeInBytes;
	}
	else{
	  currentLoadSize = (int) total;
	  currentLoadSizeInBytes = (int) (total*pairSize);
	}

	curCtr = (int *) (done_ctr_array+count);
	MEDIUM_REQ(3,4,(remote_box,
                        gasneti_handleridx(sparse_simpleScatter_request),
                        localCurBuf, currentLoadSizeInBytes,
                        currentLoadSize, elem_sz, PACK(curCtr)));
	total -= currentLoadSize;
	localCurBuf += loadSizeInBytes;
	count++;
      }

      for (i = 0; i < messageCount; i++){
	GASNET_BLOCKUNTIL(done_ctr_array[i]);
      }

      upcxxa_free((void *) done_ctr_array);
      upcxxa_free((void *) localBuffer);
    }
  }
}
/* ------------------------------------------------------------------------------------ */
extern void sparse_scatter(void **remote_addr_list,
                           void *src_data_list,
                           uint32_t remote_box,
                           size_t num_elem,
                           size_t elem_sz,
                           int atomic_elements) {
  if (upcxxa_pipelining){
    sparse_scatter_pipeline(remote_addr_list, src_data_list,
                            remote_box, num_elem, elem_sz,
                            atomic_elements);
  }
  else{
    sparse_scatter_serial(remote_addr_list, src_data_list, remote_box,
                          num_elem, elem_sz, atomic_elements);
  }
}
/* ------------------------------------------------------------------------------------ */
GASNETT_INLINE(sparse_simpleScatter_request_inner)
void sparse_simpleScatter_request_inner(gasnet_token_t token,
                                        void *addr_data_list,
                                        size_t addr_data_list_size,
                                        int num_elem,
                                        int elem_sz,
                                        void *_done_ctr) {
  int i;
  void **addr_list = (void**)addr_data_list;
  int offset = num_elem * sizeof(void *);
  char *data = ((char*)addr_data_list) + offset;
  assert(addr_data_list_size == offset + num_elem * elem_sz);
  FAST_UNPACK(elem_sz, data, addr_list);
  SHORT_REP(1,2,(token, gasneti_handleridx(sparse_done_reply),
                 PACK(_done_ctr)));
}
MEDIUM_HANDLER(sparse_simpleScatter_request, 3, 4,
               (token,addr,nbytes, a0, a1, UNPACK(a2)     ),
               (token,addr,nbytes, a0, a1, UNPACK2(a2, a3)));
/* ------------------------------------------------------------------------------------ */
GASNETT_INLINE(sparse_done_reply_inner)
void sparse_done_reply_inner(gasnet_token_t token, void *_done_ctr) {
  int *done_ctr = (int *)_done_ctr;
  *done_ctr = 1;
}
SHORT_HANDLER(sparse_done_reply, 1, 2,
              (token, UNPACK(a0)     ),
              (token, UNPACK2(a0, a1)));
/* ------------------------------------------------------------------------------------ */
GASNETT_INLINE(sparse_generalScatter_request_inner)
void sparse_generalScatter_request_inner(gasnet_token_t token,
                                         void *addr_data_list,
                                         int num_elem,
                                         int elem_sz,
                                         void *_done_ctr) {
  int i;
  void **addr_list = (void**)addr_data_list;
  int offset = num_elem * sizeof(void *);
  char *data = ((char*)addr_data_list) + offset;
  FAST_UNPACK(elem_sz, data, addr_list);
  SHORT_REP(1,2,(token, gasneti_handleridx(sparse_done_reply),
                 PACK(_done_ctr)));
  if ((num_elem*elem_sz+offset) > upcxxa_prealloc){
    upcxxa_free_handlersafe((void*)addr_data_list);
  }
}
SHORT_HANDLER(sparse_generalScatter_request, 4, 6,
              (token, UNPACK(a0),      a1, a2, UNPACK(a3)     ),
              (token, UNPACK2(a0, a1), a2, a3, UNPACK2(a4, a5)));
/* ------------------------------------------------------------------------------------ */
extern void sparse_gather_pipeline(void *tgt_data_list,
                                   void **remote_addr_list,
				   uint32_t remote_box,
                                   size_t num_elem,
                                   size_t elem_sz,
                                   int atomic_elements) {
  volatile int done_ctr = 0;

  assert(tgt_data_list && remote_addr_list &&
         remote_box >= 0 && remote_box < BOXES &&
         num_elem > 0 && elem_sz > 0);

  if (num_elem * sizeof(void *) <= gasnet_AMMaxMedium() &&
      num_elem * elem_sz <= gasnet_AMMaxMedium()) { /* fast case - everything fits in a medium msg */
      MEDIUM_REQ(5,7,(remote_box,
                      gasneti_handleridx(sparse_simpleGather_request),
                      remote_addr_list,
                      (num_elem * sizeof(void *)), num_elem, elem_sz,
                      PACK(tgt_data_list), PACK(&done_ctr),
                      atomic_elements));
      GASNET_BLOCKUNTIL(done_ctr);
  } else { /*pipeline gather requests */
      size_t total = num_elem;
      int loadSize;
      int ptrLimit;
      int dataLimit;
      int count = 0;
      int messageCount;

      ptrLimit = gasnet_AMMaxMedium() / sizeof(void *);
      dataLimit = (int) (gasnet_AMMaxMedium() / elem_sz);
      if (ptrLimit > dataLimit){
	loadSize = dataLimit;
      }
      else{
	loadSize = ptrLimit;
      }
      if (num_elem % loadSize == 0){
        messageCount = (int) (num_elem / loadSize);
      }
      else{
        messageCount = (int) (num_elem / loadSize) + 1;
      }

    {
      int i;
      int volatile * done_ctr_array = (int *)
        upcxxa_malloc_atomic_uncollectable(sizeof(int) *
                                           messageCount);
      int *curCtr;
      if (!done_ctr_array) {
        fprintf(stderr, "failed to allocate %zu bytes in %s\n",
                (sizeof(int) * messageCount),
                "array gather AM handler");
        abort();
      }

      for (i = 0; i < messageCount; i++){
	done_ctr_array[i] = 0;
      }

      while (total > 0){
	int currentLoadSize;

	if (total >= loadSize){
	  currentLoadSize = loadSize;
	}
	else{
          currentLoadSize = (int) total;
	}

	curCtr = (int *) (done_ctr_array+count);
	MEDIUM_REQ(5,7,(remote_box,
                        gasneti_handleridx(sparse_simpleGather_request),
                        (void *) (remote_addr_list + count*loadSize),
                        currentLoadSize * sizeof(void *),
                        currentLoadSize, elem_sz,
                        PACK(((char *) tgt_data_list +
                              count*elem_sz*loadSize)),
                        PACK(curCtr),
                        atomic_elements));
	total -= currentLoadSize;
	count++;
      }

      for (i = 0; i < messageCount; i++){
	GASNET_BLOCKUNTIL(done_ctr_array[i]);
      }

      upcxxa_free((void *) done_ctr_array);
    }
  }
}
/* ------------------------------------------------------------------------------------ */
void sparse_gather_serial(void *tgt_data_list,
                          void **remote_addr_list,
                          uint32_t remote_box,
                          size_t num_elem,
                          size_t elem_sz,
                          int atomic_elements) {
  volatile int done_ctr = 0;

  assert(tgt_data_list && remote_addr_list &&
         remote_box >= 0 && remote_box < BOXES &&
         num_elem > 0 && elem_sz > 0);

  if (num_elem * sizeof(void *) <= gasnet_AMMaxMedium() &&
      num_elem * elem_sz <= gasnet_AMMaxMedium()) { /* fast case - everything fits in a medium msg */
    MEDIUM_REQ(5,7,(remote_box,
                    gasneti_handleridx(sparse_simpleGather_request),
                    remote_addr_list,
                    (num_elem * sizeof(void *)),
                    num_elem, elem_sz,
                    PACK(tgt_data_list), PACK(&done_ctr),
                    atomic_elements));
    GASNET_BLOCKUNTIL(done_ctr);
  } else {  /* slower case - need multiple messages */
    size_t offset = num_elem * sizeof(void *);
    size_t datasz = offset + num_elem * elem_sz;
    void * volatile remoteAllocBuf = NULL;

    if (datasz <= upcxxa_prealloc){
      /* use a preallocated buffer if one is available */
      remoteAllocBuf = get_remote_buffer(datasz, remote_box);
    }
    else{
      /* Allocate a buffer to hold the array data on the remote side. */
      SHORT_REQ(2,4,(remote_box,
                     gasneti_handleridx(misc_alloc_request),
                     PACK(datasz), PACK(&remoteAllocBuf)));
      GASNET_BLOCKUNTIL(remoteAllocBuf);
    }

    /* Transfer the addresses to the buffer with libtic (this could be
       optimized somewhat) */
    gasnet_put_bulk(remote_box, remoteAllocBuf, remote_addr_list,
                    offset);

    /* Tell the remote side to gather the data. */
    SHORT_REQ(4,6,(remote_box,
                   gasneti_handleridx(sparse_generalGather_request),
                   PACK(remoteAllocBuf), num_elem, elem_sz,
                   PACK(&done_ctr)));
    GASNET_BLOCKUNTIL(done_ctr);

    /* Transfer the data from the buffer with libtic (this could be optimized somewhat) */
    gasnet_get_bulk(tgt_data_list,
                    remote_box, ((char *)remoteAllocBuf)+offset,
                    (num_elem * elem_sz)); /* (does not handle gp escape) */

    if (datasz > upcxxa_prealloc){
      SHORT_REQ(1,2,(remote_box,
                     gasneti_handleridx(misc_delete_request),
                     PACK(remoteAllocBuf)));
    }
  }
}
/* ------------------------------------------------------------------------------------ */
void sparse_gather(void *tgt_data_list, void **remote_addr_list,
		   uint32_t remote_box, size_t num_elem,
                   size_t elem_sz, int atomic_elements) {
  if (upcxxa_pipelining){
    sparse_gather_pipeline(tgt_data_list, remote_addr_list,
                           remote_box, num_elem, elem_sz,
                           atomic_elements);
  }
  else{
    sparse_gather_serial(tgt_data_list, remote_addr_list, remote_box,
                         num_elem, elem_sz, atomic_elements);
  }
}
/* ------------------------------------------------------------------------------------ */
GASNETT_INLINE(sparse_simpleGather_request_inner)
void sparse_simpleGather_request_inner(gasnet_token_t token,
                                       void *_addr_list,
                                       size_t addr_list_size,
                                       int num_elem,
                                       int elem_sz,
                                       void *_tgt_data_list,
                                       void *_done_ctr,
                                       int atomic_elements) {
  int i;
  void **addr_list = (void**)_addr_list;
  size_t datasz = num_elem * elem_sz;
  char *data = upcxxa_malloc_handlersafe(datasz);
  if (!data) {
    fprintf(stderr, "failed to allocate %zu bytes in %s\n",
            datasz, "array gather AM handler");
    abort();
  }
  assert(addr_list_size == sizeof(void *)*num_elem);
  FAST_PACK(elem_sz, data, addr_list);
  MEDIUM_REP(4,6,(token,
                  gasneti_handleridx(sparse_simpleGather_reply),
                  data, datasz, num_elem, elem_sz,
                  PACK(_tgt_data_list), PACK(_done_ctr)));
  upcxxa_free_handlersafe(data);
}
MEDIUM_HANDLER(sparse_simpleGather_request, 5, 7,
               (token,addr,nbytes, a0, a1, UNPACK(a2),
                UNPACK(a3),      a4),
               (token,addr,nbytes, a0, a1, UNPACK2(a2, a3),
                UNPACK2(a4, a5), a6));
/* ------------------------------------------------------------------------------------ */
GASNETT_INLINE(sparse_simpleGather_reply_inner)
void sparse_simpleGather_reply_inner(gasnet_token_t token,
                                     void *data_list,
                                     size_t data_list_size,
                                     int num_elem,
                                     int elem_sz,
                                     void *_tgt_data_list,
                                     void *_done_ctr) {
  void *tgt_data_list = (void *)_tgt_data_list;
  int *done_ctr = (int *)_done_ctr;
  assert(data_list_size == num_elem * elem_sz);
  memcpy(tgt_data_list, data_list, data_list_size);
  *done_ctr = 1;
}
MEDIUM_HANDLER(sparse_simpleGather_reply, 4, 6,
               (token,addr,nbytes, a0, a1, UNPACK(a2),
                UNPACK(a3)     ),
               (token,addr,nbytes, a0, a1, UNPACK2(a2, a3),
                UNPACK2(a4, a5)));
/* ------------------------------------------------------------------------------------ */
GASNETT_INLINE(sparse_generalGather_request_inner)
void sparse_generalGather_request_inner(gasnet_token_t token,
                                        void *addr_data_list,
                                        int num_elem,
                                        int elem_sz,
                                        void *_done_ctr) {
  int i;
  void **addr_list = (void**)addr_data_list;
  int offset = num_elem * sizeof(void *);
  char *data = ((char*)addr_data_list) + offset;
  FAST_PACK(elem_sz, data, addr_list);
  SHORT_REP(1,2,(token, gasneti_handleridx(sparse_done_reply),
                 PACK(_done_ctr)));
}
SHORT_HANDLER(sparse_generalGather_request, 4, 6,
              (token, UNPACK(a0),      a1, a2, UNPACK(a3)     ),
              (token, UNPACK2(a0, a1), a2, a3, UNPACK2(a4, a5)));
/* ------------------------------------------------------------------------------------ */

/*
  read environment variables prealloc and pipelining
*/
void gather_init(){
  char *preallocstr;
  char *pipeliningstr;

  preallocstr = (char *) gasnet_getenv("UPCXXA_PREALLOC");
  if (preallocstr == NULL){
    upcxxa_prealloc = 0;
  }
  else{
    upcxxa_prealloc = atoi(preallocstr);
    assert(upcxxa_prealloc >= 0);
    if (upcxxa_prealloc > 0){
      buffer_init();
      /* convert KB to bytes */
      upcxxa_prealloc *= 1024;
    }
  }
  pipeliningstr = (char *) gasnet_getenv("UPCXXA_PIPELINING");
  if (pipeliningstr == NULL){
    upcxxa_pipelining = 0;
  }
  else{
    upcxxa_pipelining = atoi(pipeliningstr);
  }
}
