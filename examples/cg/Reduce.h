#pragma once

#include <collective.h>

template<class T> struct datatype_wrapper {};
template<> struct datatype_wrapper<char> {
  typedef char number_type;
  typedef char int_type;
  static const upcxx_datatype_t value = UPCXX_CHAR;
};
template<> struct datatype_wrapper<unsigned char> {
  typedef unsigned char number_type;
  typedef unsigned char int_type;
  static const upcxx_datatype_t value = UPCXX_UCHAR;
};
template<> struct datatype_wrapper<short> {
  typedef short number_type;
  typedef short int_type;
  static const upcxx_datatype_t value = UPCXX_SHORT;
};
template<> struct datatype_wrapper<unsigned short> {
  typedef unsigned short number_type;
  typedef unsigned short int_type;
  static const upcxx_datatype_t value = UPCXX_USHORT;
};
template<> struct datatype_wrapper<int> {
  typedef int number_type;
  typedef int int_type;
  static const upcxx_datatype_t value = UPCXX_INT;
};
template<> struct datatype_wrapper<unsigned int> {
  typedef unsigned int number_type;
  typedef unsigned int int_type;
  static const upcxx_datatype_t value = UPCXX_UINT;
};
template<> struct datatype_wrapper<long> {
  typedef long number_type;
  typedef long int_type;
  static const upcxx_datatype_t value = UPCXX_LONG;
};
template<> struct datatype_wrapper<unsigned long> {
  typedef unsigned long number_type;
  typedef unsigned long int_type;
  static const upcxx_datatype_t value = UPCXX_ULONG;
};
template<> struct datatype_wrapper<long long> {
  typedef long long number_type;
  typedef long long int_type;
  static const upcxx_datatype_t value = UPCXX_LONG_LONG;
};
template<> struct datatype_wrapper<unsigned long long> {
  typedef unsigned long long number_type;
  typedef unsigned long long int_type;
  static const upcxx_datatype_t value = UPCXX_ULONG_LONG;
};
template<> struct datatype_wrapper<float> {
  typedef float number_type;
  typedef float float_type;
  static const upcxx_datatype_t value = UPCXX_FLOAT;
};
template<> struct datatype_wrapper<double> {
  typedef double number_type;
  typedef double float_type;
  static const upcxx_datatype_t value = UPCXX_DOUBLE;
};

#define NUMBER_TYPE(T) typename datatype_wrapper<T>::number_type
#define INTEGER_TYPE(T) typename datatype_wrapper<T>::int_type

class Reduce {
 private:
  template<class T> static T reduce(T val, upcxx_op_t op) {
    T redval = reduce(val, op, 0);
    T bcval;
    upcxx::upcxx_bcast(&redval, &bcval, sizeof(T), 0);
    return bcval;
  }
  template<class T> static T reduce(T val, upcxx_op_t op, int dst) {
    T redval;
    upcxx::upcxx_reduce(&val, &redval, 1, dst, op, datatype_wrapper<T>::value);
    return redval;
  }

 public:
  template<class T> static NUMBER_TYPE(T) add(T val) {
    return reduce(val, UPCXX_SUM);
  }
  template<class T> static NUMBER_TYPE(T) mult(T val) {
    return reduce(val, UPCXX_PROD);
  }
  template<class T> static NUMBER_TYPE(T) max(T val) {
    return reduce(val, UPCXX_MAX);
  }
  template<class T> static NUMBER_TYPE(T) min(T val) {
    return reduce(val, UPCXX_MIN);
  }

  template<class T> static INTEGER_TYPE(T) lor(T val) {
    return reduce(val, UPCXX_LOR);
  }
  template<class T> static INTEGER_TYPE(T) lxor(T val) {
    return reduce(val, UPCXX_LXOR);
  }
  template<class T> static INTEGER_TYPE(T) land(T val) {
    return reduce(val, UPCXX_LAND);
  }
  template<class T> static INTEGER_TYPE(T) bor(T val) {
    return reduce(val, UPCXX_BOR);
  }
  template<class T> static INTEGER_TYPE(T) bxor(T val) {
    return reduce(val, UPCXX_BXOR);
  }
  template<class T> static INTEGER_TYPE(T) band(T val) {
    return reduce(val, UPCXX_BAND);
  }

  template<class T> static NUMBER_TYPE(T) add(T val, int dst) {
    return reduce(val, UPCXX_SUM, dst);
  }
  template<class T> static NUMBER_TYPE(T) mult(T val, int dst) {
    return reduce(val, UPCXX_PROD, dst);
  }
  template<class T> static NUMBER_TYPE(T) max(T val, int dst) {
    return reduce(val, UPCXX_MAX, dst);
  }
  template<class T> static NUMBER_TYPE(T) min(T val, int dst) {
    return reduce(val, UPCXX_MIN, dst);
  }

  template<class T> static INTEGER_TYPE(T) lor(T val, int dst) {
    return reduce(val, UPCXX_LOR, dst);
  }
  template<class T> static INTEGER_TYPE(T) lxor(T val, int dst) {
    return reduce(val, UPCXX_LXOR, dst);
  }
  template<class T> static INTEGER_TYPE(T) land(T val, int dst) {
    return reduce(val, UPCXX_LAND, dst);
  }
  template<class T> static INTEGER_TYPE(T) bor(T val, int dst) {
    return reduce(val, UPCXX_BOR, dst);
  }
  template<class T> static INTEGER_TYPE(T) bxor(T val, int dst) {
    return reduce(val, UPCXX_BXOR, dst);
  }
  template<class T> static INTEGER_TYPE(T) band(T val, int dst) {
    return reduce(val, UPCXX_BAND, dst);
  }
};
