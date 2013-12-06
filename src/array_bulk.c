#include <stdlib.h>
#include <assert.h>
#include <array_bulk.h>
#include "gasnet_api.h"
#include <portable_inttypes.h>
#include "allocate.h"

#define AM2_HAS_HUGE_SEGMENTS 0 /* can't assume GASNet SEGMENT_EVERYTHING configuration */ 
#define AM2_REPLY_REQUIRED    0

/* No multithreading support for now */
#define BOXES       gasnet_nodes() /* TODO: use UPC++ global_machine.node_count() */
#define MYBOXPROCS  1              /* TODO: use UPC++ my_node.processor_count() */
#define MYBOXPROC   0              /* TODO: use UPC++ my_processor.local_id() */

#define TIC_AM_SEGOFFSET 0

#if PLATFORM_ARCH_32
  #define TIC_SHORT_HANDLER(name, cnt32, cnt64, innerargs32, innerargs64)            \
    GASNETI_HANDLER_SCOPE void name ## _32(gasnet_token_t token ARGS ## cnt32) { \
      name innerargs32 ;                                               \
    } static int _dummy_##name = sizeof(_dummy_##name)
  #define TIC_MEDIUM_HANDLER(name, cnt32, cnt64, innerargs32, innerargs64)                     \
    GASNETI_HANDLER_SCOPE void name ## _32(gasnet_token_t token, void *addr, size_t nbytes \
                           ARGS ## cnt32) {                                                \
      name innerargs32 ;                                                         \
    } static int _dummy_##name = sizeof(_dummy_##name)
#elif PLATFORM_ARCH_64
  #define TIC_SHORT_HANDLER(name, cnt32, cnt64, innerargs32, innerargs64)            \
    GASNETI_HANDLER_SCOPE void name ## _64(gasnet_token_t token ARGS ## cnt64) { \
      name innerargs64 ;                                               \
    } static int _dummy_##name = sizeof(_dummy_##name)
  #define TIC_MEDIUM_HANDLER(name, cnt32, cnt64, innerargs32, innerargs64)                     \
    GASNETI_HANDLER_SCOPE void name ## _64(gasnet_token_t token, void *addr, size_t nbytes \
                           ARGS ## cnt64) {                                                \
      name innerargs64 ;                                                         \
    } static int _dummy_##name = sizeof(_dummy_##name)
#endif

#define TIC_LONG_HANDLER TIC_MEDIUM_HANDLER

#define ti_malloc_handlersafe(s) gasnet_seg_alloc(s)
#define ti_free_handlersafe(p)   gasnet_seg_free(p)
#define ti_malloc_atomic_uncollectable(s) gasnet_seg_alloc(s)
#define ti_malloc_atomic_huge(s) gasnet_seg_alloc(s)
#define ti_free(p) gasnet_seg_free(p)

/* look into how to sync puts in UPC++ */
#define ti_write_sync()

/* macro for issuing a do-nothing AM reply */
#if AM2_REPLY_REQUIRED
  #define TIC_NULL_REPLY(token) \
    SHORT_REP(0,0,(token, gasneti_handleridx(misc_null_reply)))
#else
  #define TIC_NULL_REPLY(token) 
#endif

/* FIX THIS VVV */
#define TIC_TRANSLATE_FUNCTION_ADDR(localaddr, targetbox) \
  (localaddr)

/**************************
 * INITIALIZATION/STATICS *
 *************************/

/*
  preallocate buffer size in bytes
*/
static int tic_prealloc;

/*
  pipelining is 1 if using pipelining
*/
static int tic_pipelining;

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
  bufferArray = (struct Buffer **) ti_malloc_atomic_uncollectable(sizeof(struct Buffer *)*MYBOXPROCS);
  for (i = 0; i < MYBOXPROCS; i++){
    struct Buffer *tmp;
    bufferArray[i] = (struct Buffer *) ti_malloc_atomic_uncollectable(sizeof(struct Buffer)*BOXES);
    tmp = bufferArray[i];
    for (j = 0; j < BOXES; j++){
	tmp->size = -1;
	tmp->ptr = NULL;
	tmp++;
    }
  }
}

/*
  in the case of preallocate buffer, return a pointer to the buffer of size datasz or greater for node remote_box
*/
void *get_remote_buffer(int datasz, int remote_box){
  void * volatile remoteAllocBuf = NULL;
  struct Buffer *myBuffers = bufferArray[MYBOXPROC];
  struct Buffer *buffer = myBuffers+remote_box;
  
  /* Allocate a buffer to hold the array data on the remote sideif there is no preallocated buffer of this size or greater. */
  if (buffer->size >= datasz){
      remoteAllocBuf = buffer->ptr;
  }
  else if (buffer->size == -1){
      SHORT_REQ(2,3,(remote_box, gasneti_handleridx(misc_alloc_request), 
			 datasz, PACK(&remoteAllocBuf)));
      GASNET_BLOCKUNTIL(remoteAllocBuf);
      assert((((uintptr_t)remoteAllocBuf) % sizeof(void *) == 0));
      buffer->size = datasz;
      buffer->ptr = remoteAllocBuf;
  }
  else{
      /* first delete old buffer that is too small */
      SHORT_REQ(1,2,(remote_box, gasneti_handleridx(misc_delete_request), 
			 PACK(buffer->ptr)));
      /* allocate new buffer with size datasz */
      SHORT_REQ(2,3,(remote_box, gasneti_handleridx(misc_alloc_request), 
			 datasz, PACK(&remoteAllocBuf)));
      GASNET_BLOCKUNTIL(remoteAllocBuf);
      assert((((uintptr_t)remoteAllocBuf) % sizeof(void *) == 0));
      buffer->size = datasz;
      buffer->ptr = remoteAllocBuf;
  }      
  return remoteAllocBuf;
}

typedef void *(*pack_method_t)(void *, int *, void *);
typedef void *(*unpack_method_t)(void *, void *);

/* ------------------------------------------------------------------------------------ */
/***************************
 * Miscellaneous handlers. *
 ***************************/

GASNETT_INLINE(misc_null_reply)
void misc_null_reply(gasnet_token_t token) {
  return;
}
TIC_SHORT_HANDLER(misc_null_reply, 0, 0,
              (token),
              (token));
