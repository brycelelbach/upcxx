/**
 * Shared variable implementation
 */

#pragma once

#include "gasnet_api.h"

namespace upcxx
{
  // **************************************************************************
  // Implement global address space shared variable template  
  // **************************************************************************
  
  /*
   * shared_var_addr = shared_var_addr_P0 - my_shared_var_addr;
   *
   * where shared_var_addr_P0 is the local address of a global
   * datum on rank 0 and my_shared_var_addr is the local address of
   * the datum on my rank.
   */
  extern void *shared_var_addr;
  extern size_t total_shared_var_sz;

  /**
   * \ingroup gasgroup
   * \brief globally shared variable
   *
   * shared_var variables can only be defined in the file scope
   * (not inside a function).  Global data reside on CPU place 0
   * (node 0). It only supports contiguous data types that can
   * use direct data memcpy (shallow copy) for assignment operations
   * without a customized copy function (deep copy) because the
   * system doesn't have a general mechanism to perform deep copy
   * across nodes.
   *
   * Example usage: 
   *   shared_var<int> x=123;
   *
   *   void foo() { // can be executed on any cpu place
   *     int y = x.get();
   *     x = 456;
   *   }
   *
   *   std::cout << x;
   *   printf("x=%d\n", (int)x); // explicit casting is needed for printf
   */
  template<typename T>
  struct shared_var
  {
  private:
    size_t _my_offset;
    T _val;

  public:
    shared_var()
    {
      _my_offset = total_shared_var_sz;
      total_shared_var_sz += sizeof(T);
    }
      
    inline shared_var(const T &val) : _val(val)
    {
      _my_offset = total_shared_var_sz;
      total_shared_var_sz += sizeof(T);
    }

    // copy constructor
    inline
    shared_var(const shared_var<T> &g) : _val(g._val)
    {
      _my_offset = total_shared_var_sz;
      total_shared_var_sz += sizeof(T);
    }

    T& get()
    {
      gasnet_get(&_val, 0, (char *)shared_var_addr+_my_offset, sizeof(T));
      return _val;
    }
    
    void put(const T &val)
    {
      _val = val;
      gasnet_put(0, (char *)shared_var_addr+_my_offset, &_val, sizeof(T));
    }
      
    shared_var<T>& operator =(const T &val)
    {
      put(val);
      return *this;
    }

#define UPCXX_SHARED_VAR_ASSIGN_OP(OP) \
    shared_var<T>& operator OP(const T &a) \
    { \
      gasnet_get(&_val, 0, (char *)shared_var_addr+_my_offset, sizeof(T)); \
      _val OP a; \
      put(_val); \
      return *this; \
    }

    UPCXX_SHARED_VAR_ASSIGN_OP(+=)

    UPCXX_SHARED_VAR_ASSIGN_OP(-=)

    UPCXX_SHARED_VAR_ASSIGN_OP(*=)

    UPCXX_SHARED_VAR_ASSIGN_OP(/=)

    UPCXX_SHARED_VAR_ASSIGN_OP(%=)

    UPCXX_SHARED_VAR_ASSIGN_OP(^=)

    UPCXX_SHARED_VAR_ASSIGN_OP(|=)

    UPCXX_SHARED_VAR_ASSIGN_OP(&=)

    UPCXX_SHARED_VAR_ASSIGN_OP(<<=)

    UPCXX_SHARED_VAR_ASSIGN_OP(>>=)

    //////

    // copy assignment 
    shared_var<T>& operator =(shared_var<T> &g)
    {
      if (this != &g) {
        put(g.get());
      }
      return *this;
    }
        
    // type conversion operator
    operator T() 
    { 
      return get();
    }

    inline global_ptr<T> operator &()
    {
      return global_ptr<T>(&_val, 0);
    }

  }; // struct shared_var

} // namespace upcxx

