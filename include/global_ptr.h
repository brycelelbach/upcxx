/**
 * Multi-node global pointers and memory allocation and copy functions
 */

#pragma once

#include <iostream>

// #include "allocate.h"
#include "upcxx_runtime.h"
#include "event.h"
#include "global_ref.h"

using namespace std;

namespace upcxx
{
  bool is_memory_shared_with(rank_t r);

  void *pshm_remote_addr2local(rank_t r, void *addr);

  /// \cond SHOW_INTERNAL
  // base_ptr is the base class of global pointer types.
  template<typename T, typename place_t>
  class base_ptr
  {
  public:
    typedef place_t place_type;
    typedef T value_type;

    base_ptr(T *ptr, const place_t &pla) : _ptr(ptr), _pla(pla)
    {}

    base_ptr(const place_t &pla, T *ptr) : _ptr(ptr), _pla(pla)
    {}

    place_t where() const
    {
      return _pla;
    }

    T* raw_ptr() const
    {
      return _ptr;
    }

    size_t operator - (const base_ptr<T, place_t> &x) const
    {
      assert (x.where() == this->where());
      return this->raw_ptr() - x.raw_ptr();
    }

    template <typename T2>
    bool operator == (const base_ptr<T2, place_t> &rhs) const
    {
      return (where() == rhs.where() && raw_ptr() == rhs.raw_ptr());
    }

    template <typename T2>
    bool operator != (const base_ptr<T2, place_t> &rhs) const
    {
      return (where() != rhs.where() || raw_ptr() != rhs.raw_ptr());
    }

#ifdef UPCXX_HAVE_CXX11
    bool operator != (decltype(nullptr)) const
    {
      return (raw_ptr() != nullptr);
    }

    bool operator == (decltype(nullptr)) const
    {
      return (raw_ptr() == nullptr);
    }
#endif

    bool isnull() const
    {
      return (raw_ptr() == NULL);
    }

  protected:
    T *_ptr;
    place_t _pla;
  }; // close of base_ptr
  /// \endcond SHOW_INTERNAL

  /**
   * \defgroup gasgroup Global Address Space primitives
   * This group of API defines basic elements of a global address
   * space programming model.
   */

  /**
   * \ingroup gasgroup
   * \brief Global address space pointer type
   *
   * \tparam T element type of the data being pointed to
   * \tparam place_t the type of the location where the data resides
   *
   * \see test_global_ptr.cpp
   */
  template<typename T>
  class global_ptr : public base_ptr<T, rank_t>
  {
    typedef T value_type;

  public:
    inline explicit global_ptr() : base_ptr<T, rank_t>((T *)NULL, 0) {}

    inline explicit global_ptr(T *ptr)
      : base_ptr<T, rank_t>(ptr, global_myrank()) {}

    inline
    global_ptr(T *ptr, rank_t pla) :
      base_ptr<T, rank_t>(ptr, pla) {}

    inline
    global_ptr(rank_t pla, T *ptr) :
      base_ptr<T, rank_t>(ptr, pla) {}

    inline
    global_ptr(const base_ptr<T, rank_t> &p)
      : base_ptr<T, rank_t>(p) {}

    inline
    global_ptr(const global_ptr<T> &p)
    : base_ptr<T, rank_t>(p.raw_ptr(), p.where()) {}

    // type casting operator for local pointers
#ifdef UPCXX_HAVE_CXX11
    explicit
#endif
    operator T*() const
    {
      if (this->where() == global_myrank()) {
        // return raw_ptr if the data pointed to is on the same rank
        return this->raw_ptr();
      }

#if GASNET_PSHM
      return (T*)pshm_remote_addr2local(this->where(), this->raw_ptr());
#else
      // return NULL if this global address can't casted to a valid
      // local address
      return NULL;
#endif
    }

    template <typename T2>
    global_ref<T> operator [] (T2 i) const
    {
      return global_ref<T>(this->where(), (T *)this->raw_ptr() + i);
    }

    // Support -> operator when pointing to a local object
    T* operator->() const
    {
      if (this->where() == myrank()) {
        return this->raw_ptr();
      } else {
        cerr << "global_ptr " << *this << " is pointing to a remote object "
              << "but the '->' operator is supported only when pointing to "
              << "a local object.  Please use 'memberof(global_ptr, filed)'\n";
        gasnet_exit(1);
      }
      return NULL; // should never get here
    }

    global_ref<T> operator *() const
    {
      return global_ref<T>(this->where(), (T *)this->raw_ptr());
    }

    // type casting operator for placed pointers
    template<typename T2>
    operator global_ptr<T2>() const
    {
      return global_ptr<T2>((T2 *)this->raw_ptr(), this->where());
    }

    // pointer arithmetic
    template <typename T2>
    global_ptr<T> operator +(T2 i) const
    {
      return global_ptr<T>(((T *)this->raw_ptr()) + i, this->where());
    }
  };

  // Special case for global_ptr<void> - a void global pointer
  template<>
  struct global_ptr<void> : public base_ptr<void, rank_t>
  {
  public:
    inline explicit global_ptr() : base_ptr<void, rank_t>((void *)NULL, 0) {}

    inline explicit global_ptr(void *ptr, rank_t pla = global_myrank()) :
      base_ptr<void, rank_t>(ptr, pla) {}

    inline global_ptr(const base_ptr<void, rank_t> &p)
    : base_ptr<void, rank_t>(p) {}

    inline global_ptr(const global_ptr<void> &p)
    : base_ptr<void, rank_t>(p) {}

    // Implicit type conversion to global_ptr<void> is very
    // dangerous!
    template<typename T2>
    inline explicit global_ptr(const global_ptr<T2> &p)
      : base_ptr<void, rank_t>(p.raw_ptr(), p.where()) {}

    // type casting operator for local pointers
#ifdef UPCXX_HAVE_CXX11
    explicit
#endif
    operator void*()
    {
      if (this->where() == global_myrank()) {
        // return raw_ptr if the data pointed to is on the same rank
        return this->raw_ptr();
      }
      
#if GASNET_PSHM
      return pshm_remote_addr2local(this->where(), this->raw_ptr());
#else
      // return NULL if this global address can't casted to a valid
      // local address
      return NULL;
#endif
    }

    // type casting operator for placed pointers
    template<typename T2>
    operator global_ptr<T2>()
    {
      return global_ptr<T2>((T2 *)this->raw_ptr(), this->where());
    }

    template<typename T2>
    global_ptr<void>& operator = (const global_ptr<T2> &p)
    {
      _ptr = p.raw_ptr();
      _pla = p.where();
      return *this;
    }
  };

  template<typename T>
  std::ostream& operator<<(std::ostream& out, global_ptr<T> ptr)
  {
    return out << "{ " << ptr.where() << " addr: " << ptr.raw_ptr() << " }";
  }

  int remote_inc(global_ptr<long> ptr);

}  // namespace upcxx


