/**
 * Multi-node 1-D global arrays 
 *
 * See test_global_array.cpp and gups.cpp for usage examples
 */

#pragma once

#include "gasnet_api.h"
#include "async.h"

// #define UPCXX_DEBUG

namespace upcxx
{
  template<typename T> struct global_ptr;

  // obj is a global_ref or a global_ptr of the object.
  // m is a field/member of the global object.
  #define memberof(obj, m)                                      \
    upcxx::make_memberof((obj).where(), (obj).raw_ptr()->m)

  /// \cond SHOW_INTERNAL
  template<typename T, typename place_t = rank_t>
  struct global_ref
  {
    global_ref(place_t pla, T *ptr)
    {
      _pla = pla;
      _ptr = ptr;
    }

    global_ref<T>& operator = (const T &rhs)
    {
      if (_pla == myrank()) {
        *_ptr = rhs;
      } else {
        // if not local
        gasnet_put(_pla, _ptr, (void *)&rhs, sizeof(T));
      }
      return *this;
    }

    global_ref<T>& operator = (const global_ref<T> &rhs)
    {
      T val = rhs.get();
      if (_pla == myrank()) {
        *_ptr = val;
      } else {
        // if not local
        gasnet_put(_pla, _ptr, (void *)&val, sizeof(T));
      }
      return *this;
    }

    template<typename T2>
    T operator + (const T2 &rhs)
    {
      // YZ: should use move semantics here
      return (get() + rhs);
    }

    global_ref<T>& operator ^= (const T &rhs)
    {
      if (_pla == myrank()) {
        *_ptr ^= rhs;
      } else {
        // if not local
        T tmp;
        gasnet_get(&tmp, _pla, _ptr, sizeof(T));
        tmp ^= rhs;
        gasnet_put(_pla, _ptr, (void *)&tmp, sizeof(T));
      }
      return *this;
    }

    global_ref<T>& operator += (const T &rhs)
    {
      if (_pla == myrank()) {
        *_ptr += rhs;
      } else {
        // if not local
        T tmp;
        gasnet_get(&tmp, _pla, _ptr, sizeof(T));
        tmp += rhs;
        gasnet_put(_pla, _ptr, (void *)&tmp, sizeof(T));
      }
      return *this;
    }

    template <typename T2>
    bool operator == (const T2 &rhs)
    {
      return (get() == (T)rhs);
    }

    template <typename T2>
    bool operator != (const T2 &rhs)
    {
      return (get() != (T)rhs);
    }

    T get() const
    {
      if (_pla == myrank()) {
        return (*_ptr);
      } else {
        // if not local
        T tmp;
        gasnet_get(&tmp, _pla, _ptr, sizeof(T));
        return tmp;
      }
    }

    operator T() const
    {
      if (_pla == myrank()) {
        return (*_ptr);
      } else {
        // if not local
        T tmp;
        gasnet_get(&tmp, _pla, _ptr, sizeof(T));
        return tmp;
      }
    }

#if 0 // UPCXX_HAVE_CXX11 YZ: this is not yet supported by icpc 13.1
    template<typename T2>
    explicit operator T2*() const
    {
      return (T2*)get();
    }
#endif

    global_ptr<T> operator &()
    {
      return global_ptr<T>(_ptr, _pla);
    }

    // YZ: Needs C++11 auto, decltype
#ifdef UPCXX_HAVE_CXX11
    template <typename T2>
    auto operator [](T2 i) -> decltype(this->get()[i])
    {
      T tmp = get();
      return tmp[i];
    }
#endif

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