/* ------------------------------------------------------------------------------------ */
GASNETT_INLINE(misc_delete_request)
void misc_delete_request(gasnet_token_t token, void *addr) {
  ti_free_handlersafe(addr);
  TIC_NULL_REPLY(token);
}
TIC_SHORT_HANDLER(misc_delete_request, 1, 2,
              (token, UNPACK(a0)    ),
              (token, UNPACK2(a0, a1)));
/* ------------------------------------------------------------------------------------ */
/* Size is in bytes. */
GASNETT_INLINE(misc_alloc_request)
void misc_alloc_request(gasnet_token_t token, int size, void *destptr) {
  void *buf = ti_malloc_handlersafe(size);
  SHORT_REP(2,4,(token, gasneti_handleridx(misc_alloc_reply), 
                   PACK(buf), PACK(destptr)));
}
TIC_SHORT_HANDLER(misc_alloc_request, 2, 3,
              (token, a0, UNPACK(a1)    ),
              (token, a0, UNPACK2(a1, a2)));
/* ------------------------------------------------------------------------------------ */
GASNETT_INLINE(misc_alloc_reply)
void misc_alloc_reply(gasnet_token_t token, void *buf, void *destptr) {
  void ** premoteAllocBuf = (void **)destptr;
  (*premoteAllocBuf) = (void *)buf;
}
TIC_SHORT_HANDLER(misc_alloc_reply, 2, 4,
              (token, UNPACK(a0),     UNPACK(a1)),
              (token, UNPACK2(a0, a1), UNPACK2(a2, a3)));
/* ------------------------------------------------------------------------------------ *
 *  Rectangular strided copy
 * ------------------------------------------------------------------------------------ */
/************************************
 * Get an array from a remote node. *
 ***********************************/

/* Call _packMethod to pack the array with copy_desc as its "copy descriptor",
   returning the packed data to remote address remoteBuffer. */
GASNETT_INLINE(strided_pack_request)
void strided_pack_request(gasnet_token_t token, void *copy_desc, size_t copy_desc_size,
			void *_packMethod, void *remoteBuffer, void *pack_info_ptr, 
			int atomicelements) {
  int array_data_len;
  void *array_data;
  pack_method_t packMethod = (pack_method_t)_packMethod;
  /* A NULL buffer pointer tells the packer to allocate its own buffer. 
   * (void*)-1 tells it to use handlersafe, uncollectable memory so it doesn't disappear on us
   */
  array_data = (*packMethod)(copy_desc, &array_data_len, (void*)-1);

  #if defined(USE_DISTRIBUTED_GC) && !defined(USE_GC_NONE)
    /* inform distributed GC that some pointers may be escaping */
    if (!atomicelements)
      dgc_ptr_esc_all_gps(array_data, array_data_len, 1);
  #endif

  if (array_data_len <= gasnet_AMMaxMedium()) {
    /* Common, fast case. Send back the packed data with the reply. */
    MEDIUM_REP(4,7,(token, gasneti_handleridx(strided_pack_reply), array_data, array_data_len, 
                      PACK(remoteBuffer), PACK(0), 
                      0, PACK(pack_info_ptr)));
    ti_free_handlersafe(array_data);
  }
  else {
#if 0
    gasnett_local_wmb(); /* ensure data is committed before signalling initiator to pull data */
#endif
    /* Slower case. Why doesn't the darned AM_ReplyXfer() work? */
    MEDIUM_REP(4,7,(token, gasneti_handleridx(strided_pack_reply), array_data, gasnet_AMMaxMedium(), 
                       PACK(remoteBuffer), PACK(array_data), 
		       array_data_len, PACK(pack_info_ptr)));
  }
}
TIC_MEDIUM_HANDLER(strided_pack_request, 4, 7,
  (token,addr,nbytes, UNPACK(a0),     UNPACK(a1),     UNPACK(a2),     a3),
  (token,addr,nbytes, UNPACK2(a0, a1), UNPACK2(a2, a3), UNPACK2(a4, a5), a6));

/* ------------------------------------------------------------------------------------ */
/* The reply handler needs to pass this information back to the caller. */
typedef struct {
  volatile int pack_spin; /* 0 while waiting, 1 if completed, 2 if using fragmentation */
  void * volatile remoteBuffer;  
  volatile int dataSize;  
} pack_return_info_t;

/* Sets the spin flag to 1 if the array fit in the message reply and we're
   done; otherwise it sets the spinlock variable to be the remote address 
   of the packed data, which must be sent over with a libtic bulk 
   transfer. */
