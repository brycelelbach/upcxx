/**
 * future.h - Implement future 
 * require C++11
 */

#include <memory> // for shared_pointer
#include <cassert>

namespace upcxx
{
  struct future_am_t {
    void *rv_ptr;
    size_t rv_sz;
    bool *rv_ready_ptr;
  };
    
  /**
   * \addtogroup asyncgroup Asynchronous task execution and Progress
   * @{
   * Futures are used to get the return value of an async function
   */
  template<class T>  
  struct future {
    volatile bool _ready;
    //std::shared_ptr<T> _rv_ptr;
    T* _rv_ptr;
    
  public:
    inline future()
    {
      _ready = false;
      //_rv_ptr = std::make_shared<T>();
      _rv_ptr = new T();
    }
  
    void wait()
    {
      while (!ready) {
        advance();
      }
    }

    inline T get() { return *_rv_ptr; };

    inline future_am_t* get_future_am()
    {
      future_am_t *fam = malloc(sizeof(future_am_t));
      assert(fam != NULL);
      fam->rv_ptr = _rv_ptr;
      fam->rv_sz = sizeof(T);
      fam->rv_ready_ptr = &ready;
      return fam;
    }
  };  // end of struct future
  /// @}

} // end of namespace upcxx
