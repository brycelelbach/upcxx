/**
 * future.h - Implement future 
 * require C++11
 */

#include <memory> // for shared_pointer
#include <cassert>

namespace upcxx
{
  struct future_am_t {
    size_t rv_sz;
    void *rv_ptr;
    bool *rv_ready_ptr;
  };
    
  /**
   * \addtogroup asyncgroup Asynchronous task execution and Progress
   * @{
   * Futures are used to get the return value of an async function
   */
  
  template<class T>  
  struct _future {
    T val;
    volatile bool ready;
    
  public:
    inline _future() : val(), ready(false)
    {
    }
  
    void wait()
    {
      while (!ready) {
        advance();
      }
    }

    inline T get() { wait(); return val; };

    inline future_am_t* get_future_am()
    {
      future_am_t *fam = (future_am_t *)malloc(sizeof(future_am_t));
      assert(fam != NULL);
      fam->rv_ptr = &val;
      fam->rv_sz = sizeof(T);
      fam->rv_ready_ptr = &ready;
      return fam;
    }
  };
  
  template<class T>
  struct future {
    std::shared_ptr< _future<T> > _ptr;
    
  public:
    inline future()
    {
      _ptr = std::make_shared< _future<T> >();
    }
  };  // end of struct future
  
  
  /// @}

} // end of namespace upcxx
