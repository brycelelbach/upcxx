#pragma once

#include <collective.h>

template<class T> struct datatype_wrapper {};
template<> struct datatype_wrapper<char> {
  static const upcxx_datatype_t value = UPCXX_CHAR;
};
template<> struct datatype_wrapper<unsigned char> {
  static const upcxx_datatype_t value = UPCXX_UCHAR;
};
template<> struct datatype_wrapper<short> {
  static const upcxx_datatype_t value = UPCXX_SHORT;
};
template<> struct datatype_wrapper<unsigned short> {
  static const upcxx_datatype_t value = UPCXX_USHORT;
};
template<> struct datatype_wrapper<int> {
  static const upcxx_datatype_t value = UPCXX_INT;
};
template<> struct datatype_wrapper<unsigned int> {
  static const upcxx_datatype_t value = UPCXX_UINT;
};
template<> struct datatype_wrapper<long> {
  static const upcxx_datatype_t value = UPCXX_LONG;
};
template<> struct datatype_wrapper<unsigned long> {
  static const upcxx_datatype_t value = UPCXX_ULONG;
};
template<> struct datatype_wrapper<long long> {
  static const upcxx_datatype_t value = UPCXX_LONG_LONG;
};
template<> struct datatype_wrapper<unsigned long long> {
  static const upcxx_datatype_t value = UPCXX_ULONG_LONG;
};
template<> struct datatype_wrapper<float> {
  static const upcxx_datatype_t value = UPCXX_FLOAT;
};
template<> struct datatype_wrapper<double> {
  static const upcxx_datatype_t value = UPCXX_DOUBLE;
};

class Reduce {
 private:
  template<class T> static T reduce(T val, upcxx_op_t op) {
    T redval = reduce(val, op, 0);
    T bcval;
    upcxx_bcast(&redval, &bcval, sizeof(T), 0);
    return bcval;
  }
  template<class T> static T reduce(T val, upcxx_op_t op, int dst) {
    T redval;
    upcxx_reduce(&val, &redval, 1, dst, op, datatype_wrapper<T>::value);
    return redval;
  }

 public:
  template<class T> static T add(T val) { return reduce(val, UPCXX_SUM); }
  template<class T> static T mult(T val) { return reduce(val, UPCXX_PROD); }
  template<class T> static T max(T val) { return reduce(val, UPCXX_MAX); }
  template<class T> static T min(T val) { return reduce(val, UPCXX_MIN); }
  template<class T> static T lor(T val) { return reduce(val, UPCXX_LOR); }
  template<class T> static T lxor(T val) { return reduce(val, UPCXX_LXOR); }
  template<class T> static T land(T val) { return reduce(val, UPCXX_LAND); }
  template<class T> static T bor(T val) { return reduce(val, UPCXX_BOR); }
  template<class T> static T bxor(T val) { return reduce(val, UPCXX_BXOR); }
  template<class T> static T band(T val) { return reduce(val, UPCXX_BAND); }

  template<class T> static T add(T val, int dst) {
    return reduce(val, UPCXX_SUM, dst);
  }
  template<class T> static T mult(T val, int dst) {
    return reduce(val, UPCXX_PROD, dst);
  }
  template<class T> static T max(T val, int dst) {
    return reduce(val, UPCXX_MAX, dst);
  }
  template<class T> static T min(T val, int dst) {
    return reduce(val, UPCXX_MIN, dst);
  }
  template<class T> static T lor(T val, int dst) {
    return reduce(val, UPCXX_LOR, dst);
  }
  template<class T> static T lxor(T val, int dst) {
    return reduce(val, UPCXX_LXOR, dst);
  }
  template<class T> static T land(T val, int dst) {
    return reduce(val, UPCXX_LAND, dst);
  }
  template<class T> static T bor(T val, int dst) {
    return reduce(val, UPCXX_BOR, dst);
  }
  template<class T> static T bxor(T val, int dst) {
    return reduce(val, UPCXX_BXOR, dst);
  }
  template<class T> static T band(T val, int dst) {
    return reduce(val, UPCXX_BAND, dst);
  }
};
