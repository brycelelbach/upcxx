/**
 * Shared variable implementation
 */

#pragma once

namespace upcxx
{
  // **************************************************************************
  // Implement global address space shared variable template  
  // **************************************************************************
  
  /*
   * shared_var_offset = shared_var_addr_P0 - my_shared_var_addr;
   *
   * where shared_var_addr_P0 is the local address of a global
   * datum on node 0 and my_shared_var_addr is the local address of
   * the datum on my node.
   */
  extern size_t shared_var_offset; 

  /**
   * \ingroup gasgroup
   * \brief globally shared variable
   *
   * shared_var variables can only be defined in the file scope
   * (not inside a function).  Global data reside on CPU place 0
   * (node 0). It only supports basic data types/classes that can
   * use direct data memcpy (shallow copy) for assignment operations
   * without a customized copy function (deep copy) because the
   * system doesn't have a general mechanism to perform deep copy
   * across nodes.
   *
   * Example usage: 
   *   shared_var<int> x=123;
   *
   *   void foo() { // can be executed on any cpu place
   *     int y = x.get_value();
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
    T _val;

  public:
    shared_var() { }
      
    shared_var(const T &val) : _val(val) { }

    // copy constructor
    inline
    shared_var(const shared_var<T> &g) : _val(g._val) { }

    T& get_value()
    {
      if (my_node.id() != 0) {
        gasnet_get(&_val, 0, (char *)&_val+shared_var_offset, sizeof(T)); 
      }

      return _val;
    }
    
    void put_value(const T &val)
    {
      _val = val;
      if (my_node.id() != 0) {
        gasnet_put(0, (char *)&_val+shared_var_offset, &_val, sizeof(T));
      }
    }
      
    shared_var<T>& operator =(const T &val)
    {
      put_value(val);
      return *this;
    }

    shared_var<T>& operator +=(const T &a)
    {
      if (my_node.id() != 0) {
        gasnet_get(&_val, 0, (char *)&_val+shared_var_offset, sizeof(T));
      }
      _val += a;
      put_value(_val);
      return *this;
    }

    // copy assignment 
    shared_var<T>& operator =(shared_var<T> &g)
    {
      if (this != &g) {
        put_value(g.get_value());
      }
      return *this;
    }
        
    // type conversion operator
    operator T() 
    { 
      return get_value();
    }

    inline ptr_to_shared<T> operator &()
    {
      return ptr_to_shared<T>(&_val, node(0));
    }

  }; // struct shared_var

} // namespace upcxx

