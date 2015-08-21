/**
 * reduce.h - provides higher-level interface to reductions
 * \see collective.h for low-level reduction interface
 */

#pragma once

#include <upcxx/allocate.h>
#include <upcxx/coll_flags.h>
#include <upcxx/collective.h>
#include <upcxx/interfaces_internal.h>
//#include <team.h>

#define UPCXXR_WRAPPER_DECL(T, num_class, type_code)    \
  template<> struct datatype_wrapper<T> {               \
    typedef T number_type;                              \
    typedef T num_class;                                \
    static const upcxx_datatype_t value = type_code;    \
  }

#define UPCXXR_NUMBER_TYPE(T)                   \
  typename datatype_wrapper<T>::number_type
#define UPCXXR_INTEGER_TYPE(T)                  \
  typename datatype_wrapper<T>::int_type

#define UPCXXR_REDUCE_ALL_DECL(ret_type, op_name, op_code)      \
  template<class T> static ret_type op_name(T val) {            \
    return reduce_internal(val, op_code);                       \
  }

#define UPCXXR_REDUCE_TO_DECL(ret_type, op_name, op_code)       \
  template<class T> static ret_type op_name(T val, int root) {  \
    return reduce_internal(val, op_code, root);                 \
  }

#define UPCXXR_REDUCE_BULK_ALL_DECL(ret_type, op_name, op_code)         \
  template<class T> static void op_name(T *src, T *dst, int count,      \
                                        ret_type * = 0) {               \
    reduce_internal(src, dst, count, op_code);                          \
  }

#define UPCXXR_REDUCE_BULK_TO_DECL(ret_type, op_name, op_code)          \
  template<class T> static void op_name(T *src, T *dst, int count,      \
                                        int root, ret_type * = 0) {     \
    reduce_internal(src, dst, count, op_code, root);                    \
  }

#define UPCXXR_ARRAY_NUM_TYPE(Array)                    \
  UPCXXR_NUMBER_TYPE(UPCXXI_ARRAY_ELEM_TYPE(Array))

#define UPCXXR_ARRAY_INT_TYPE(Array)                    \
  UPCXXR_INTEGER_TYPE(UPCXXI_ARRAY_ELEM_TYPE(Array))

#define UPCXXR_REDUCE_ARRAY_NUM_ALL_DECL(op_name, op_code)      \
  template<class Array>                                         \
  static void op_name(Array src, Array dst,                     \
                      UPCXXR_ARRAY_NUM_TYPE(Array) * = 0) {     \
    op_name(src.storage_ptr(), dst.storage_ptr(), src.size());  \
  }

#define UPCXXR_REDUCE_ARRAY_INT_ALL_DECL(op_name, op_code)      \
  template<class Array>                                         \
  static void op_name(Array src, Array dst,                     \
                      UPCXXR_ARRAY_INT_TYPE(Array) * = 0) {     \
    op_name(src.storage_ptr(), dst.storage_ptr(), src.size());  \
  }

#define UPCXXR_REDUCE_ARRAY_NUM_TO_DECL(op_name, op_code)     \
  template<class Array>                                       \
  static void op_name(Array src, Array dst, int root,         \
                      UPCXXR_ARRAY_NUM_TYPE(Array) * = 0) {   \
    op_name(src.storage_ptr(), dst.storage_ptr(), src.size(), \
            root);                                            \
  }

#define UPCXXR_REDUCE_ARRAY_INT_TO_DECL(op_name, op_code)     \
  template<class Array>                                       \
  static void op_name(Array src, Array dst, int root,         \
                      UPCXXR_ARRAY_INT_TYPE(Array) * = 0) {   \
    op_name(src.storage_ptr(), dst.storage_ptr(), src.size(), \
            root);                                            \
  }

#define UPCXXR_REDUCE_NUM_DECLS(op_name, op_code)                       \
  UPCXXR_REDUCE_ALL_DECL(UPCXXR_NUMBER_TYPE(T), op_name, op_code)       \
  UPCXXR_REDUCE_TO_DECL(UPCXXR_NUMBER_TYPE(T), op_name, op_code)        \
  UPCXXR_REDUCE_BULK_ALL_DECL(UPCXXR_NUMBER_TYPE(T), op_name, op_code)  \
  UPCXXR_REDUCE_BULK_TO_DECL(UPCXXR_NUMBER_TYPE(T), op_name, op_code)   \
  UPCXXR_REDUCE_ARRAY_NUM_ALL_DECL(op_name, op_code)                    \
  UPCXXR_REDUCE_ARRAY_NUM_TO_DECL(op_name, op_code)

