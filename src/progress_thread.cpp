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
#define PROGRESS_HELPER_PAUSE_USEC 1000 // YZ: change to 100?

namespace upcxx
{
  /**
   * Argument struct for the \c progress_helper() thread
   */
  struct progress_helper_args
  {
    // lock for operations on the task queue
    pthread_mutex_t * tq_mutex;

    // copies of _max_dispatch_in/out for calls to upcxx::advance()
    int max_dispatch_in, max_dispatch_out;

    // boolean to signal the progress thread to exit
    bool *progress_thread_stop;
  };

  bool _progress_thread_stop, _progress_thread_running;
  pthread_t _progress_thread;
  pthread_mutex_t _tq_mutex;

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
      // pthread_mutex_lock( args->tq_mutex );

      // check the stop flag, possibly exiting
      if ( *args->progress_thread_stop ) {
        // pthread_mutex_unlock( args->tq_mutex );
        delete args;
        return NULL;
      }

      // drain the task queue
      int tasks_completed;
      tasks_completed = upcxx::advance( args->max_dispatch_in, args->max_dispatch_out );

      // pthread_mutex_unlock( args->tq_mutex );

      if (tasks_completed ==0) {
        // pause briefly
        usleep( PROGRESS_HELPER_PAUSE_USEC );
        // ?? gasnett_sched_yield(); // yield the cpu if there is no work
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

    // set up progress helper argument struct
    args = new progress_helper_args;
    args->tq_mutex = &_tq_mutex;
    args->max_dispatch_in = 10;
    args->max_dispatch_out = 10;
    args->progress_thread_stop = &_progress_thread_stop;

    // set thread as joinable
    pthread_attr_init( &th_attr );
    pthread_attr_setdetachstate( &th_attr, PTHREAD_CREATE_JOINABLE );

    // turn off stop flag
    _progress_thread_stop = false;

    // start the thread
    assert( pthread_create( &_progress_thread, &th_attr, progress_helper,
                            (void *)args ) == 0 );
    _progress_thread_running = true;
  }

  // signal to stop the progress thread and wait on it
  void
  progress_thread_stop()
  {
    // set the stop flag
    // pthread_mutex_lock( &_tq_mutex );
    _progress_thread_stop = true;
    // pthread_mutex_unlock( &_tq_mutex );

    // wait for thread to stop
    assert( pthread_join( _progress_thread, NULL ) == 0 );
    _progress_thread_running = false;
  }

}