GASNETT_INLINE(strided_pack_reply)
void strided_pack_reply(gasnet_token_t token, void *array_data, size_t array_data_size, 
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
TIC_MEDIUM_HANDLER(strided_pack_reply, 4, 7,
  (token,addr,nbytes, UNPACK(a0),     UNPACK(a1),     a2, UNPACK(a3)    ),
  (token,addr,nbytes, UNPACK2(a0, a1), UNPACK2(a2, a3), a4, UNPACK2(a5, a6)));
/* ------------------------------------------------------------------------------------ */
extern void get_array(void *pack_method, void *copy_desc, int copy_desc_size, 
	              int tgt_box, void *buffer, int atomicelements) {
  pack_return_info_t info;
  info.pack_spin = 0;
  /* A copy_desc is an object that contains the array descriptor. */
  assert(copy_desc && copy_desc_size > 0 && copy_desc_size <= gasnet_AMMaxMedium());
  MEDIUM_REQ(4,7,(tgt_box, gasneti_handleridx(strided_pack_request), 
		       copy_desc, copy_desc_size, 
                       PACK(TIC_TRANSLATE_FUNCTION_ADDR(pack_method,tgt_box)), 
		       PACK(buffer), 
                       PACK(&info), atomicelements));
  GASNET_BLOCKUNTIL(info.pack_spin);
  /* If the data was too big to be sent in the reply, use a Tic call
     to do the job. */
  if (info.pack_spin != 1) {
    /* pull the data to use using a bulk copy (first fragment has already been copied) */
    gasnet_get_bulk((void *)((uintptr_t)buffer+gasnet_AMMaxMedium()), 
                 tgt_box, ((uint8_t *)info.remoteBuffer) + gasnet_AMMaxMedium(), 
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

/* The fastest way of doing things....take the data given and unpack it. */
GASNETT_INLINE(strided_unpackAll_request)
void strided_unpackAll_request(gasnet_token_t token, void *packedArrayData, size_t nBytes, 
		   void *_unpackMethod, int copyDescSize, void *unpack_spin_ptr) {
 void *temp = NULL;
 if (((uintptr_t)packedArrayData) % 8 != 0) {
   /* it seems that some crappy AM implementations (ahem.. the NOW)
    * fail to double-word align the medium-sized message buffer
    * fix that problem if it occurs
    */
   temp = ti_malloc_handlersafe(nBytes+8);
   memcpy(temp, packedArrayData, nBytes);
   packedArrayData = temp;
 } 
 {
   unpack_method_t unpackMethod = (unpack_method_t)_unpackMethod;
   void *copyDesc = packedArrayData;
   void *arrayData = (void *)((uintptr_t)packedArrayData + copyDescSize);
   assert(((uintptr_t)copyDesc) % 8 == 0 && ((uintptr_t)arrayData) % 8 == 0);
   (*unpackMethod)(copyDesc, arrayData);
#if 0
   gasnett_local_wmb(); /* ensure data is committed before signalling completion */
#endif
   SHORT_REP(1,2,(token, gasneti_handleridx(strided_unpack_reply), PACK(unpack_spin_ptr)));
 }
 if (temp) ti_free_handlersafe(temp);
}
TIC_MEDIUM_HANDLER(strided_unpackAll_request, 3, 5,
  (token,addr,nbytes, UNPACK(a0),     a1, UNPACK(a2)    ),
  (token,addr,nbytes, UNPACK2(a0, a1), a2, UNPACK2(a3, a4)));
/* ------------------------------------------------------------------------------------ */
GASNETT_INLINE(strided_unpack_reply)
void strided_unpack_reply(gasnet_token_t token, void *unpack_spin_ptr) {
  int *punpack_spin = (int *)unpack_spin_ptr;
  *(punpack_spin) = 1;
}
TIC_SHORT_HANDLER(strided_unpack_reply, 1, 2,
              (token, UNPACK(a0)    ),
              (token, UNPACK2(a0, a1)));
/* ------------------------------------------------------------------------------------ */
/* The data was provided by previous calls; just unpack it with the given
   descriptor. */
GASNETT_INLINE(strided_unpackOnly_request)
void strided_unpackOnly_request(gasnet_token_t token, void *copyDesc, size_t copyDescSize,
			      void *bufAddr, void *_unpackMethod, void *unpack_spin_ptr) {
  unpack_method_t unpackMethod = (unpack_method_t)_unpackMethod;
  (*unpackMethod)(copyDesc, (void *)bufAddr);
  ti_free_handlersafe((void *)bufAddr);
#if 0
  gasnett_local_wmb(); /* ensure data is committed before signalling completion */
#endif
  SHORT_REP(1,2,(token, gasneti_handleridx(strided_unpack_reply), PACK(unpack_spin_ptr)));
}
TIC_MEDIUM_HANDLER(strided_unpackOnly_request, 3, 6,
  (token,addr,nbytes, UNPACK(a0),     UNPACK(a1),     UNPACK(a2)    ),
  (token,addr,nbytes, UNPACK2(a0, a1), UNPACK2(a2, a3), UNPACK2(a4, a5)));
/* ------------------------------------------------------------------------------------ */
/* Send a contiguous array of data to a node, and have it get unpacked into
   a Titanium array. */
extern void put_array(void *unpack_method, void *copy_desc, int copy_desc_size,
                      void *array_data, int array_data_size, int tgt_box) {
  void *data;
  /* ensure double-word alignment for array data */
  int copy_desc_size_padded = ((copy_desc_size-1)/8 + 1) * 8; 
  int data_size = copy_desc_size_padded + array_data_size;
  volatile int unpack_spin = 0;

  /* Fast(er), hopefully common case. */
  if (data_size <= gasnet_AMMaxMedium()) {
    /* The remote end needs the descriptor and the array data. Since the
       data is small, copying the array data will be faster than
       sending two messages. */
    data = (void *) ti_malloc_atomic_huge(data_size);
    assert(data && copy_desc_size_padded % 8 == 0);
    memcpy(data, copy_desc, copy_desc_size);
    /* All math is done in bytes. sizeof(void*) should be irrelevant. */
    memcpy((void *)((uintptr_t)data + copy_desc_size_padded), array_data, array_data_size);
    MEDIUM_REQ(3,5,(tgt_box, gasneti_handleridx(strided_unpackAll_request), 
			 data, data_size, 
                         PACK(TIC_TRANSLATE_FUNCTION_ADDR(unpack_method,tgt_box)), 
			 copy_desc_size_padded, 
                         PACK(&unpack_spin)));
    GASNET_BLOCKUNTIL(unpack_spin);
    ti_free(data);
  }
  else { /* Slow case. */
    /* Allocate a buffer to hold the array data on the remote side. */
    void * volatile remoteAllocBuf = NULL;
    SHORT_REQ(2,3,(tgt_box, gasneti_handleridx(misc_alloc_request), 
			array_data_size, PACK(&remoteAllocBuf)));
    GASNET_BLOCKUNTIL(remoteAllocBuf);
    /* Transfer the data to the buffer with libtic. */
    gasnet_put_bulk(tgt_box, remoteAllocBuf, array_data, array_data_size);
    /* Tell the remote side to unpack the data. */
    assert(copy_desc_size <= gasnet_AMMaxMedium());
    MEDIUM_REQ(3,6,(tgt_box, gasneti_handleridx(strided_unpackOnly_request),
			 copy_desc, copy_desc_size, 
                         PACK(remoteAllocBuf), 
			 PACK(TIC_TRANSLATE_FUNCTION_ADDR(unpack_method,tgt_box)), 
                         PACK(&unpack_spin)));
    GASNET_BLOCKUNTIL(unpack_spin);
  }
}
/* ------------------------------------------------------------------------------------ *
 *  Sparse scatter-gather
 * ------------------------------------------------------------------------------------ */
/* use for packing in gather handlers */
#define FAST_PACK(elem_sz, data, addr_list) do {      \
  switch(elem_sz){                                    \
  case sizeof(uint8_t):                               \
    {                                                 \
      uint8_t *ubyteData = (uint8_t *) data;          \
      uint8_t **ubyteAddrList = (uint8_t **) addr_list; \
      for (i = 0; i < num_elem; i++) {                \
	ubyteData[i] = *(ubyteAddrList[i]);           \
      }                                               \
    }                                                 \
    break;                                            \
  case sizeof(uint16_t):                              \
    {                                                 \
      uint16_t *charData = (uint16_t *) data;         \
      uint16_t **charAddrList = (uint16_t **) addr_list; \
      for (i = 0; i < num_elem; i++) {                \
	charData[i] = *(charAddrList[i]);             \
      }                                               \
    }                                                 \
    break;                                            \
  case sizeof(uint32_t):                              \
    {                                                 \
      uint32_t *uintData = (uint32_t *) data;         \
      uint32_t **uintAddrList = (uint32_t **) addr_list; \
      for (i = 0; i < num_elem; i++) {                \
	uintData[i] = *(uintAddrList[i]);             \
      }                                               \
    }                                                 \
    break;                                            \
  case sizeof(uint64_t):                              \
    {                                                 \
      uint64_t *ulongData = (uint64_t *) data;        \
      uint64_t **ulongAddrList = (uint64_t **) addr_list; \
      for (i = 0; i < num_elem; i++) {                \
	ulongData[i] = *(ulongAddrList[i]);           \
      }                                               \
    }                                                 \
    break;                                            \
  default:                                            \
    for (i = 0; i < num_elem; i++) {                  \
      memcpy(data + i*elem_sz, addr_list[i], elem_sz);\
    }                                                 \
  }} while(0)
/* use for unpacking in scatter handlers */
#define FAST_UNPACK(elem_sz, data, addr_list) do {    \
  switch(elem_sz){                                    \
  case sizeof(uint8_t):                               \
    {                                                 \
      uint8_t *ubyteData = (uint8_t *) data;          \
      uint8_t **ubyteAddrList = (uint8_t **) addr_list; \
      for (i = 0; i < num_elem; i++) {                \
	*(ubyteAddrList[i]) = ubyteData[i];           \
      }                                               \
    }                                                 \
    break;                                            \
  case sizeof(uint16_t):                              \
    {                                                 \
      uint16_t *charData = (uint16_t *) data;         \
      uint16_t **charAddrList = (uint16_t **) addr_list; \
      for (i = 0; i < num_elem; i++) {                \
	*(charAddrList[i]) = charData[i];             \
      }                                               \
    }                                                 \
    break;                                            \
  case sizeof(uint32_t):                              \
    {                                                 \
      uint32_t *uintData = (uint32_t *) data;         \
      uint32_t **uintAddrList = (uint32_t **) addr_list; \
      for (i = 0; i < num_elem; i++) {                \
	*(uintAddrList[i]) = uintData[i];             \
      }                                               \
    }                                                 \
    break;                                            \
  case sizeof(uint64_t):                              \
    {                                                 \
      uint64_t *ulongData = (uint64_t *) data;        \
      uint64_t **ulongAddrList = (uint64_t **) addr_list; \
      for (i = 0; i < num_elem; i++) {                \
	*(ulongAddrList[i]) = ulongData[i];           \
      }                                               \
    }                                                 \
    break;                                            \
  default:                                            \
    for (i = 0; i < num_elem; i++) {                  \
      memcpy(addr_list[i], data + i*elem_sz, elem_sz);\
    }                                                 \
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

extern void sparse_scatter_serial(void **remote_addr_list, void *src_data_list, 
                           int remote_box, int num_elem, int elem_sz, int atomic_elements) {
  volatile int done_ctr = 0;
  int datasz;
  int offset;

  assert(src_data_list && remote_addr_list && 
         remote_box >= 0 && remote_box < BOXES &&
         num_elem > 0 && elem_sz > 0);

  offset = num_elem * sizeof(void *);
  datasz = offset + num_elem * elem_sz;
  if (datasz <= gasnet_AMMaxMedium()) { 
    /* fast case - everything fits in a medium msg */
    char *data = ti_malloc_atomic_huge(datasz);
    memcpy(data, remote_addr_list, offset);
    memcpy(data+offset, src_data_list, num_elem * elem_sz);
    #if defined(USE_DISTRIBUTED_GC) && !defined(USE_GC_NONE)
      /* inform distributed GC that some pointers may be escaping */
      if (!atomic_elements) 
        GC_PTR_ESC_ALL_GPS(data+offset, num_elem * elem_sz);
    #endif
    MEDIUM_REQ(3,4,(remote_box, gasneti_handleridx(sparse_simpleScatter_request),
			 data, datasz, 
                         num_elem, elem_sz,
                         PACK(&done_ctr)));
    GASNET_BLOCKUNTIL(done_ctr);
    ti_free(data);
  } else {  /* slower case - need multiple messages */
    void * volatile remoteAllocBuf = NULL;

    if (datasz <= tic_prealloc){
      /* use preallocate buffer if one is available */
      remoteAllocBuf = get_remote_buffer(datasz, remote_box);
    }
    else{
      /* Allocate a buffer to hold the array data on the remote side. */
      SHORT_REQ(2,3,(remote_box, gasneti_handleridx(misc_alloc_request), 
			 datasz, PACK(&remoteAllocBuf)));
      GASNET_BLOCKUNTIL(remoteAllocBuf);
    }

    /* Transfer the addresses and data to the buffer with libtic (this could be optimized somewhat) */
    gasnet_put_nbi(remote_box, remoteAllocBuf, remote_addr_list, offset);
    #if defined(USE_DISTRIBUTED_GC) && !defined(USE_GC_NONE)
      /* inform distributed GC that some pointers may be escaping */
      if (!atomic_elements) 
        GC_PTR_ESC_ALL_GPS(src_data_list, num_elem * elem_sz);
    #endif
    gasnet_put_nbi(remote_box, ((char *)remoteAllocBuf)+offset, src_data_list, num_elem * elem_sz);
    ti_write_sync();

    /* Tell the remote side to scatter the data. */
    SHORT_REQ(4,6,(remote_box, gasneti_handleridx(sparse_generalScatter_request),
		       PACK(remoteAllocBuf), 
		       num_elem, elem_sz, 
		       PACK(&done_ctr)));
    GASNET_BLOCKUNTIL(done_ctr);
  }
}
/* ------------------------------------------------------------------------------------ */
extern void sparse_scatter_pipeline(void **remote_addr_list, void *src_data_list, 
                           int remote_box, int num_elem, int elem_sz, int atomic_elements) {
  volatile int done_ctr = 0;
  int datasz;
  int offset;

  assert(src_data_list && remote_addr_list && 
         remote_box >= 0 && remote_box < BOXES &&
         num_elem > 0 && elem_sz > 0);

  offset = num_elem * sizeof(void *);
  datasz = offset + num_elem * elem_sz;
  if (datasz <= gasnet_AMMaxMedium()) { 
    /* fast case - everything fits in a medium msg */
    char *data = ti_malloc_atomic_huge(datasz);
    memcpy(data, remote_addr_list, offset);
    memcpy(data+offset, src_data_list, num_elem * elem_sz);
    #if defined(USE_DISTRIBUTED_GC) && !defined(USE_GC_NONE)
      /* inform distributed GC that some pointers may be escaping */
      if (!atomic_elements) 
        GC_PTR_ESC_ALL_GPS(data+offset, num_elem * elem_sz);
    #endif
    MEDIUM_REQ(3,4,(remote_box, gasneti_handleridx(sparse_simpleScatter_request),
			 data, datasz, 
                         num_elem, elem_sz,
                         PACK(&done_ctr)));
    GASNET_BLOCKUNTIL(done_ctr);
    ti_free(data);
  } else {  /* slower case - need multiple messages */
    int total = num_elem;
    int loadSize;
    int count = 0;
    int messageCount;
    int pairSize = sizeof(void *)+elem_sz;
    char *localBuffer;
    char *localCurBuf;
    int bufferSize;
    int loadSizeInBytes;
    int bufferPadding;
    int lastLoadSize;
    int i;
    int addrLoadSize;
    int dataLoadSize;

#if AM2_HAS_HUGE_SEGMENTS
    void * volatile remoteAllocBuf = NULL;
    void *curAllocBuf = NULL;
    loadSize = gasnet_AMMaxLongRequest() / pairSize;
#else
    loadSize = gasnet_AMMaxMedium() / pairSize;
#endif

    addrLoadSize = loadSize*sizeof(void *);
    dataLoadSize = loadSize*elem_sz;
    loadSizeInBytes = addrLoadSize+dataLoadSize;
    bufferPadding = loadSizeInBytes % sizeof(void *);
    if (bufferPadding != 0){
      bufferPadding = sizeof(void *) - bufferPadding;
    }
    loadSizeInBytes += bufferPadding;
    lastLoadSize = num_elem % loadSize;
    if (lastLoadSize == 0){
      messageCount = num_elem / loadSize;
      bufferSize = loadSizeInBytes * messageCount;
      /*
      combine the address list and data list into
      a local buffer before sending
      */
      localBuffer = ti_malloc_atomic_huge(bufferSize);
      localCurBuf = localBuffer;
      for (i = 0; i < messageCount; i++){
	memcpy(localCurBuf, remote_addr_list+i*loadSize, addrLoadSize);
	memcpy(localCurBuf+addrLoadSize, ((char *)src_data_list+i*dataLoadSize), dataLoadSize);
	localCurBuf += loadSizeInBytes;
      }
    }
    else{
      messageCount = num_elem / loadSize + 1;
      bufferSize = loadSizeInBytes * (messageCount - 1) + lastLoadSize * pairSize;
      /*
      combine the address list and data list into
      a buffer before sending
      */
      localBuffer = ti_malloc_atomic_huge(bufferSize);
      localCurBuf = localBuffer;
      for (i = 0; i < messageCount-1; i++){
	memcpy(localCurBuf, remote_addr_list+i*loadSize, addrLoadSize);
	memcpy(localCurBuf+addrLoadSize, ((char *)src_data_list+i*dataLoadSize), dataLoadSize);
	localCurBuf += loadSizeInBytes;
      }
      memcpy(localCurBuf, remote_addr_list+(messageCount-1)*loadSize, lastLoadSize*sizeof(void *));
      memcpy(localCurBuf+lastLoadSize*sizeof(void *), ((char *)src_data_list+(messageCount-1)*dataLoadSize), lastLoadSize*elem_sz);
    }
    localCurBuf = localBuffer;

#if AM2_HAS_HUGE_SEGMENTS      
    if (bufferSize <= tic_prealloc){
      /* use preallocate buffer if one is available */
      remoteAllocBuf = get_remote_buffer(bufferSize, remote_box);
    }
    else{
      /* get a buffer for this gather use only */
      SHORT_REQ(2,3,(remote_box, gasneti_handleridx(misc_alloc_request), 
			 bufferSize, PACK(&remoteAllocBuf)));
      GASNET_BLOCKUNTIL(remoteAllocBuf);
    }

    curAllocBuf = remoteAllocBuf;
#endif
         
#if defined(USE_DISTRIBUTED_GC) && !defined(USE_GC_NONE)
    /* inform distributed GC that some pointers may be escaping */
    if (!atomic_elements) 
      GC_PTR_ESC_ALL_GPS(src_data_list, num_elem * elem_sz);
#endif

    {
      int volatile * done_ctr_array = (int *)ti_malloc_atomic_uncollectable(sizeof(int) * messageCount);
      int *curCtr;

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
	  currentLoadSize = total;
	  currentLoadSizeInBytes = total*pairSize;
	}

	curCtr = (int *) (done_ctr_array+count);
#if AM2_HAS_HUGE_SEGMENTS
	LONG_REQ(3,4,(remote_box, 
			       gasneti_handleridx(sparse_largeScatterNoDelete_request), (void *) (localCurBuf),
			       currentLoadSizeInBytes, 
                               (void *)(uintptr_t)(((char*)curAllocBuf)-TIC_AM_SEGOFFSET),
                               currentLoadSize, elem_sz,  
			       PACK(curCtr)));
	curAllocBuf = (void *) (((char *) curAllocBuf) + loadSizeInBytes);
#else
	MEDIUM_REQ(3,4,(remote_box, gasneti_handleridx(sparse_simpleScatter_request),
			 localCurBuf, currentLoadSizeInBytes, 
                         currentLoadSize, elem_sz,
                         PACK(curCtr)));
#endif
	total -= currentLoadSize;
	localCurBuf += loadSizeInBytes;
	count++;         
      }

      for (i = 0; i < messageCount; i++){
	GASNET_BLOCKUNTIL(done_ctr_array[i]);
      }

      ti_free((void *) done_ctr_array);
      ti_free((void *) localBuffer);

#if AM2_HAS_HUGE_SEGMENTS
      if (bufferSize > tic_prealloc){
	SHORT_REQ(1,2,(remote_box, gasneti_handleridx(misc_delete_request), 
			   PACK(remoteAllocBuf)));
      }
#endif
    }
  }
}
/* ------------------------------------------------------------------------------------ */
extern void sparse_scatter(void **remote_addr_list, void *src_data_list, 
                           int remote_box, int num_elem, int elem_sz, int atomic_elements) {
  if (tic_pipelining){
    sparse_scatter_pipeline(remote_addr_list, src_data_list, remote_box, num_elem, elem_sz, atomic_elements);
  }
  else{
    sparse_scatter_serial(remote_addr_list, src_data_list, remote_box, num_elem, elem_sz, atomic_elements);
  }
}
/* ------------------------------------------------------------------------------------ */
GASNETT_INLINE(sparse_simpleScatter_request)
void sparse_simpleScatter_request(gasnet_token_t token, void *addr_data_list, size_t addr_data_list_size,
			      int num_elem, int elem_sz, 
                              void *_done_ctr) {
  int i;
  void **addr_list = (void**)addr_data_list;
  int offset = num_elem * sizeof(void *);
  char *data = ((char*)addr_data_list) + offset;
  assert(addr_data_list_size == offset + num_elem * elem_sz);
  FAST_UNPACK(elem_sz, data, addr_list);
  SHORT_REP(1,2,(token, gasneti_handleridx(sparse_done_reply), PACK(_done_ctr)));
} 
TIC_MEDIUM_HANDLER(sparse_simpleScatter_request, 3, 4,
  (token,addr,nbytes, a0, a1, UNPACK(a2)    ),
  (token,addr,nbytes, a0, a1, UNPACK2(a2, a3)));
/* ------------------------------------------------------------------------------------ */
GASNETT_INLINE(sparse_done_reply)
void sparse_done_reply(gasnet_token_t token, void *_done_ctr) {
  int *done_ctr = (int *)_done_ctr;
  *done_ctr = 1;
}
TIC_SHORT_HANDLER(sparse_done_reply, 1, 2,
  (token, UNPACK(a0)    ),
  (token, UNPACK2(a0, a1)));
/* ------------------------------------------------------------------------------------ */
GASNETT_INLINE(sparse_generalScatter_request)
void sparse_generalScatter_request(gasnet_token_t token, void *addr_data_list,
                                  int num_elem, int elem_sz, 
                                  void *_done_ctr) {
  int i;
  void **addr_list = (void**)addr_data_list;
  int offset = num_elem * sizeof(void *);
  char *data = ((char*)addr_data_list) + offset;
  FAST_UNPACK(elem_sz, data, addr_list);
  SHORT_REP(1,2,(token, gasneti_handleridx(sparse_done_reply), PACK(_done_ctr)));
  if ((num_elem*elem_sz+offset) > tic_prealloc){
    ti_free_handlersafe((void*)addr_data_list);
  }
} 
TIC_SHORT_HANDLER(sparse_generalScatter_request, 4, 6,
  (token, UNPACK(a0),     a1, a2, UNPACK(a3)    ),
  (token, UNPACK2(a0, a1), a2, a3, UNPACK2(a4, a5)));
/* ------------------------------------------------------------------------------------ */
GASNETT_INLINE(sparse_largeScatterNoDelete_request)
void sparse_largeScatterNoDelete_request(gasnet_token_t token, void *addr_data_list, size_t addr_data_list_size, int num_elem, int elem_sz, void *_done_ctr){
  int i;
  void **addr_list = (void**)addr_data_list;
  int offset = num_elem * sizeof(void *);
  char *data = ((char*)addr_data_list) + offset;
  FAST_UNPACK(elem_sz, data, addr_list);
  SHORT_REP(1,2,(token, gasneti_handleridx(sparse_done_reply), PACK(_done_ctr)));
} 
TIC_LONG_HANDLER(sparse_largeScatterNoDelete_request, 3, 4,
  (token,addr,nbytes, a0, a1, UNPACK(a2)),
  (token,addr,nbytes, a0, a1, UNPACK2(a2, a3)));
/* ------------------------------------------------------------------------------------ */
GASNETT_INLINE(sparse_largeGather_request)
void sparse_largeGather_request(gasnet_token_t token, void *_addr_list, size_t addr_list_size,
				int num_elem, int elem_sz, 
				void *_tgt_data_list, void *_done_ctr,
				int atomic_elements) {
  int i;
  void **addr_list = (void**)_addr_list;
  int datasz = num_elem * elem_sz;
  int offset = num_elem * sizeof(void *);
  char *data = ((char*)_addr_list) + offset;
  
  assert(addr_list_size == sizeof(void *)*num_elem);
 
  FAST_PACK(elem_sz, data, addr_list);
  #if defined(USE_DISTRIBUTED_GC) && !defined(USE_GC_NONE)
    /* inform distributed GC that some pointers may be escaping */
    if (!atomic_elements)
      dgc_ptr_esc_all_gps(data, datasz, 1);
  #endif
  LONG_REP(1,2,(token, 
                gasneti_handleridx(sparse_largeGather_reply), data, datasz,
                (void *)(uintptr_t)(((char*)_tgt_data_list)-TIC_AM_SEGOFFSET),
                PACK(_done_ctr)));  
} 
TIC_LONG_HANDLER(sparse_largeGather_request, 5, 7,
  (token,addr,nbytes, a0, a1, UNPACK(a2),     UNPACK(a3),     a4),
  (token,addr,nbytes, a0, a1, UNPACK2(a2, a3), UNPACK2(a4, a5), a6));
/* ------------------------------------------------------------------------------------ */
GASNETT_INLINE(sparse_largeGather_reply)
void sparse_largeGather_reply(gasnet_token_t token, void *data_list, size_t data_list_size,
			      void *_done_ctr){
  int *done_ctr = (int *)_done_ctr;
  *done_ctr = 1;
}
TIC_LONG_HANDLER(sparse_largeGather_reply, 1, 2,
  (token,addr,nbytes, UNPACK(a0)    ),
  (token,addr,nbytes, UNPACK2(a0, a1)));
/* ------------------------------------------------------------------------------------ */
extern void sparse_gather_pipeline(void *tgt_data_list, void **remote_addr_list, 
				   int remote_box, int num_elem, int elem_sz, int atomic_elements) {
  volatile int done_ctr = 0;

  assert(tgt_data_list && remote_addr_list && 
         remote_box >= 0 && remote_box < BOXES &&
         num_elem > 0 && elem_sz > 0);

  if (num_elem * sizeof(void *) <= gasnet_AMMaxMedium() &&
      num_elem * elem_sz <= gasnet_AMMaxMedium()) { /* fast case - everything fits in a medium msg */
      MEDIUM_REQ(5,7,(remote_box, gasneti_handleridx(sparse_simpleGather_request),
			  remote_addr_list, num_elem * sizeof(void *), 
			  num_elem, elem_sz,
			  PACK(tgt_data_list), PACK(&done_ctr),
			  atomic_elements));
      GASNET_BLOCKUNTIL(done_ctr);
  } else { /*pipeline gather requests */
      int total = num_elem;
      int loadSize;
      int ptrLimit;
      int dataLimit;
      int count = 0;
      int messageCount;

#if AM2_HAS_HUGE_SEGMENTS
      void * volatile remoteAllocBuf = NULL;
      void *curAllocBuf = NULL;
      int bufferSize;
      int loadSizeInBytes;
      int bufferPadding;
      int lastLoadSize;
      ptrLimit = gasnet_AMMaxLongRequest() / sizeof(void *);
      dataLimit = gasnet_AMMaxLongReply() / elem_sz;
#else
      ptrLimit = gasnet_AMMaxMedium() / sizeof(void *);
      dataLimit = gasnet_AMMaxMedium() / elem_sz;
#endif
      if (ptrLimit > dataLimit){
	loadSize = dataLimit;
      }
      else{
	loadSize = ptrLimit;
      }
      
#if AM2_HAS_HUGE_SEGMENTS
      loadSizeInBytes = loadSize*(sizeof(void *) + elem_sz);
      bufferPadding = loadSizeInBytes % sizeof(void *);
      if (bufferPadding != 0){
	bufferPadding = sizeof(void *) - bufferPadding;
      }
      loadSizeInBytes += bufferPadding;
      lastLoadSize = num_elem % loadSize;
      if (lastLoadSize == 0){
	messageCount = num_elem / loadSize;
	bufferSize = loadSizeInBytes * messageCount;
      }
      else{
	messageCount = num_elem / loadSize + 1;
	bufferSize = loadSizeInBytes * (messageCount - 1) + lastLoadSize * (sizeof(void *) + elem_sz);
      }
      
      if (bufferSize <= tic_prealloc){
	/* use preallocate buffer if one is available */
	remoteAllocBuf = get_remote_buffer(bufferSize, remote_box);
      }
      else{
	/* get a buffer for this gather use only */
	SHORT_REQ(2,3,(remote_box, gasneti_handleridx(misc_alloc_request), 
			   bufferSize, PACK(&remoteAllocBuf)));
	GASNET_BLOCKUNTIL(remoteAllocBuf);
      }

      curAllocBuf = remoteAllocBuf;
#else
      if (num_elem % loadSize == 0){
	messageCount = num_elem / loadSize;
      }
      else{
	messageCount = num_elem / loadSize + 1;
      }
#endif

    {
      int i;
      int volatile * done_ctr_array = (int *)ti_malloc_atomic_uncollectable(sizeof(int) * messageCount);
      int *curCtr;

      for (i = 0; i < messageCount; i++){
	done_ctr_array[i] = 0;
      }

      while (total > 0){
	int currentLoadSize;

	if (total >= loadSize){
	  currentLoadSize = loadSize;
	}
	else{
	  currentLoadSize = total;
	}

	curCtr = (int *) (done_ctr_array+count);
#if AM2_HAS_HUGE_SEGMENTS
	LONG_REQ(5,7,(remote_box, 
			       gasneti_handleridx(sparse_largeGather_request), (void *) (remote_addr_list + count*loadSize),
			       currentLoadSize*sizeof(void*), 
                               (void *)(uintptr_t)(((char*)curAllocBuf)-TIC_AM_SEGOFFSET),
                               currentLoadSize, elem_sz, 
			       PACK(((char *) tgt_data_list + count*elem_sz*loadSize)), 
			       PACK(curCtr), atomic_elements));
	curAllocBuf = (void *) (((char *) curAllocBuf) + loadSizeInBytes);
#else
	MEDIUM_REQ(5,7,(remote_box, gasneti_handleridx(sparse_simpleGather_request),
			    (void *) (remote_addr_list + count*loadSize), currentLoadSize * sizeof(void *), 
			    currentLoadSize, elem_sz,
			    PACK(((char *) tgt_data_list + count*elem_sz*loadSize)), 
			    PACK(curCtr),
			    atomic_elements));
#endif
	total -= currentLoadSize;
	count++;         
      }

      for (i = 0; i < messageCount; i++){
	GASNET_BLOCKUNTIL(done_ctr_array[i]);
      }

      ti_free((void *) done_ctr_array);

#if AM2_HAS_HUGE_SEGMENTS
      if (bufferSize > tic_prealloc){
	SHORT_REQ(1,2,(remote_box, gasneti_handleridx(misc_delete_request), 
			   PACK(remoteAllocBuf)));
      }
#endif
    }
  }
}
/* ------------------------------------------------------------------------------------ */
void sparse_gather_serial(void *tgt_data_list, void **remote_addr_list, 
                          int remote_box, int num_elem, int elem_sz, int atomic_elements) {
  volatile int done_ctr = 0;

  assert(tgt_data_list && remote_addr_list && 
         remote_box >= 0 && remote_box < BOXES &&
         num_elem > 0 && elem_sz > 0);

  if (num_elem * sizeof(void *) <= gasnet_AMMaxMedium() &&
      num_elem * elem_sz <= gasnet_AMMaxMedium()) { /* fast case - everything fits in a medium msg */
    MEDIUM_REQ(5,7,(remote_box, gasneti_handleridx(sparse_simpleGather_request),
			 remote_addr_list, num_elem * sizeof(void *), 
                         num_elem, elem_sz,
                         PACK(tgt_data_list), PACK(&done_ctr),
                         atomic_elements));
    GASNET_BLOCKUNTIL(done_ctr);
  } else {  /* slower case - need multiple messages */
    int offset = num_elem * sizeof(void *);
    int datasz = offset + num_elem * elem_sz;
    void * volatile remoteAllocBuf = NULL;

    if (datasz <= tic_prealloc){
      /* use a preallocated buffer if one is available */
      remoteAllocBuf = get_remote_buffer(datasz, remote_box);
    }
    else{
      /* Allocate a buffer to hold the array data on the remote side. */
      SHORT_REQ(2,3,(remote_box, gasneti_handleridx(misc_alloc_request), 
			 datasz, PACK(&remoteAllocBuf)));
      GASNET_BLOCKUNTIL(remoteAllocBuf);
    }

    /* Transfer the addresses to the buffer with libtic (this could be optimized somewhat) */
    gasnet_put_bulk(remote_box, remoteAllocBuf, remote_addr_list, offset);

    /* Tell the remote side to gather the data. */
    SHORT_REQ(4,6,(remote_box, gasneti_handleridx(sparse_generalGather_request),
			 PACK(remoteAllocBuf), 
                         num_elem, elem_sz, 
                         PACK(&done_ctr)));
    GASNET_BLOCKUNTIL(done_ctr);

    /* Transfer the data from the buffer with libtic (this could be optimized somewhat) */
    gasnet_get_bulk(tgt_data_list, 
                 remote_box, ((char *)remoteAllocBuf)+offset,
                 num_elem * elem_sz); /* (does not handle gp escape) */

    if (datasz > tic_prealloc){
      SHORT_REQ(1,2,(remote_box, gasneti_handleridx(misc_delete_request), 
			 PACK(remoteAllocBuf)));
    }
  }
}
/* ------------------------------------------------------------------------------------ */
void sparse_gather(void *tgt_data_list, void **remote_addr_list, 
		   int remote_box, int num_elem, int elem_sz, int atomic_elements) {
  if (tic_pipelining){
    sparse_gather_pipeline(tgt_data_list, remote_addr_list, remote_box, num_elem, elem_sz, atomic_elements);
  }
  else{
    sparse_gather_serial(tgt_data_list, remote_addr_list, remote_box, num_elem, elem_sz, atomic_elements);
  }
}
/* ------------------------------------------------------------------------------------ */
GASNETT_INLINE(sparse_simpleGather_request)
void sparse_simpleGather_request(gasnet_token_t token, void *_addr_list, size_t addr_list_size,
			      int num_elem, int elem_sz, 
                              void *_tgt_data_list, void *_done_ctr,
                              int atomic_elements) {
  int i;
  void **addr_list = (void**)_addr_list;
  int datasz = num_elem * elem_sz;
  char *data = ti_malloc_handlersafe(datasz);
  assert(addr_list_size == sizeof(void *)*num_elem);
  FAST_PACK(elem_sz, data, addr_list);
  #if defined(USE_DISTRIBUTED_GC) && !defined(USE_GC_NONE)
    /* inform distributed GC that some pointers may be escaping */
    if (!atomic_elements)
      dgc_ptr_esc_all_gps(data, datasz, 1);
  #endif
  MEDIUM_REP(4,6,(token, gasneti_handleridx(sparse_simpleGather_reply),
		       data, datasz, 
                       num_elem, elem_sz,
                       PACK(_tgt_data_list), PACK(_done_ctr)));
  ti_free_handlersafe(data);
} 
TIC_MEDIUM_HANDLER(sparse_simpleGather_request, 5, 7,
  (token,addr,nbytes, a0, a1, UNPACK(a2),     UNPACK(a3),     a4),
  (token,addr,nbytes, a0, a1, UNPACK2(a2, a3), UNPACK2(a4, a5), a6));
/* ------------------------------------------------------------------------------------ */
GASNETT_INLINE(sparse_simpleGather_reply)
void sparse_simpleGather_reply(gasnet_token_t token, void *data_list, size_t data_list_size,
			      int num_elem, int elem_sz, 
                              void *_tgt_data_list, void *_done_ctr) {
  void *tgt_data_list = (void *)_tgt_data_list;
  int *done_ctr = (int *)_done_ctr;
  assert(data_list_size == num_elem * elem_sz);
  memcpy(tgt_data_list, data_list, data_list_size);
  *done_ctr = 1;
}
TIC_MEDIUM_HANDLER(sparse_simpleGather_reply, 4, 6,
  (token,addr,nbytes, a0, a1, UNPACK(a2),     UNPACK(a3)    ),
  (token,addr,nbytes, a0, a1, UNPACK2(a2, a3), UNPACK2(a4, a5)));
/* ------------------------------------------------------------------------------------ */
GASNETT_INLINE(sparse_generalGather_request)
void sparse_generalGather_request(gasnet_token_t token, void *addr_data_list,
                                  int num_elem, int elem_sz, 
                                  void *_done_ctr) {
  int i;
  void **addr_list = (void**)addr_data_list;
  int offset = num_elem * sizeof(void *);
  char *data = ((char*)addr_data_list) + offset;
  FAST_PACK(elem_sz, data, addr_list);
  SHORT_REP(1,2,(token, gasneti_handleridx(sparse_done_reply), PACK(_done_ctr)));
} 
TIC_SHORT_HANDLER(sparse_generalGather_request, 4, 6,
  (token, UNPACK(a0),     a1, a2, UNPACK(a3)    ),
  (token, UNPACK2(a0, a1), a2, a3, UNPACK2(a4, a5)));
/* ------------------------------------------------------------------------------------ */

/*
  read environment variables prealloc and pipelining
*/
void gather_init(){
  char *preallocstr;
  char *pipeliningstr;

  preallocstr = (char *) gasnet_getenv("TI_PREALLOC");
  if (preallocstr == NULL){
    tic_prealloc = 0;
  }
  else{
    tic_prealloc = atoi(preallocstr);
    assert(tic_prealloc >= 0);
    if (tic_prealloc > 0){
      buffer_init();
      /* convert KB to bytes */
      tic_prealloc *= 1024;
    }
  }
  pipeliningstr = (char *) gasnet_getenv("TI_PIPELINING");
  if (pipeliningstr == NULL){
    tic_pipelining = 0;
  }
  else{
    tic_pipelining = atoi(pipeliningstr);
  }
}