#define UPCXXR_REDUCE_INT_DECLS(op_name, op_code)                       \
  UPCXXR_REDUCE_ALL_DECL(UPCXXR_INTEGER_TYPE(T), op_name, op_code)      \
  UPCXXR_REDUCE_TO_DECL(UPCXXR_INTEGER_TYPE(T), op_name, op_code)       \
  UPCXXR_REDUCE_BULK_ALL_DECL(UPCXXR_INTEGER_TYPE(T), op_name, op_code) \
  UPCXXR_REDUCE_BULK_TO_DECL(UPCXXR_INTEGER_TYPE(T), op_name, op_code)  \
  UPCXXR_REDUCE_ARRAY_INT_ALL_DECL(op_name, op_code)                    \
  UPCXXR_REDUCE_ARRAY_INT_TO_DECL(op_name, op_code)

namespace upcxx {

  template<class T> struct datatype_wrapper {};
  UPCXXR_WRAPPER_DECL(char, int_type, UPCXX_CHAR);
  UPCXXR_WRAPPER_DECL(unsigned char, int_type, UPCXX_UCHAR);
  UPCXXR_WRAPPER_DECL(short, int_type, UPCXX_SHORT);
  UPCXXR_WRAPPER_DECL(unsigned short, int_type, UPCXX_USHORT);
  UPCXXR_WRAPPER_DECL(int, int_type, UPCXX_INT);
  UPCXXR_WRAPPER_DECL(unsigned int, int_type, UPCXX_UINT);
  UPCXXR_WRAPPER_DECL(long, int_type, UPCXX_LONG);
  UPCXXR_WRAPPER_DECL(unsigned long, int_type, UPCXX_ULONG);
  UPCXXR_WRAPPER_DECL(long long, int_type, UPCXX_LONG_LONG);
  UPCXXR_WRAPPER_DECL(unsigned long long, int_type, UPCXX_ULONG_LONG);
  UPCXXR_WRAPPER_DECL(float, float_type, UPCXX_FLOAT);
  UPCXXR_WRAPPER_DECL(double, float_type, UPCXX_DOUBLE);

  class reduce {
  private:
    template<class T> static T reduce_internal(T val, upcxx_op_t op) {
      static T *src, *dst;
      if (!src) {
        src = upcxx::allocate<T>(2); // YZ: memory leaking?
        dst = src + 1;
      }
      src[0] = reduce_internal(val, op, 0);
      upcxx::bcast(src, dst, sizeof(T), 0);
      return dst[0];
    }

    template<class T> static T reduce_internal(T val, upcxx_op_t op,
                                               int root) {
      static T *src, *dst;
      if (!src) {
        src = upcxx::allocate<T>(2); // YZ: memory leaking?
        dst = src + 1;
      }
      src[0] = val;
      upcxx::reduce(src, dst, 1, root, op, datatype_wrapper<T>::value);
      return dst[0];
    }

    template<class T> static void reduce_internal(T *src, T *dst,
                                                  int count,
                                                  upcxx_op_t op) {
      reduce_internal(src, dst, count, op, 0);
      upcxx::bcast(dst, dst, count * sizeof(T), 0);
    }

    template<class T> static void reduce_internal(T *src, T *dst,
                                                  int count,
                                                  upcxx_op_t op,
                                                  int root) {
      upcxx::reduce(src, dst, count, root, op, datatype_wrapper<T>::value);
    }

  public:
    UPCXXR_REDUCE_NUM_DECLS(add,  UPCXX_SUM)
    UPCXXR_REDUCE_NUM_DECLS(mult, UPCXX_PROD)
    UPCXXR_REDUCE_NUM_DECLS(max,  UPCXX_MAX)
    UPCXXR_REDUCE_NUM_DECLS(min,  UPCXX_MIN)

    UPCXXR_REDUCE_INT_DECLS(lor,  UPCXX_LOR)
    UPCXXR_REDUCE_INT_DECLS(lxor, UPCXX_LXOR)
    UPCXXR_REDUCE_INT_DECLS(land, UPCXX_LAND)
    UPCXXR_REDUCE_INT_DECLS(bor,  UPCXX_BOR)
    UPCXXR_REDUCE_INT_DECLS(bxor, UPCXX_BXOR)
    UPCXXR_REDUCE_INT_DECLS(band, UPCXX_BAND)
  };

} /* namespace upcxx */

#undef UPCXXR_WRAPPER_DECL
#undef UPCXXR_NUMBER_TYPE
#undef UPCXXR_INTEGER_TYPE
#undef UPCXXR_REDUCE_ALL_DECL
#undef UPCXXR_REDUCE_TO_DECL
#undef UPCXXR_REDUCE_BULK_ALL_DECL
#undef UPCXXR_REDUCE_BULK_TO_DECL
#undef UPCXXR_ARRAY_NUM_TYPE
#undef UPCXXR_ARRAY_INT_TYPE
#undef UPCXXR_REDUCE_ARRAY_NUM_ALL_DECL
#undef UPCXXR_REDUCE_ARRAY_INT_ALL_DECL
#undef UPCXXR_REDUCE_ARRAY_NUM_TO_DECL
#undef UPCXXR_REDUCE_ARRAY_INT_TO_DECL
#undef UPCXXR_REDUCE_NUM_DECLS
#undef UPCXXR_REDUCE_INT_DECLS
