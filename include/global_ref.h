/**
 * Multi-node 1-D global arrays 
 *
 * See test_global_array.cpp and gups.cpp for usage examples
 */

#pragma once

#ifdef UPCXX_HAVE_CXX11
#include <type_traits> // for remove reference
#endif

#include "gasnet_api.h"
// #include "async.h"

// #define UPCXX_DEBUG

namespace upcxx
{
  template<typename T> struct global_ptr;

  // obj is a global_ref_base or a global_ptr of the object.
  // m is a field/member of the global object.
  #define upcxx_memberof(obj, m) \
    upcxx::make_memberof((obj).where(), (obj).raw_ptr()->m)

  #ifdef UPCXX_SHORT_MACROS
  # define memberof upcxx_memberof
  #endif

  /// \cond SHOW_INTERNAL
  template<typename T, typename place_t = rank_t>
  struct global_ref_base
  {
    global_ref_base(place_t pla, T *ptr)
      : _pla(pla), _ptr(ptr)
    {}


    global_ref_base(const global_ref_base &r)
      : _pla(r.where()), _ptr(r.raw_ptr())
    {}

    global_ref_base& operator = (const T &rhs)
    {
      if (_pla == global_myrank()) {
        *_ptr = rhs;
      } else {
        // if not local
        gasnet_put(_pla, _ptr, (void *)&rhs, sizeof(T));
      }
      return *this;
    }

    global_ref_base& operator = (const global_ref_base<T> &rhs)
    {
      T val = rhs.get();
      if (_pla == global_myrank()) {
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

#define UPCXX_GLOBAL_REF_ASSIGN_OP(OP) \
    global_ref_base<T>& operator OP (const T &rhs) \
    { \
      if (_pla == global_myrank()) { \
        *_ptr OP rhs; \
      } else { \
       T tmp; \
       gasnet_get(&tmp, _pla, _ptr, sizeof(T)); \
       tmp OP rhs; \
       gasnet_put(_pla, _ptr, (void *)&tmp, sizeof(T)); \
      } \
      return *this; \
    }

    UPCXX_GLOBAL_REF_ASSIGN_OP(+=)

    UPCXX_GLOBAL_REF_ASSIGN_OP(-=)

    UPCXX_GLOBAL_REF_ASSIGN_OP(*=)

    UPCXX_GLOBAL_REF_ASSIGN_OP(/=)

    UPCXX_GLOBAL_REF_ASSIGN_OP(%=)

    UPCXX_GLOBAL_REF_ASSIGN_OP(^=)

    UPCXX_GLOBAL_REF_ASSIGN_OP(|=)

    UPCXX_GLOBAL_REF_ASSIGN_OP(&=)

    UPCXX_GLOBAL_REF_ASSIGN_OP(<<=)

    UPCXX_GLOBAL_REF_ASSIGN_OP(>>=)

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
      if (_pla == global_myrank()) {
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
      if (_pla == global_myrank()) {
        return (*_ptr);
      } else {
        // if not local
        T tmp;
        gasnet_get(&tmp, _pla, _ptr, sizeof(T));
        return tmp;
      }
    }

    global_ptr<T> operator &()
    {
      return global_ptr<T>(_ptr, _pla);
    }

    const global_ptr<T> operator &() const
    {
      return global_ptr<T>(_ptr, _pla);
    }

    T* raw_ptr() const
    {
      return _ptr;
    }

    place_t where() const
    {
      return _pla;
    }

  protected:
    T *_ptr;
    place_t _pla;
  }; // struct global_ref_base
  /// \endcond

  template<typename T, typename place_t = rank_t>
  struct global_ref : public global_ref_base<T, place_t>
  {
    global_ref(place_t pla, T *ptr) : global_ref_base<T, place_t>(pla, ptr)
    { }

    global_ref(const global_ref_base<T, place_t> &r) :
      global_ref_base<T, place_t>(r)
    { }

    inline global_ref operator=(const T& rhs)
    {
      return global_ref_base<T, place_t>::operator=(rhs);
    }
  };

  template<typename T>
  struct global_ref<T*> : public global_ref_base<T*>
  {
    global_ref(rank_t pla, T **ptr) : global_ref_base<T*>(pla, ptr)
    { }

    global_ref(const global_ref_base<T*> &r) :
      global_ref_base<T*>(r)
    { }

    inline global_ref operator=(const T*& rhs)
    {
      return global_ref_base<T*>::operator=(rhs);
    }

    template<typename T2>
    global_ref<T> operator [](T2 i)
    {
      T* tmp = this->get();
      return global_ref<T>(this->where(), tmp+i); // _ptr has type T**, *_ptr has type T*
    }
  };

  // Specialization for constant-size array types
  // Note that some operations are Invalid for array types (e.g., +, =, get()),
  // thus we use global_ref_base<T> as the base type
  template<typename T, size_t N>
  struct global_ref<T[N]> : public global_ref_base<T>
  {
    global_ref(rank_t pla, T (*ptr)[N]) : global_ref_base<T>(pla, (T*)ptr)
    { }

    template<typename T2>
    global_ref<T> operator [](T2 i)
    {
      return global_ref<T>(this->where(), (T*)this->raw_ptr() + i);
    }
  };

  template<typename T>
  struct global_ref<global_ptr<T> > : public global_ref_base<global_ptr<T> >
  {
    global_ref(rank_t pla, global_ptr<T> *ptr) : global_ref_base<global_ptr<T> >(pla, ptr)
    { }

    global_ref(const global_ref_base<global_ptr<T> > &r) :
      global_ref_base<global_ptr<T> >(r)
    { }

    inline global_ref operator=(const global_ptr<T>& rhs)
    {
      return global_ref_base<global_ptr<T> >::operator=(rhs);
    }

#ifdef UPCXX_HAVE_CXX11
    template <typename T2>
    auto operator [](T2 i) -> decltype(this->get()[i])
    {
      global_ptr<T> tmp = this->get();
      return tmp[i];
    }
#endif
  };

  template<typename T, typename place_t>
  global_ref<T, place_t> make_memberof(place_t where, T &member) {
    return global_ref<T, place_t>(where, &member);
  }
} // namespace upcxx
