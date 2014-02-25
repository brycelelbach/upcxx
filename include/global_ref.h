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

    const global_ptr<T> operator &() const
    {
      return global_ptr<T>(_ptr, _pla);
    }

    // AK: should this be returning (*_ptr)[i] instead?
    T& operator [](int i)
    {
      return _ptr[i];
    }

  private:
    T *_ptr;
    place_t _pla;
  }; // struct global_ref



  template<typename T>
   struct global_ref< global_ptr<T>, node>
   {
     global_ref(node pla, global_ptr<T> *ptr)
     {
       _pla = pla;
       _ptr = ptr;
     }

     global_ref< global_ptr<T> >& operator = (const global_ptr<T> &rhs)
     {
       if (_pla.id() == my_node.id()) {
         *_ptr = rhs;
       } else {
         // if not local
         gasnet_put(_pla.id(), _ptr, (void *)&rhs, sizeof(global_ptr<T>));
       }
       return *this;
     }

     global_ref< global_ptr<T> >& operator = (const global_ref< global_ptr<T> > &rhs)
     {
        global_ptr<T>  val = rhs.get();
       if (_pla.id() == my_node.id()) {
         *_ptr = val;
       } else {
         // if not local
         gasnet_put(_pla.id(), _ptr, (void *)&val, sizeof( global_ptr<T> ));
       }
       return *this;
     }

     global_ref< global_ptr<T> >& operator ^= (const  global_ptr<T>  &rhs)
     {
       int pla_id = _pla.id();
       if (pla_id == gasnet_mynode()) {
         *_ptr ^= rhs;
       } else {
         // if not local
          global_ptr<T>  tmp;
         gasnet_get(&tmp, pla_id, _ptr, sizeof( global_ptr<T> ));
         tmp ^= rhs;
         gasnet_put(pla_id, _ptr, (void *)&tmp, sizeof( global_ptr<T> ));
       }
       return *this;
     }

     global_ref< global_ptr<T> >& operator += (const  global_ptr<T>  &rhs)
     {
       int pla_id = _pla.id();
       if (pla_id == gasnet_mynode()) {
         *_ptr += rhs;
       } else {
         // if not local
          global_ptr<T>  tmp;
         gasnet_get(&tmp, pla_id, _ptr, sizeof( global_ptr<T> ));
         tmp += rhs;
         gasnet_put(pla_id, _ptr, (void *)&tmp, sizeof( global_ptr<T> ));
       }
       return *this;
     }

      global_ptr<T>  get() const
     {
       if (_pla.id() == my_node.id()) {
         return (*_ptr);
       } else {
         // if not local
          global_ptr<T>  tmp;
         gasnet_get(&tmp, _pla.id(), _ptr, sizeof( global_ptr<T> ));
         return tmp;
       }
     }

     operator global_ptr<T> () const
     {
       if (_pla.id() == my_node.id()) {
         return (*_ptr);
       } else {
         // if not local
         global_ptr<T>  tmp;
         gasnet_get(&tmp, _pla.id(), _ptr, sizeof( global_ptr<T> ));
         return tmp;
       }
     }

     global_ptr< global_ptr<T> > operator &()
     {
       return global_ptr<T>(_ptr, _pla);
     }

     // It's would be very dangerous (error-prone) if we allow implicit type conversion!!
     explicit operator T*()
     {
       assert(_pla.id() == my_node.id());
       return (T*)_ptr->raw_ptr();
     }

     // pointer arithmetic
     template <typename T2>
     global_ptr<T> operator +(T2 i) const
     {
       global_ptr<T> tmp = get();
       return global_ptr<T>(((T *)tmp.raw_ptr()) + i, tmp.where());
     }

     global_ref<T> operator [](int i)
     {
       global_ptr<T> tmp = get();
       return tmp[i];
     }

     // \Todo add member() function

   private:
     global_ptr<T> *_ptr;
     node _pla;
   }; // struct global_ref
  /// \endcond

} // namespace upcxx
