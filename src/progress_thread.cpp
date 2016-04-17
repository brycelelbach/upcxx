/**
 * UPC++ progress thread for driving asynchronous tasks in the background
 *
 * Based on the progress thread implementation by Scott French in Convergent Matrix
 * https://github.com/swfrench/convergent-matrix
 */

#include "upcxx/upcxx.h"

#include <assert.h>
#include <pthread.h>
#include <unistd.h>  // usleep
#define PROGRESS_HELPER_PAUSE_USEC 10 // \Todo: make this as an environment variable

namespace upcxx
{
  /**
   * Argument struct for the \c progress_helper() thread
   */
  struct progress_helper_args
  {
    // copies of _max_dispatch_in/out for calls to upcxx::advance()
    int max_dispatch_in, max_dispatch_out;
  };
  
  bool _progress_thread_running = false;
  pthread_t _progress_thread;
  
  enum ThreadState { START, PAUSE, STOP };
  ThreadState state = START;
  bool _start_called = false;
  pthread_mutex_t my_mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_cond_t signal_condition = PTHREAD_COND_INITIALIZER;
  
  /**
   * The action performed by the \c progress_helper() thread
   * \param args_ptr A \c void type pointer to a progress_helper_args structure
   */
  void *
  progress_helper( void *args_ptr )
  {
    // re-cast args ptr
    progress_helper_args *args = (progress_helper_args *)args_ptr;
  
    // spin in upcxx::advance()
    while ( 1 ) {
      /* Wait for state START or STOP */
      pthread_mutex_lock(&my_mutex);
      while (state != START && state != STOP) {
        pthread_cond_wait(&signal_condition, &my_mutex);
      }
      pthread_mutex_unlock(&my_mutex);
      
      if ( state == STOP ) {
        delete args;
        return NULL;
      } 
      else {
        // drain the task queue
        int tasks_completed;
        tasks_completed = upcxx::advance( args->max_dispatch_in, args->max_dispatch_out );
        
        if (tasks_completed == 0) {
          // pause briefly
          usleep( PROGRESS_HELPER_PAUSE_USEC );
        }
      }
    }
  }
  
  // begin progress thread execution (executes progress_helper())
  void
  progress_thread_start()
  {
    pthread_attr_t th_attr;
    progress_helper_args * args;
  
    // erroneous to call while thread is running
    assert( ! _progress_thread_running );
  
    if ( ! _start_called ) {
  
      // set up progress helper argument struct
      args = new progress_helper_args;
      args->max_dispatch_in = 10;
      args->max_dispatch_out = 10;
      state = START;

      // set thread as joinable
      pthread_attr_init( &th_attr );
      pthread_attr_setdetachstate( &th_attr, PTHREAD_CREATE_JOINABLE );
  
      // start the thread
      assert( pthread_create( &_progress_thread, &th_attr, progress_helper,
                              (void *)args ) == 0 );
      _progress_thread_running = true;
  
      _start_called = true;
    }
    else {
      /* Set state to START and wake up helper thread */
      pthread_mutex_lock(&my_mutex);
      state = START;
      pthread_cond_signal(&signal_condition);
      pthread_mutex_unlock(&my_mutex);
    }
  }
  
  // signal to stop the progress thread and wait on it
  void
  progress_thread_pause()
  {
    assert(_start_called);
    assert(state == START);
    /* Set state to PAUSE and wake up helper thread */
    pthread_mutex_lock(&my_mutex);
    state = PAUSE;
    pthread_cond_signal(&signal_condition);
    pthread_mutex_unlock(&my_mutex);
    
    _progress_thread_running = false;
  }
  
  // signal to stop the progress thread and call join
  void
  progress_thread_stop()
  {
    assert(_start_called);
    assert(state == START);
    /* Set state to STOP and wake up helper thread */
    pthread_mutex_lock(&my_mutex);
    state = STOP;
    pthread_cond_signal(&signal_condition);
    pthread_mutex_unlock(&my_mutex);
    // wait for thread to stop
    assert( pthread_join( _progress_thread, NULL ) == 0 );
    _start_called = false;
    _progress_thread_running = false;
  }

}
