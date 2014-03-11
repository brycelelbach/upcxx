/**
 * Multi-node 1-D global arrays 
 *
 * See test_global_array.cpp and gups.cpp for usage examples
 */

#pragma once

#include "machine.h"
#include "gasnet_api.h"
#include "async.h"

// #define UPCXX_DEBUG

namespace upcxx
{
  template<typename T> struct global_ptr;

  // obj is a global_ref or a global_ptr of the object.
  // m is a field/member of the global object.
  #define memberof(obj, m) \
    make_memberof(obj.where(), obj.raw_ptr()->m)

  /// \cond SHOW_INTERNAL
  template<typename T, typename place_t = node>
  struct global_ref
  {
    global_ref(place_t pla, T *ptr)
    {
      _pla = pla;
      _ptr = ptr;
    }

    global_ref<T>& operator = (const T &rhs)
    {
      if (_pla.id() == my_node.id()) {
        *_ptr = rhs;
      } else {
        // if not local
        gasnet_put(_pla.id(), _ptr, (void *)&rhs, sizeof(T));
      }
      return *this;
    }

    global_ref<T>& operator = (const global_ref<T> &rhs)
    {
      T val = rhs.get();
      if (_pla.id() == my_node.id()) {
        *_ptr = val;
      } else {
        // if not local
        gasnet_put(_pla.id(), _ptr, (void *)&val, sizeof(T));
      }
      return *this;
    }

    global_ref<T>& operator ^= (const T &rhs)
    {
      int pla_id = _pla.id();
      if (pla_id == gasnet_mynode()) {
        *_ptr ^= rhs;
      } else {
        // if not local
        T tmp;
        gasnet_get(&tmp, pla_id, _ptr, sizeof(T));
        tmp ^= rhs;
        gasnet_put(pla_id, _ptr, (void *)&tmp, sizeof(T));
      }
      return *this;
    }

    global_ref<T>& operator += (const T &rhs)
    {
      int pla_id = _pla.id();
      if (pla_id == gasnet_mynode()) {
        *_ptr += rhs;
      } else {
        // if not local
        T tmp;
        gasnet_get(&tmp, pla_id, _ptr, sizeof(T));
        tmp += rhs;
        gasnet_put(pla_id, _ptr, (void *)&tmp, sizeof(T));
      }
      return *this;
    }

    T get() const
    {
      if (_pla.id() == my_node.id()) {
        return (*_ptr);
      } else {
        // if not local
        T tmp;
        gasnet_get(&tmp, _pla.id(), _ptr, sizeof(T));
        return tmp;
      }
    }

    operator T() const
    {
      if (_pla.id() == my_node.id()) {
        return (*_ptr);
      } else {
        // if not local
        T tmp;
        gasnet_get(&tmp, _pla.id(), _ptr, sizeof(T));
        return tmp;
      }
    }
    
    global_ptr<T> operator &()
    {
      return global_ptr<T>(_ptr, _pla);
    }

    T& operator [](int i)
    {
      return _ptr[i];
    }

    T* raw_ptr() const
    {
      return _ptr;
    }

    place_t where() const
    {
      return _pla;
    }

  private:
    T *_ptr;
    place_t _pla;
  }; // struct global_ref
  /// \endcond

  template<typename T, typename place_t>
  global_ref<T, place_t> make_memberof(place_t where, T &member) {
    return global_ref<T, place_t>(where, &member);
  }

} // namespace upcxx
