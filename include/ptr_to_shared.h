/**
 * Multi-node global pointers and memory allocation and copy functions
 */

#pragma once

#include <iostream>

#include "allocate.h"
#include "machine.h"
#include "event.h"

using namespace std;

namespace upcxx
{
  /// \cond SHOW_INTERNAL
  struct alloc_am_t
  {
    size_t nbytes;
    void **ptr_addr;
    event *cb_event;
  };

  struct alloc_reply_t
  {
    void **ptr_addr;
    void *ptr;
    event *cb_event;
  };

  struct free_am_t
  {
    void *ptr;
  };

  struct inc_am_t
  {
    void *ptr;
  };

  template<typename T, typename place_t>
  struct base_ptr
  {
  private:
    place_t _place;
    T *_ptr;

  public:
    typedef place_t place_type;
    typedef T value_type;

    base_ptr(T *ptr, const place_t &pla) :
      _ptr(ptr), _place(pla)
    {
    }

    place_t where() const
    {
      return _place;
    }

    T * raw_ptr() const
    {
      return _ptr;
    }

    size_t operator-(const base_ptr<T, place_t> &x) const
    {
      if (x.where() == this->where())
        return this->raw_ptr() - x.raw_ptr();
      else
        // XXX really ought to signal an error or something.
        // but that would break this code on the device.
        return 0;
    }

    bool operator==(const base_ptr<T, place_t> &x) const
    {
      return x.where() == this->where() && x.raw_ptr() == this->raw_ptr();
    }
  };
  // close of base_ptr

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
  struct ptr_to_shared : public base_ptr<T, node>
  {
  public:
    inline
    ptr_to_shared() :
    base_ptr<T, node>(NULL, my_node) {}

    inline
    ptr_to_shared(T *ptr) :
    base_ptr<T, node>(ptr, my_node) {}

    inline
    ptr_to_shared(T *ptr, node pla) :
      base_ptr<T, node>(ptr, pla) {}

    inline
    ptr_to_shared(const base_ptr<T, node> &p)
      : base_ptr<T, node>(p) {}

    inline
    ptr_to_shared(const ptr_to_shared<T> &p)
    : base_ptr<T, node>(p.raw_ptr(), p.where()) {}

    /*
    void get_value(const T &val)
    {
      if (this->where().islocal()) {
        val = *(T *) this->raw_ptr();
      } else {
        gasnet_get(&val, this->where()->node_id(), this->raw_ptr(), sizeof(T));
      }
    }

    void put_value(const T &val)
    {
      if (this->where().islocal()) {
        *(T *) this->raw_ptr() = val;
      } else {
        gasnet_put(this->where().node_id(), this->raw_ptr(), &val, sizeof(T));
      }
    }
    */

    // type casting operator for local pointers
    operator T*()
    {
      return this->raw_ptr();
    }

    // type casting operator for placed pointers
    template<typename T2>
    operator ptr_to_shared<T2>()
    {
      return ptr_to_shared<T2>((T2 *)this->raw_ptr(), this->where());
    }

    // pointer arithmetic
    const ptr_to_shared<T> operator +(int i) const
    {
      return ptr_to_shared<T>(((T *)this->raw_ptr()) + i, this->where());
    }

    // pointer arithmetic
    const ptr_to_shared<T> operator +(long i) const
    {
      return ptr_to_shared<T>(((T *)this->raw_ptr()) + i, this->where());
    }

    // pointer arithmetic
    const ptr_to_shared<T> operator +(size_t i) const
    {
      return ptr_to_shared<T>(((T *)this->raw_ptr()) + i, this->where());
    }

    // pointer arithmetic
    bool operator !=(void *p) const
    {
      return (this->raw_ptr() != p);
    }

    // pointer arithmetic
    bool operator !=(long p) const
    {
      return (this->raw_ptr() != (void *) p);
    }
  };

  // ptr_to_shared<T, node>
  template<>
  struct ptr_to_shared<void> : public base_ptr<void, node>
  {
  public:
//     inline ptr_to_shared(void *ptr = NULL) :
//     base_ptr<void, node>(ptr, my_node) {}

    inline ptr_to_shared(void *ptr = NULL, node pla = my_node) :
    base_ptr<void, node>(ptr, pla) {}

    inline ptr_to_shared(const base_ptr<void, node> &p)
    : base_ptr<void, node>(p) {}

    inline ptr_to_shared(const ptr_to_shared<void> &p)
    : base_ptr<void, node>(p) {}

    template<typename T2>
    inline ptr_to_shared(const ptr_to_shared<T2> &p)
        : base_ptr<void, node>(p.raw_ptr(), p.where()) {}

    // type casting operator for local pointers
    operator void*() {
      return this->raw_ptr();
    }

    // type casting operator for placed pointers
    template<typename T2>
    operator ptr_to_shared<T2>()
    {
      return ptr_to_shared<T2>((T2 *)this->raw_ptr(), this->where());
    }
  };

