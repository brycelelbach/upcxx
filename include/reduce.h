/**
 * reduce.h - provides higher-level interface to reductions
 * \see collective.h for low-level reduction interface
 */

#pragma once

#include <allocate.h>
#include <coll_flags.h>
#include <collective.h>
#include <team.h>

#define WRAPPER_DECL(T, num_class, type_code)           \
  template<> struct datatype_wrapper<T> {               \
    typedef T number_type;                              \
    typedef T num_class;                                \
    static const upcxx_datatype_t value = type_code;    \
  }

#define NUMBER_TYPE(T) typename datatype_wrapper<T>::number_type
#define INTEGER_TYPE(T) typename datatype_wrapper<T>::int_type

#define REDUCE_ALL_DECL(ret_type, op_name, op_code)     \
  template<class T> static ret_type op_name(T val) {    \
    return reduce_internal(val, op_code);               \
  }

#define REDUCE_TO_DECL(ret_type, op_name, op_code)              \
  template<class T> static ret_type op_name(T val, int root) {  \
    return reduce_internal(val, op_code, root);                 \
  }

#define REDUCE_BULK_ALL_DECL(ret_type, op_name, op_code)                \
  template<class T> static void op_name(T *src, T *dst, int count,      \
                                        ret_type * = 0) {               \
    reduce_internal(src, dst, count, op_code);                          \
  }

#define REDUCE_BULK_TO_DECL(ret_type, op_name, op_code)                 \
  template<class T> static void op_name(T *src, T *dst, int count,      \
                                        int root, ret_type * = 0) {     \
    reduce_internal(src, dst, count, op_code, root);                    \
  }

#define ARRAY_ELEM_TYPE(Array)                  \
  typename Array::local_elem_type

#define ARRAY_NUM_TYPE(Array)                   \
  NUMBER_TYPE(typename Array::local_elem_type)

#define ARRAY_INT_TYPE(Array)                   \
  INTEGER_TYPE(typename Array::local_elem_type)

#define REDUCE_ARRAY_NUM_ALL_DECL(op_name, op_code)                     \
  template<class Array> static void op_name(Array src,                  \
                                            Array dst,                  \
                                            ARRAY_NUM_TYPE(Array) * = 0) { \
    op_name(src.storage_ptr(), dst.storage_ptr(), src.size());          \
  }

#define REDUCE_ARRAY_INT_ALL_DECL(op_name, op_code)                     \
  template<class Array> static void op_name(Array src,                  \
                                            Array dst,                  \
                                            ARRAY_INT_TYPE(Array) * = 0) { \
    op_name(src.storage_ptr(), dst.storage_ptr(), src.size());          \
  }

#define REDUCE_ARRAY_NUM_TO_DECL(op_name, op_code)                      \
  template<class Array> static void op_name(Array src,                  \
                                            Array dst,                  \
                                            int root,                   \
                                            ARRAY_NUM_TYPE(Array) * = 0) { \
    op_name(src.storage_ptr(), dst.storage_ptr(), src.size(), root);    \
  }

#define REDUCE_ARRAY_INT_TO_DECL(op_name, op_code)                      \
  template<class Array> static void op_name(Array src,                  \
                                            Array dst,                  \
                                            int root,                   \
                                            ARRAY_INT_TYPE(Array) * = 0) { \
    op_name(src.storage_ptr(), dst.storage_ptr(), src.size(), root);    \
  }

#define REDUCE_NUM_DECLS(op_name, op_code)                   \
  REDUCE_ALL_DECL(NUMBER_TYPE(T), op_name, op_code)          \
  REDUCE_TO_DECL(NUMBER_TYPE(T), op_name, op_code)           \
  REDUCE_BULK_ALL_DECL(NUMBER_TYPE(T), op_name, op_code)     \
  REDUCE_BULK_TO_DECL(NUMBER_TYPE(T), op_name, op_code)      \
  REDUCE_ARRAY_NUM_ALL_DECL(op_name, op_code)                \
  REDUCE_ARRAY_NUM_TO_DECL(op_name, op_code)

