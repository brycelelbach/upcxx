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

#define REDUCE_ARRAY_ALL_DECL(val_type, op_name, op_code)               \
  template<class T, int N> static void op_name(ndarray<T, N> src,       \
                                               ndarray<T, N> dst,       \
                                               val_type * = 0) {        \
    return reduce(src, dst, op_code);                                   \
  }

#define REDUCE_ARRAY_TO_DECL(val_type, op_name, op_code)                \
  template<class T, int N> static void op_name(ndarray<T, N> src,       \
                                               ndarray<T, N> dst,       \
                                               int root,                \
                                               val_type * = 0) {        \
    return reduce(src, dst, op_code, root);                             \
  }

#define REDUCE_DECLS(val_type, op_name, op_code)        \
  REDUCE_ALL_DECL(val_type, op_name, op_code)           \
  REDUCE_TO_DECL(val_type, op_name, op_code)            \
  REDUCE_ARRAY_ALL_DECL(val_type, op_name, op_code)     \
  REDUCE_ARRAY_TO_DECL(val_type, op_name, op_code)

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

    template<class T, int N> static void reduce(ndarray<T, N> src,
                                                ndarray<T, N> dst,
                                                upcxx_op_t op) {
      reduce(src, dst, op, 0);
      /* upcxx_bcast(dst.storage_ptr(), dst.storage_ptr(), */
      /*             dst.size() * sizeof(T), 0); */
      gasnet_coll_broadcast(CURRENT_TEAM,
                            dst.storage_ptr(), 0, src.storage_ptr(),
                            dst.size() * sizeof(T),
                            UPCXX_GASNET_COLL_FLAG);
    }

    template<class T, int N> static void reduce(ndarray<T, N> src,
                                                ndarray<T, N> dst,
                                                upcxx_op_t op,
                                                int root) {
      /* upcxx_reduce(src.storage_ptr(), dst.storage_ptr(), src.size(), */
      /*              root, op, datatype_wrapper<T>::value); */
      gasnet_coll_reduce(CURRENT_TEAM,
                         root, dst.storage_ptr(), src.storage_ptr(),
                         0, 0, sizeof(T), src.size(),
                         datatype_wrapper<T>::value, op,
                         UPCXX_GASNET_COLL_FLAG);
    }

  public:
    REDUCE_DECLS(NUMBER_TYPE(T), add,  UPCXX_SUM)
    REDUCE_DECLS(NUMBER_TYPE(T), mult, UPCXX_PROD)
    REDUCE_DECLS(NUMBER_TYPE(T), max,  UPCXX_MAX)
    REDUCE_DECLS(NUMBER_TYPE(T), min,  UPCXX_MIN)

    REDUCE_DECLS(INTEGER_TYPE(T), lor,  UPCXX_LOR)
    REDUCE_DECLS(INTEGER_TYPE(T), lxor, UPCXX_LXOR)
    REDUCE_DECLS(INTEGER_TYPE(T), land, UPCXX_LAND)
    REDUCE_DECLS(INTEGER_TYPE(T), bor,  UPCXX_BOR)
    REDUCE_DECLS(INTEGER_TYPE(T), bxor, UPCXX_BXOR)
    REDUCE_DECLS(INTEGER_TYPE(T), band, UPCXX_BAND)
  };

} /* namespace upcxx */

#undef UPCXX_GASNET_COLL_FLAG
#undef WRAPPER_DECL
#undef NUMBER_TYPE
#undef INTEGER_TYPE
#undef REDUCE_ALL_DECL
#undef REDUCE_TO_DECL
#undef REDUCE_ARRAY_ALL_DECL
#undef REDUCE_ARRAY_TO_DECL
#undef REDUCE_DECLS