  template<typename T>
  inline std::ostream& operator<<(std::ostream& out, ptr_to_shared<T> ptr)
  {
    return out << "{ " << ptr.where() << " addr: " << ptr.raw_ptr() << " }";
  }

  // **************************************************************************
  // Implement copy, allocate and free with ptr for GASNet backend
  // **************************************************************************

  int copy(ptr_to_shared<void> src, ptr_to_shared<void> dst, size_t nbytes);

  /**
   * \ingroup gasgroup
   * \brief copy data from cpu to cpu
   *
   * \tparam T type of the element
   * \param src the pointer of src data
   * \param dst the pointer of dst data
   * \param count the number of element to be copied
   *
   * \see test_global_ptr.cpp
   */
  template<typename T>
  int copy(ptr_to_shared<T> src, ptr_to_shared<T> dst, size_t count)
  {
    size_t nbytes = count * sizeof(T);
    return copy((ptr_to_shared<void>)src, (ptr_to_shared<void>)dst, nbytes);
  }

  int async_copy(ptr_to_shared<void> src,
                 ptr_to_shared<void> dst,
                 size_t bytes,
                 event *e = NULL);

  gasnet_handle_t async_copy2(ptr_to_shared<void> src,
                              ptr_to_shared<void> dst,
                              size_t bytes);
  
  inline void sync_nb(gasnet_handle_t h)
  {
    gasnet_wait_syncnb(h);
  }

  /**
   * \ingroup gasgroup
   * \brief Non-blocking copy data from cpu to cpu
   *
   * \tparam T type of the element
   * \param src the pointer of src data
   * \param dst the pointer of dst data
   * \param count the number of element to be copied
   * \param e the event which should be notified after the completion of the copy
   */
  template<typename T>
  int async_copy(ptr_to_shared<T> src,
                 ptr_to_shared<T> dst,
                 size_t count,
                 event *e = NULL)
  {
    size_t nbytes = count * sizeof(T);
    return async_copy((ptr_to_shared<void>)src,
                      (ptr_to_shared<void>)dst,
                      nbytes,
                      e);
  }

  template<typename T>
  gasnet_handle_t async_copy2(ptr_to_shared<T> src,
                              ptr_to_shared<T> dst,
                              size_t count)
  {
    size_t nbytes = count * sizeof(T);
    return async_copy2((ptr_to_shared<void>)src, (ptr_to_shared<void>)dst, 
                       nbytes);
  }

  inline void async_copy_fence()
  {
    gasnet_wait_syncnbi_all();
  }

  inline int async_copy_try()
  {
    return gasnet_try_syncnbi_all();
  }

  /**
   * \ingroup gasgroup
   * \brief allocate memory in the global address space at a specific node
   *
   * \tparam T type of the element
   *
   * \return the pointer to the allocated pace
   * \param count the number of element to be copied
   * \param pla the node where the memory space should be allocated
   */
  template<typename T>
  ptr_to_shared<T> allocate(node pla, size_t count)
  {
    void *addr;
    size_t nbytes = count * sizeof(T);

    if (pla.islocal() == true) {
#ifdef USE_GASNET_FAST_SEGMENT
      addr = gasnet_seg_alloc(nbytes);
#else
      addr = malloc(nbytes);
#endif
      assert(addr != NULL);
    } else {
      event e;
      e.incref();
      alloc_am_t am = { nbytes, &addr, &e };
      GASNET_SAFE(gasnet_AMRequestMedium0(pla.id(), 
                                          ALLOC_CPU_AM, 
                                          &am, 
                                          sizeof(am)));
      e.wait();
    }

    ptr_to_shared<T> ptr((T *)addr, pla);

#ifdef DEBUG
    fprintf(stderr, "allocated %llu bytes at cpu %d\n",
            addr, pla);
#endif

    return ptr;
  }

  /**
   * \ingroup gasgroup
   * \brief free memory in the global address space
   *
   * \tparam T type of the element
   *
   * \param ptr the pointer to which the memory space should be freed
   */
  template<typename T>
  int deallocate(ptr_to_shared<T> ptr)
  {
    if (ptr.where().islocal() == true) {
#ifdef USE_GASNET_FAST_SEGMENT
      gasnet_seg_free(ptr.raw_ptr());
#else
      free(ptr.raw_ptr());
#endif
    } else {
      free_am_t am;
      am.ptr = ptr.raw_ptr();
      GASNET_SAFE(gasnet_AMRequestMedium0(ptr.where().node_id(), 
                                          FREE_CPU_AM, &am, 
                                          sizeof(am)));
    }
    return UPCXX_SUCCESS;
  }

  inline int remote_inc(ptr_to_shared<long> ptr)
  {
    inc_am_t am;
    am.ptr = ptr.raw_ptr();
    GASNET_SAFE(gasnet_AMRequestMedium0(ptr.where().node_id(), 
                                        INC_AM, &am, sizeof(am)));
    return UPCXX_SUCCESS;
  }
 
}  // namespace upcxx


