/**
 * UPCXX collectives
 */

#include <assert.h>
#include "collective.h"

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

extern const char *upcxx_op_strs[];

namespace upcxx {
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

  void init_collectives()
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

} // end of namespace upcxx