#define REDUCE_INT_DECLS(op_name, op_code)                   \
  REDUCE_ALL_DECL(INTEGER_TYPE(T), op_name, op_code)         \
  REDUCE_TO_DECL(INTEGER_TYPE(T), op_name, op_code)          \
  REDUCE_BULK_ALL_DECL(INTEGER_TYPE(T), op_name, op_code)    \
  REDUCE_BULK_TO_DECL(INTEGER_TYPE(T), op_name, op_code)     \
  REDUCE_ARRAY_INT_ALL_DECL(op_name, op_code)                \
  REDUCE_ARRAY_INT_TO_DECL(op_name, op_code)

namespace upcxx {

  template<class T> struct datatype_wrapper {};
  WRAPPER_DECL(char, int_type, UPCXX_CHAR);
  WRAPPER_DECL(unsigned char, int_type, UPCXX_UCHAR);
  WRAPPER_DECL(short, int_type, UPCXX_SHORT);
  WRAPPER_DECL(unsigned short, int_type, UPCXX_USHORT);
  WRAPPER_DECL(int, int_type, UPCXX_INT);
  WRAPPER_DECL(unsigned int, int_type, UPCXX_UINT);
  WRAPPER_DECL(long, int_type, UPCXX_LONG);
  WRAPPER_DECL(unsigned long, int_type, UPCXX_ULONG);
  WRAPPER_DECL(long long, int_type, UPCXX_LONG_LONG);
  WRAPPER_DECL(unsigned long long, int_type, UPCXX_ULONG_LONG);
  WRAPPER_DECL(float, float_type, UPCXX_FLOAT);
  WRAPPER_DECL(double, float_type, UPCXX_DOUBLE);

  class reduce {
  private:
    template<class T> static T reduce_internal(T val, upcxx_op_t op) {
      static T *src, *dst;
      if (!src) {
        src = (T *) gasnet_seg_alloc(2 * sizeof(T));
        dst = src + 1;
      }
      src[0] = reduce_internal(val, op, 0);
      upcxx_bcast(src, dst, sizeof(T), 0);
      return dst[0];
    }

    template<class T> static T reduce_internal(T val, upcxx_op_t op,
                                               int root) {
      static T *src, *dst;
      if (!src) {
        src = (T *) gasnet_seg_alloc(2 * sizeof(T));
        dst = src + 1;
      }
      src[0] = val;
      upcxx_reduce(src, dst, 1, root, op, datatype_wrapper<T>::value);
      return dst[0];
    }

    template<class T> static void reduce_internal(T *src, T *dst,
                                                  int count,
                                                  upcxx_op_t op) {
      reduce_internal(src, dst, count, op, 0);
      upcxx_bcast(dst, dst, count * sizeof(T), 0);
    }

    template<class T> static void reduce_internal(T *src, T *dst,
                                                  int count,
                                                  upcxx_op_t op,
                                                  int root) {
      upcxx_reduce(src, dst, count, root, op, datatype_wrapper<T>::value);
    }

  public:
    REDUCE_NUM_DECLS(add,  UPCXX_SUM)
    REDUCE_NUM_DECLS(mult, UPCXX_PROD)
    REDUCE_NUM_DECLS(max,  UPCXX_MAX)
    REDUCE_NUM_DECLS(min,  UPCXX_MIN)

    REDUCE_INT_DECLS(lor,  UPCXX_LOR)
    REDUCE_INT_DECLS(lxor, UPCXX_LXOR)
    REDUCE_INT_DECLS(land, UPCXX_LAND)
    REDUCE_INT_DECLS(bor,  UPCXX_BOR)
    REDUCE_INT_DECLS(bxor, UPCXX_BXOR)
    REDUCE_INT_DECLS(band, UPCXX_BAND)
  };

} /* namespace upcxx */

#undef WRAPPER_DECL
#undef NUMBER_TYPE
#undef INTEGER_TYPE
#undef REDUCE_ALL_DECL
#undef REDUCE_TO_DECL
#undef REDUCE_BULK_ALL_DECL
#undef REDUCE_BULK_TO_DECL
#undef ARRAY_ELEM_TYPE
#undef ARRAY_NUM_TYPE
#undef ARRAY_INT_TYPE
#undef REDUCE_ARRAY_NUM_ALL_DECL
#undef REDUCE_ARRAY_INT_ALL_DECL
#undef REDUCE_ARRAY_NUM_TO_DECL
#undef REDUCE_ARRAY_INT_TO_DECL
#undef REDUCE_NUM_DECLS
#undef REDUCE_INT_DECLS
