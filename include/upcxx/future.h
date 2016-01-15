/**
 * future.h - Implement future 
 * require C++11
 */

#include <memory> // for shared_pointer
#include <cassert>

namespace upcxx
{
  /**
   * \ingroup internalgroup
   * Internal objects for storing the values of futures
   */
  struct future_storage_t {
    size_t sz;
    void *data;
    volatile bool ready; 

    future_storage_t() : sz(0), data(nullptr), ready(false)
    { }

    template<class T>
    explicit future_storage_t(const T& val)
    {
      sz = sizeof(T);
      data = malloc(sz);
      assert(data != NULL);
      memcpy(data, &val, sizeof(T));
    }   
    
    ~future_storage_t()
    {
      if (data != NULL) free(data);
    }

    inline void store(void *val, size_t val_sz)
    {
      assert(val != NULL);
      if (data != NULL) free(data);
      sz = val_sz;
      data = malloc(sz);
      assert(data != NULL);
      memcpy(data, val, sz);      
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

    inline T get()
    {
      wait();
      assert(_ptr->data != NULL);
      return *(T *)_ptr->data;
    }

    inline future_storage_t *ptr() { return _ptr.get(); }
    
  };  // end of struct future

  inline
  std::ostream& operator<<(std::ostream& out, const future_storage_t& fs)
  {
    return out << "future_storage_t: sz " << fs.sz
               << " data " << fs.data
               << " ready " << fs.ready
               << "\n";
  }
  
  template<typename T>
  inline
  std::ostream& operator<<(std::ostream& out, const future<T>& f)
  {
    out << "future: use_count: " << f._ptr.use_count() 
        << " storage ptr: " << f._ptr.get();
    if (f._ptr.get() != NULL) {
      out << " ready: " << f._ptr.get()->ready 
          << " sz: " << f._ptr.get()->sz
          << " val: "<< *(T*)(f._ptr.get()->data);
    }
    return out << "\n";;    
  }

  template<>
  inline
  std::ostream& operator<<(std::ostream& out, const future<void>& f)
  {
    out << "future: use_count: " << f._ptr.use_count() 
        << " storage ptr: " << f._ptr.get();
    if (f._ptr.get() != NULL) {
      out << " ready: " << f._ptr.get()->ready
          << " sz: " << f._ptr.get()->sz;
    }
    return out << "\n";;
  }

  /// @}

} // end of namespace upcxx
