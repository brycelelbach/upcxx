/**
 * broadcast.h - implements a safe broadcast scheme that converts local
 * pointers and Titanium-style arrays to global pointers and arrays
 * when broadcasting. Any type can be added to this scheme by defining
 * a global_type typedef within the type and an explicit conversion to
 * that global_type. Also implements bulk broadcasts over any array
 * that defines a local_elem_type typedef and storage_ptr() and size()
 * methods.
 * \see collective.h for low-level broadcast interface
 */

#pragma once

#include <global_ptr.h>
#include <collective.h>
#include <interfaces_internal.h>

namespace upcxx {
  template<class T>
  UPCXXI_GLOBALIZE_TYPE(T) broadcast(T val, int root) {
    UPCXXI_GLOBALIZE_TYPE(T) sval = (UPCXXI_GLOBALIZE_TYPE(T)) val;
    UPCXXI_GLOBALIZE_TYPE(T) result;
    upcxx_bcast(&sval, &result, sizeof(UPCXXI_GLOBALIZE_TYPE(T)),
                root);
    return result;
  }

  template<class T> void broadcast(UPCXXI_NONGLOBALIZE_TYPE(T) * src,
                                   T *dst, size_t count, int root) {
    upcxx_bcast(src, dst, count*sizeof(T), root);
  }

  template<class Array>
  void broadcast(Array src, Array dst, int root,
                 UPCXXI_ARRAY_NG_TYPE(Array) * = 0) {
    upcxx_bcast(src.storage_ptr(), dst.storage_ptr(),
                src.size()*sizeof(UPCXXI_ARRAY_ELEM_TYPE(Array)),
                root);
  }
} // namespace upcxx
