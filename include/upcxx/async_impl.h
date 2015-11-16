/*
 * aysnc_impl.h - implementation details of asynchronous tasks
 */

#pragma once

#include <iostream>

#include "gasnet_api.h"
#include "event.h"
#include "future.h"
#ifdef UPCXX_HAVE_CXX11
#include <type_traits> 
#else
#include "async_templates.h"
#endif
#include "active_coll.h"
#include "group.h"
#include "range.h"
#include "team.h"
#include "global_ptr.h"
#include "utils.h"

// #define UPCXX_DEBUG
#define UPCXX_APPLY_IMPL2

namespace upcxx
{
#ifdef UPCXX_HAVE_CXX11

#ifdef UPCXX_APPLY_IMPL1
  template<typename Function, typename... Ts>
  struct generic_arg {
    Function kernel;
    std::tuple<Ts...> args;

    generic_arg(Function k, Ts... as) :
      kernel(k), args{as...} {}

    void apply() {
      upcxx::apply(kernel, args);
    }
  };
#endif // UPCXX_APPLY_IMPL1

#ifdef UPCXX_APPLY_IMPL2
  template<int ...>
  struct seq { };

  template<int N, int ...S>
  struct gens : gens<N-1, N-1, S...> { };

  template<int ...S>
  struct gens<0, S...> {
    typedef seq<S...> type;
  };

  template<typename Function, typename... Ts>
  struct generic_arg {
    Function kernel;
    future_storage_t *fs_ptr;
    std::tuple<Ts...> args;

    generic_arg(Function k, Ts... as) :
      kernel(k), args(as...), fs_ptr(NULL) {}

    generic_arg(future_storage_t *rv_ptr, Function k, Ts... as) :
      kernel(k), args(as...), fs_ptr(rv_ptr) {}

    // The return type of Function is non-void
    template<typename F = Function, int ...S>
    typename std::enable_if<!std::is_void<typename std::result_of<F(Ts...)>::type>::value>::type*
    callFunc(seq<S...>)
    {
      typename std::result_of<Function(Ts...)>::type rv;
      rv = kernel(std::get<S>(args) ...);
      if (fs_ptr != NULL) {
        // This fs_ptr is the pointer on task initiator
        // fs_ptr->store(rv);
        future_storage_t *tmp_fs;
        tmp_fs = new future_storage_t(rv);
        return tmp_fs;
      }
      return NULL;
    }

    // The return type of Function is void
    template<typename F = Function, int ...S>
    typename std::enable_if<std::is_void<typename std::result_of<F(Ts...)>::type>::value>::type*
    callFunc(seq<S...>)
    {
      kernel(std::get<S>(args) ...);
      return NULL;
    }

    void* apply()
    {
      // upcxx::apply(kernel, args);
      return callFunc(typename gens<sizeof...(Ts)>::type());
    }
  }; // close of struct generic_arg
#endif // UPCXX_APPLY_IMPL2

  /* Active Message wrapper function */
  template <typename Function, typename... Ts>
  void* async_wrapper(void *args) {
    generic_arg<Function, Ts...> *a =
      (generic_arg<Function, Ts...> *) args;
    return a->apply();
  }
#endif

#define MAX_ASYNC_ARG_COUNT 16 // max number of arguments
#define MAX_ASYNC_ARG_SIZE 512 // max size of all arguments (in nbytes)
  
  /// \cond SHOW_INTERNAL
  struct async_task  {
    rank_t _caller; // the place where async is called, for reply
    rank_t _callee; // the place where the task should be executed
    event *_ack; // Acknowledgment event pointer on caller node
    generic_fp _fp;
    void *_am_src; // active message src buffer
    void *_am_dst; // active message dst buffer
    size_t _arg_sz;
    char _args[MAX_ASYNC_ARG_SIZE];
    
    inline async_task()
        : _caller(0), _callee(0), _ack(NULL), _fp(NULL),
          _am_src(NULL), _am_dst(NULL), _arg_sz(0) { };

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
    
    inline size_t nbytes(void)
    {
      return (sizeof(async_task) - MAX_ASYNC_ARG_SIZE + _arg_sz);
    }

#ifndef UPCXX_HAVE_CXX11
# include "async_impl_templates1.h"
#else
    template<typename Function, typename... Ts>
    inline async_task(rank_t caller, rank_t callee, event *ack,
                      Function k, const Ts &... as) {
      generic_arg<Function, Ts...> args(k, as...);
      init_async_task(caller, callee, ack, async_wrapper<Function, Ts...>,
                      (size_t) sizeof(args), (void *) &args);
    }
#endif
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
    future_storage_t *fs;
    char future_val[0];
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
    if (tmp->_caller == global_myrank() && tmp->_ack != NULL) {
      tmp->_ack->incref();
    }
    
    if (after != NULL ) {
      upcxx_mutex_lock(&all_events_lock);
      // add task to the callback list if the event is in flight
      if (after->count() > 0) {
        after->_add_done_cb(tmp);
        upcxx_mutex_unlock(&all_events_lock);
        return;
      }
      upcxx_mutex_unlock(&all_events_lock);
    }
    
    // enqueue the async task
    if (task->_callee == global_myrank()) {
      // local task
      assert(in_task_queue != NULL);
      upcxx_mutex_lock(&in_task_queue_lock);
      queue_enqueue(in_task_queue, tmp);
      upcxx_mutex_unlock(&in_task_queue_lock);
    } else {
      // remote task
      assert(out_task_queue != NULL);
      upcxx_mutex_lock(&out_task_queue_lock);
      queue_enqueue(out_task_queue, tmp);
      upcxx_mutex_unlock(&out_task_queue_lock);
    }
  } // end of submit_task
  
  struct am_bcast_arg {
    range target; // set of remote nodes
    uint32_t root_index;
    async_task task;
    
    inline size_t nbytes()
    {
      return sizeof(target) + sizeof(root_index) + task.nbytes();
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
    
#ifndef UPCXX_HAVE_CXX11
# include "async_impl_templates2.h"
#else
    template<typename Function, typename... Ts>
    inline future<typename std::result_of<Function(Ts...)>::type>
    operator()(Function k, const Ts &... as)
    {
      future<typename std::result_of<Function(Ts...)>::type> rv;
      generic_arg<Function, Ts...> args(rv.ptr(), k, as...);
      launch(async_wrapper<Function, Ts...>, (void *) &args,
             sizeof(args));
      return rv;
    }
#endif
  }; // gasnet_launcher
  
  /// \endcond
} // namespace upcxx
