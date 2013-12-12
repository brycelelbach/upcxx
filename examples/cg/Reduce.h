#pragma once

#include <collective.h>

#ifdef USE_TEAMS
# include "Team.h"
# define CURRENT_TEAM Team::currentTeam()->gasnet_team()
#else
# define CURRENT_TEAM GASNET_TEAM_ALL
#endif

#define UPCXX_GASNET_COLL_FLAG \
  (GASNET_COLL_IN_MYSYNC | GASNET_COLL_OUT_MYSYNC | GASNET_COLL_LOCAL)

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
    return reduce(val, op_code);                        \
  }

#define REDUCE_TO_DECL(ret_type, op_name, op_code)              \
  template<class T> static ret_type op_name(T val, int root) {  \
    return reduce(val, op_code, root);                          \
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
    return reduce(src, dst, op_code);                                   \
  }

#define REDUCE_ARRAY_INT_ALL_DECL(op_name, op_code)                     \
  template<class Array> static void op_name(Array src,                  \
                                            Array dst,                  \
                                            ARRAY_INT_TYPE(Array) * = 0) { \
    return reduce(src, dst, op_code);                                   \
  }

#define REDUCE_ARRAY_NUM_TO_DECL(op_name, op_code)                      \
  template<class Array> static void op_name(Array src,                  \
                                            Array dst,                  \
                                            int root,                   \
                                            ARRAY_NUM_TYPE(Array) * = 0) { \
    return reduce(src, dst, op_code, root);                             \
  }

#define REDUCE_ARRAY_INT_TO_DECL(op_name, op_code)                      \
  template<class Array> static void op_name(Array src,                  \
                                            Array dst,                  \
                                            int root,                   \
                                            ARRAY_INT_TYPE(Array) * = 0) { \
    return reduce(src, dst, op_code, root);                             \
  }

#define REDUCE_NUM_DECLS(op_name, op_code)              \
  REDUCE_ALL_DECL(NUMBER_TYPE(T), op_name, op_code)     \
  REDUCE_TO_DECL(NUMBER_TYPE(T), op_name, op_code)      \
  REDUCE_ARRAY_NUM_ALL_DECL(op_name, op_code)           \
  REDUCE_ARRAY_NUM_TO_DECL(op_name, op_code)

#define REDUCE_INT_DECLS(op_name, op_code)              \
  REDUCE_ALL_DECL(INTEGER_TYPE(T), op_name, op_code)    \
  REDUCE_TO_DECL(INTEGER_TYPE(T), op_name, op_code)     \
  REDUCE_ARRAY_INT_ALL_DECL(op_name, op_code)           \
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

  class Reduce {
  private:
    template<class T> static T reduce(T val, upcxx_op_t op) {
      T redval = reduce(val, op, 0);
      T bcval;
      /* upcxx_bcast(&redval, &bcval, sizeof(T), 0); */
      gasnet_coll_broadcast(CURRENT_TEAM,
                            &bcval, 0, &redval, sizeof(T),
                            UPCXX_GASNET_COLL_FLAG);
      return bcval;
    }

    template<class T> static T reduce(T val, upcxx_op_t op, int root) {
      T redval;
      /* upcxx_reduce(&val, &redval, 1, root, op, datatype_wrapper<T>::value); */
      gasnet_coll_reduce(CURRENT_TEAM,
                         root, &redval, &val, 0, 0, sizeof(T), 1,
                         datatype_wrapper<T>::value, op,
                         UPCXX_GASNET_COLL_FLAG);
      return redval;
    }

    template<class Array> static void reduce(Array src,
                                             Array dst,
                                             upcxx_op_t op) {
      reduce(src, dst, op, 0);
      /* upcxx_bcast(dst.storage_ptr(), dst.storage_ptr(), */
      /*             dst.size() * sizeof(T), 0); */
      gasnet_coll_broadcast(CURRENT_TEAM,
                            dst.storage_ptr(), 0, src.storage_ptr(),
                            dst.size() * sizeof(ARRAY_ELEM_TYPE(Array)),
                            UPCXX_GASNET_COLL_FLAG);
    }

    template<class Array> static void reduce(Array src,
                                             Array dst,
                                             upcxx_op_t op,
                                             int root) {
      /* upcxx_reduce(src.storage_ptr(), dst.storage_ptr(), src.size(), */
      /*              root, op, datatype_wrapper<T>::value); */
      gasnet_coll_reduce(CURRENT_TEAM,
                         root, dst.storage_ptr(), src.storage_ptr(),
                         0, 0, sizeof(ARRAY_ELEM_TYPE(Array)), src.size(),
                         datatype_wrapper<ARRAY_ELEM_TYPE(Array)>::value,
                         op, UPCXX_GASNET_COLL_FLAG);
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

#undef UPCXX_GASNET_COLL_FLAG
#undef WRAPPER_DECL
#undef NUMBER_TYPE
#undef INTEGER_TYPE
#undef REDUCE_ALL_DECL
#undef REDUCE_TO_DECL
#undef ARRAY_ELEM_TYPE
#undef ARRAY_NUM_TYPE
#undef ARRAY_INT_TYPE
#undef REDUCE_ARRAY_NUM_ALL_DECL
#undef REDUCE_ARRAY_INT_ALL_DECL
#undef REDUCE_ARRAY_NUM_TO_DECL
#undef REDUCE_ARRAY_INT_TO_DECL
#undef REDUCE_NUM_DECLS
#undef REDUCE_INT_DECLS
