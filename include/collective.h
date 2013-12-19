/**
 * collecitve.h - provide global collective operations
 * \see team.h for team collective operations
 */

#pragma once

#include <assert.h>

#include "gasnet_api.h"
#include "upcxx_types.h"
#include "coll_flags.h"
#include "team.h"

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

extern const char *upcxx_op_strs[];

namespace upcxx
{
  template<class T>
  void _int_reduce_fn(void *results, size_t result_count,
                      const void *left_operands, size_t left_count,
                      const void *right_operands,
                      size_t elem_size, int flags, int arg)
  {
    size_t i;
    T *res = (T *) results;
    const T *src1 = (const T *)left_operands;
    const T *src2 = (const T *)right_operands;
    assert(elem_size == sizeof(T));
    assert(result_count == left_count);

    upcxx_op_t op_t = (upcxx_op_t)arg;
    switch(op_t) {
    case UPCXX_SUM:
      for(i=0; i<result_count; i++) {
        res[i] = src1[i] + src2[i];
      } break;
    case UPCXX_PROD:
      for(i=0; i<result_count; i++) {
        res[i] = src1[i] * src2[i];
      } break;
    case UPCXX_MAX:
      for(i=0; i<result_count; i++) {
        res[i] = MAX(src1[i], src2[i]);
      } break;
    case UPCXX_MIN:
      for(i=0; i<result_count; i++) {
        res[i] = MIN(src1[i], src2[i]);
      } break;
    case UPCXX_LAND:
      for(i=0; i<result_count; i++) {
        res[i] = src1[i] && src2[i];
      } break;
    case UPCXX_BAND:
      for(i=0; i<result_count; i++) {
        res[i] = src1[i] & src2[i];
      } break;
    case UPCXX_LOR:
      for(i=0; i<result_count; i++) {
        res[i] = src1[i] || src2[i];
      } break;
    case UPCXX_BOR:
      for(i=0; i<result_count; i++) {
        res[i] = src1[i] | src2[i];
      } break;
    case UPCXX_LXOR:
      for(i=0; i<result_count; i++) {
        res[i] = !(src1[i] == src2[i]);
      } break;
    case UPCXX_BXOR:
      for(i=0; i<result_count; i++) {
        res[i] = src1[i] ^ src2[i];
      } break;
    default:
      fprintf(stderr, "Reduction op %s is not supported yet!\n",
              upcxx_op_strs[op_t]);
      exit(1);
    }
  } // end of _int_reduce_fn

  template<class T>
  void _float_reduce_fn(void *results, size_t result_count,
                        const void *left_operands, size_t left_count,
                        const void *right_operands,
                        size_t elem_size, int flags, int arg)
  {
    size_t i;
    T *res = (T *) results;
    const T *src1 = (const T *)left_operands;
    const T *src2 = (const T *)right_operands;
    assert(elem_size == sizeof(T));
    assert(result_count == left_count);
    
    upcxx_op_t op_t = (upcxx_op_t)arg;
    switch(op_t) {
    case UPCXX_SUM:
      for(i=0; i<result_count; i++) {
        res[i] = src1[i] + src2[i];
      } break;
    case UPCXX_PROD:
      for(i=0; i<result_count; i++) {
        res[i] = src1[i] * src2[i];
      } break;
    case UPCXX_MAX:
      for(i=0; i<result_count; i++) {
        res[i] = MAX(src1[i], src2[i]);
      } break;
    case UPCXX_MIN:
      for(i=0; i<result_count; i++) {
        res[i] = MIN(src1[i], src2[i]);
      } break;
    default:
      fprintf(stderr, "Reduction op %s is not supported yet!\n",
              upcxx_op_strs[op_t]);
      exit(1);
    }
  } // end of _float_reduce_fn
  
