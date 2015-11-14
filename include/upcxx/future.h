/**
 * future.h - Implement future 
 * require C++11
 */

#include <memory> // for shared_pointer
#include <cassert>

namespace upcxx
{
  struct future_storage_t {
    size_t sz;
    void *data;
    volatile bool ready; 
    future_storage_t() : sz(0), data(nullptr), ready(false)
    { }
      
    future_storage_t(size_t storage_sz)
    {
      sz = storage_sz;
      if (sz > 0) {
        data = malloc(sz);
        assert(data != nullptr);
      } else {
        data = nullptr;
      }
      ready = false;
    }

    template<class T>
    future_storage_t(typename std::enable_if<std::is_trivially_copyable<T>::value, T>::type  t)
    {
      sz = sizeof(T);
      data = malloc(sz);
      assert(data != NULL);
      memcpy(data, &t, sizeof(T));
    }   
    
    ~future_storage_t()
    {
      if (data != NULL) free(data);
    }

    // template<class T>
    // void store(typename std::enable_if<std::is_trivially_copyable<T>::value, T>::type  t)
    // {
    //   if (data != NULL) free(data);
    //   sz = sizeof(T);
    //   data = malloc(sz);
    //   assert(data != NULL);
    //   memcpy(data, &t, sizeof(T));      
    // }

    template<class T>
    void store(T t)
    {
      if (data != NULL) free(data);
      sz = sizeof(T);
      data = malloc(sz);
      assert(data != NULL);
      memcpy(data, &t, sizeof(T));      
    }

  };
    
  /**
   * \addtogroup asyncgroup Asynchronous task execution and Progress
   * @{
   * Futures are used to get the return value of an async function
   */
  template<class T>
  struct future {
    std::shared_ptr<future_storage_t> _ptr;
    
  public:
    inline future()
    {
      _ptr = std::make_shared<future_storage_t>();
    }

    inline void wait()
    {
      while (!_ptr->ready) {
        upcxx::advance();
      }
    }

    inline T get() { wait(); return *(T *)_ptr->data; };

    inline future_storage_t *ptr() { return _ptr.get(); }
    
  };  // end of struct future
  
  
  /// @}

} // end of namespace upcxx
