/*
 * aysnc_impl.h - implementation details of asynchronous tasks
 */

#pragma once

#include <iostream>

#include "gasnet_api.h"
#include "event.h"
#include "async_templates.h"
#include "active_coll.h"
#include "group.h"
#include "range.h"
#include "team.h"
#include "global_ptr.h"

using namespace std;

// #define DEBUG

namespace upcxx
{
#define MAX_ASYNC_ARG_COUNT 16 // max number of arguments
#define MAX_ASYNC_ARG_SIZE 512 // max size of all arguments (in nbytes)
  
  /// \cond SHOW_INTERNAL
  struct async_task  {
    rank_t _caller; // the place where async is called, for reply
    rank_t _callee; // the place where the task should be executed
    event *_ack; // Acknowledgment event pointer on caller node
    generic_fp _fp;
    void *dst; // active message dst buffer
    void *src; // active message src buffer
    size_t _arg_sz;
    char _args[MAX_ASYNC_ARG_SIZE];
    
    inline void init_async_task(rank_t caller,
                                rank_t callee,
                                event *ack,
                                generic_fp fp,
                                size_t arg_sz,
                                void *async_args)
    {
      assert(arg_sz <= MAX_ASYNC_ARG_SIZE);
      // set up the task message
      // Increase the event reference at submission
      /*
       if (ack != NULL) {
       ack->incref();
       }
       */
      this->_caller = caller;
      this->_callee = callee;
      this->_ack = ack;
      this->_fp = fp;
      this->_arg_sz = arg_sz;
      if (arg_sz > 0) {
        memcpy(&this->_args, async_args, arg_sz);
      }
    }
    
    inline async_task() : _arg_sz(0) { };
            
    inline size_t nbytes(void)
    {
      return (sizeof(async_task) - MAX_ASYNC_ARG_SIZE + _arg_sz);
    }

    #include "async_impl_templates1.h"
    
  }; // close of async_task
  
  inline std::ostream& operator<<(std::ostream& out, const async_task& task)
  {
    return out << "async task: caller " << task._caller
    << " callee " << task._callee
    << " ack_event " << task._ack
    << " fp " << (void *)task._fp
    << " arg_sz " << task._arg_sz
    << "\n";
    // << " actual size in bytes " << task.nbytes()
  }
  
  struct async_done_am_t {
    event *ack_event;
  };
  
  // Add a task the async queue
  // \Todo: add "move" semantics to submit_task!
  inline void submit_task(async_task *task, event *after = NULL)
  {
    async_task *tmp = new async_task;
    assert(tmp != NULL);
    assert(task != NULL);
    memcpy(tmp, task, task->nbytes());
    
    // Increase the reference of the ack event of the task
    if (tmp->_ack != NULL) {
      tmp->_ack->incref();
    }
    
    if (after != NULL ) {
      after->lock();
      // add task to the callback list if the event is inflight
      //
      if (after->count() > 0) {
        after->add_done_cb(tmp);
        after->unlock();
        return;
      }
      after->unlock();
    }
    
    // enqueue the async task
    if (task->_callee == myrank()) {
      // local task
      assert(in_task_queue != NULL);
      gasnet_hsl_lock(&in_task_queue_lock);
      queue_enqueue(in_task_queue, tmp);
      gasnet_hsl_unlock(&in_task_queue_lock);
    } else {
      // remote task
      assert(out_task_queue != NULL);
      gasnet_hsl_lock(&out_task_queue_lock);
      queue_enqueue(out_task_queue, tmp);
      gasnet_hsl_unlock(&out_task_queue_lock);
    }
  } // end of submit_task
  
  struct am_bcast_arg {
    range target; // set of remote nodes
    uint32_t root_index;
    int use_domain;
    async_task task;
    //kernel kernel; // function kernel to be executed on remote nodes
    //size_t arg_sz; // argument size in bytes
    //char *args[0]; // arguments for the kernel
    
    inline size_t nbytes()
    {
      return sizeof(target) + sizeof(root_index) +
      sizeof(use_domain) + task.nbytes() + 4; // plus 4 for padding
    }
  };
  
  template<typename T>
  inline size_t aux_type_size() { return sizeof(T); }
  
  template<>
  inline size_t aux_type_size<void>() { return 0; }
  
  /**
   * \ingroup internalgroup
   * ganset_launcher internal function object for storing async function
   * state for later execution
   *
   * \tparam dest the type of the places where the async function
   *
   */
  template<typename dest = rank_t>
  struct gasnet_launcher {
  private:
    dest _there; // should we use node_t?
    event *_ack; // acknowledgment event
    event *_after; // predecessor event
    group _g;
    
  public:
    gasnet_launcher(dest there, event *ack, event *after=NULL)
    : _there(there), _ack(ack), _after(after), _g(group(1, -1))
    {
    }
    
    inline void set_dependent_event(event *dep)
    {
      _after = dep;
    }
    
    inline void set_group(group g)
    {
      _g = g;
    }
    
    /* launch a general kernel */
    void launch(generic_fp fp, void *async_args, size_t arg_sz);
    
    /* launch a general kernel */
    void launch(generic_fp fp, void *async_args, size_t arg_sz,
                void *rv, size_t rv_sz);
    
    /// \TODO: add implicit async launch
        
    typedef void (*K1s)(group);
    inline void operator()(K1s kernel)
    {
      group g(_g.size(), _g.index());
      generic_arg1<K1s, group> args(kernel, g);
      launch(async_wrapper1<K1s, group>, &args, (size_t)sizeof(args));
    }
        
    // template<typename Function>
    // inline future operator()(Function kernel)
    // {
    //   async_task task(myrank(), _there, _ack, kernel);
    //   submit_task(&task, _after);
    //   //launch((generic_fp)kernel, (void *)NULL, (size_t)0);
    // }

    #include "async_impl_templates2.h"
    
  }; // gasnet_launcher
  
  /// \endcond
} // namespace upcxx
