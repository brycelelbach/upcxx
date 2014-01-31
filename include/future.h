/**
 * future.h - Implement future and promise data types
 */

#include "event.h"

namespace upcxx
{


  /**
   * \addtogroup asyncgroup Asynchronous task execution and Progress
   * @{
   * Futures are used to notify asych task completion and invoke callback functions.
   */
  template<class T>  
  struct future {
    T _rv;
    event *e;

  public:
    inline future() : _rv(), e();
    { 
    }

    inline int count() const { return _count; }

#ifdef USE_CXX11
    inline T&& get() { return _rv; };
#else
    inline T& get() { return _rv; };
#endif

    inline void run_cb()
    {
      assert (_count == 0);

      gasnet_hsl_lock(&_lock);
      // add done_cb to the task queue
      if (_num_done_cb > 0) {
        gasnet_hsl_lock(&async_lock);
        for (int i=0; i<_num_done_cb; i++) {
          if (_done_cb[i] != NULL)
            queue_enqueue(async_task_queue, _done_cb[i]);
        }
        gasnet_hsl_unlock(&async_lock);
      }
      gasnet_hsl_unlock(&_lock);
    }

    inline bool isdone() const { return (_count == 0); }
      
    // Increment the reference counter for the future
    inline  int incref(uint32_t c=1) { return (_count += c); }

    // Decrement the reference counter for the event
    inline void decref() 
    {
      if (_count == 0) {
        fprintf(stderr,
                "Fatal error: attempt to decrement a future (%p) with 0 references!\n",
                this);
        exit(1);
      }
        
      gasnet_hsl_lock(&_lock);
      _count--;
      gasnet_hsl_unlock(&_lock);

      if (isdone()) {
        run_cb();
      }
    }

    /**
     * Wait for the asynchronous event to complete
     */
    void wait(); 

    /**
     * Return 1 if the task is done; return 0 if not
     */
    int test();
    
    /**
     * Assigns the contents of another future object
     */
    operator = (const future& other);
    
  };  // end of class Future
  /// @}

  inline
  std::ostream& operator<<(std::ostream& out, const future& f)
  {
    return out << "upcxx::future: count " << f.count()
               << ", # of callback tasks " << f.num_done_cb()
               << "\n";
  }

  inline void wait(future *f)
  {
    f.wait();
  }

  inline void wait()
  {
    wait(default_future);
  }

  inline int test(future_t& f = default_future)
  {
    if (f == NULL) {
      return 1;
    }
    
    return f.test();
  }
} // end of namespace upcxx