  template<class T>
  void upcxx_reduce(T *src, T *dst, size_t count, uint32_t root,
                    upcxx_op_t op, upcxx_datatype_t dt)
  {
    //    void _gasnet_coll_reduce(gasnet_team_handle_t team,
    //                             gasnet_image_t dstimage, void *dst,
    //                             void *src, size_t src_blksz, size_t src_offset,
    //                             size_t elem_size, size_t elem_count,
    //                             gasnet_coll_fn_handle_t func, int func_arg,
    //                             int flags GASNETE_THREAD_FARG) ;

    // YZ: check consistency of T and dt
    gasnet_coll_reduce(CURRENT_GASNET_TEAM, root, dst, src, 0, 0, sizeof(T),
                       count, dt, op, UPCXX_GASNET_COLL_FLAG);
  }

  static inline void init_collectives()
  {
    gasnet_coll_fn_entry_t fntable[UPCXX_DATATYPE_COUNT];

    fntable[UPCXX_CHAR].fnptr = _int_reduce_fn<char>;
    fntable[UPCXX_CHAR].flags = 0;
    fntable[UPCXX_UCHAR].fnptr = _int_reduce_fn<unsigned char>;
    fntable[UPCXX_UCHAR].flags = 0;
    fntable[UPCXX_SHORT].fnptr = _int_reduce_fn<short>;
    fntable[UPCXX_SHORT].flags = 0;
    fntable[UPCXX_USHORT].fnptr = _int_reduce_fn<unsigned short>;
    fntable[UPCXX_USHORT].flags = 0;
    fntable[UPCXX_INT].fnptr = _int_reduce_fn<int>;
    fntable[UPCXX_INT].flags = 0;
    fntable[UPCXX_UINT].fnptr = _int_reduce_fn<unsigned int>;
    fntable[UPCXX_UINT].flags = 0;
    fntable[UPCXX_LONG].fnptr = _int_reduce_fn<long>;
    fntable[UPCXX_LONG].flags = 0;
    fntable[UPCXX_ULONG].fnptr = _int_reduce_fn<unsigned long>;
    fntable[UPCXX_ULONG].flags = 0;
    fntable[UPCXX_LONG_LONG].fnptr = _int_reduce_fn<long long>;
    fntable[UPCXX_LONG_LONG].flags = 0;
    fntable[UPCXX_ULONG_LONG].fnptr = _int_reduce_fn<unsigned long long>;
    fntable[UPCXX_ULONG_LONG].flags = 0;
    fntable[UPCXX_FLOAT].fnptr = _float_reduce_fn<float>;
    fntable[UPCXX_FLOAT].flags = 0;
    fntable[UPCXX_DOUBLE].fnptr = _float_reduce_fn<double>;
    fntable[UPCXX_DOUBLE].flags = 0;

#if GASNET_PAR
#error "UPCXX Lib doens't support collectives in GASNET_PAR mode for now!"
#else
    gasnet_coll_init(NULL, 0, fntable, UPCXX_DATATYPE_COUNT, 0);
#endif
  }
  
  static inline void upcxx_bcast(void *src, void *dst, size_t nbytes, uint32_t root)
  {
    gasnet_coll_broadcast(CURRENT_GASNET_TEAM, dst, root, src, nbytes, 
                          UPCXX_GASNET_COLL_FLAG);
  }
  
  static inline void upcxx_gather(void *src, void *dst, size_t nbytes, uint32_t root)
  {
    gasnet_coll_gather(CURRENT_GASNET_TEAM, root, dst, src, nbytes,
                       UPCXX_GASNET_COLL_FLAG);
  }
  
  static inline void upcxx_allgather(void *src, void *dst, size_t nbytes)
  {
    gasnet_coll_gather_all(CURRENT_GASNET_TEAM, dst, src, nbytes,
                           UPCXX_GASNET_COLL_FLAG);
  }

  static inline void upcxx_alltoall(void *src, void *dst, size_t nbytes)
  {
    gasnet_coll_exchange(CURRENT_GASNET_TEAM, dst, src, nbytes,
                         UPCXX_GASNET_COLL_FLAG);
  }
  
} // end of namespace upcxx
