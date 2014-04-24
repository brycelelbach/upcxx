/**
 * aysnc.h - asynchronous task execution
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

using namespace std;

// #define DEBUG

namespace upcxx
{
#define MAX_ASYNC_ARG_COUNT 8 // max number of arguments
#define MAX_ASYNC_ARG_SIZE 512 // max size of all arguments (in nbytes)

  /// \cond SHOW_INTERNAL
  struct async_task  {
    rank_t _caller; // the place where async is called, for reply
    rank_t _callee; // the place where the task should be executed
    event *_ack; // Acknowledgment event pointer on caller node
    generic_fp _fp;  // use an index to a function table instead?
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

    template<typename Function>
    inline async_task(rank_t caller, rank_t callee, event *ack,
                      Function kernel)
    {
      init_async_task(caller, callee, ack,
                      (generic_fp)kernel, 0, (void *)NULL);
    }

    template<typename Function, typename T1>
    inline async_task(rank_t caller, rank_t callee, event *ack,
                      Function kernel, const T1 &a1)
    {
      generic_arg1<Function, T1> args(kernel, a1);
      init_async_task(caller, callee, ack,
                      async_wrapper1<Function, T1>,
                      (size_t)sizeof(args),
                      (void *)&args);
    }

    template<typename Function, typename T1, typename T2>
    inline async_task(rank_t caller, rank_t callee, event *ack,
                      Function kernel, const T1& a1, const T2& a2)
    {
      generic_arg2<Function, T1, T2> args(kernel, a1, a2);
      init_async_task(caller, callee, ack,
                      async_wrapper2<Function, T1, T2>,
                      (size_t)sizeof(args),
                      (void *)&args);
    }

    template<typename Function, typename T1, typename T2, typename T3>
    inline async_task(rank_t caller, rank_t callee, event *ack,
                      Function kernel, const T1& a1, const T2& a2, const T3& a3)
    {
      generic_arg3<Function, T1, T2, T3> args(kernel, a1, a2, a3);
      init_async_task(caller, callee, ack,
                      async_wrapper3<Function, T1, T2, T3>,
                      (size_t)sizeof(args),
                      (void *)&args);
    }

    template<typename Function, typename T1, typename T2, typename T3,
             typename T4>
    inline async_task(rank_t caller, rank_t callee, event *ack,
                      Function kernel, const T1& a1, const T2& a2, const T3& a3,
                      const T4& a4)
    {
      generic_arg4<Function, T1, T2, T3, T4> args(kernel, a1, a2, a3, a4);
      init_async_task(caller, callee, ack,
                      async_wrapper4<Function, T1, T2, T3, T4>,
                      (size_t)sizeof(args),
                      (void *)&args);
    }

    template<typename Function, typename T1, typename T2, typename T3,
             typename T4, typename T5>
    inline async_task(rank_t caller, rank_t callee, event *ack,
                      Function kernel, const T1& a1, const T2& a2, const T3& a3,
                      const T4& a4, const T5& a5)
    {
      generic_arg5<Function, T1, T2, T3, T4, T5>
        args(kernel, a1, a2, a3, a4, a5);

      init_async_task(caller, callee, ack,
                      async_wrapper5<Function, T1, T2, T3, T4, T5>,
                      (size_t)sizeof(args),
                      (void *)&args);
    }

    template<typename Function, typename T1, typename T2, typename T3,
             typename T4, typename T5, typename T6>
    inline async_task(rank_t caller, rank_t callee, event *ack,
                      Function kernel, const T1& a1, const T2& a2, const T3& a3,
                      const T4& a4, const T5& a5, const T6& a6)
    {
      generic_arg6<Function, T1, T2, T3, T4, T5, T6>
        args(kernel, a1, a2, a3, a4, a5, a6);

      init_async_task(caller, callee, ack,
                      async_wrapper6<Function, T1, T2, T3, T4, T5, T6>,
                      (size_t)sizeof(args),
                      (void *)&args);
    }

    template<typename Function, typename T1, typename T2, typename T3,
             typename T4, typename T5, typename T6, typename T7>
    inline async_task(rank_t caller, rank_t callee, event *ack,
                      Function kernel, const T1& a1, const T2& a2, const T3& a3,
                      const T4& a4, const T5& a5, const T6& a6, const T7& a7)
    {
      generic_arg7<Function, T1, T2, T3, T4, T5, T6, T7>
        args(kernel, a1, a2, a3, a4, a5, a6, a7);

      init_async_task(caller, callee, ack,
                      async_wrapper7<Function, T1, T2, T3, T4, T5, T6, T7>,
                      (size_t)sizeof(args),
                      (void *)&args);
    }

    template<typename Function, typename T1, typename T2, typename T3,
             typename T4, typename T5, typename T6, typename T7, typename T8>
    inline async_task(rank_t caller, rank_t callee, event *ack,
                      Function kernel, const T1& a1, const T2& a2, const T3& a3,
                      const T4& a4, const T5& a5, const T6& a6, const T7& a7,
                      const T8& a8)
    {
      generic_arg8<Function, T1, T2, T3, T4, T5, T6, T7, T8>
        args(kernel, a1, a2, a3, a4, a5, a6, a7, a8);

      init_async_task(caller, callee, ack,
                      async_wrapper8<Function, T1, T2, T3, T4, T5, T6, T7, T8>,
                      (size_t)sizeof(args),
                      (void *)&args);
    }

    inline size_t nbytes(void)
    {
      return (sizeof(_caller) + sizeof(_caller) + sizeof(_ack)
              + sizeof(_fp) + sizeof(_arg_sz) + _arg_sz);
    }

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
       outstanding_events.push_back(tmp->_ack);
    }

    if (after != NULL) {
      after->add_done_cb(tmp);
      return;
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

    template<typename Function>
    inline void operator()(Function kernel)
    {
      launch((generic_fp)kernel, (void *)NULL, (size_t)0);
    }

    typedef void (*K1s)(group);
    inline void operator()(K1s kernel)
    {
      group g(_g.size(), _g.index());
      generic_arg1<K1s, group> args(kernel, g);
      launch(async_wrapper1<K1s, group>, &args, (size_t)sizeof(args));
    }

    template<typename Function, typename T1>
    inline void operator()(Function kernel, T1 a1)
    {
      generic_arg1<Function, T1> args(kernel, a1);
      launch(async_wrapper1<Function, T1>,
             (void *)&args, (size_t)sizeof(args));
    }

    template<typename Function, typename T1, typename T2>
    inline void operator()(Function kernel, T1 a1, T2 a2)
    {
      generic_arg2<Function, T1, T2> args(kernel, a1, a2);
      launch(async_wrapper2<Function, T1, T2>,
             (void *)&args, (size_t)sizeof(args));
    }

    template<typename Function, typename T1, typename T2, typename T3>
    inline void operator()(Function kernel, T1 a1, T2 a2, T3 a3)
    {
      generic_arg3<Function, T1, T2, T3> args(kernel, a1, a2, a3);
      launch(async_wrapper3<Function, T1, T2, T3>,
             (void *)&args, (size_t)sizeof(args));
    }

    template<typename Function, typename T1, typename T2, typename T3,
             typename T4>
    inline void operator()(Function kernel, T1 a1, T2 a2, T3 a3, T4 a4)
    {
      generic_arg4<Function, T1, T2, T3, T4> args(kernel, a1, a2, a3, a4);
      launch(async_wrapper4<Function, T1, T2, T3, T4>,
             (void *)&args, (size_t)sizeof(args));
    }

    template<typename Function, typename T1, typename T2, typename T3,
             typename T4, typename T5>
    inline void operator()(Function kernel, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5)
    {
      generic_arg5<Function, T1, T2, T3, T4, T5>
        args(kernel, a1, a2, a3, a4, a5);

      launch(async_wrapper5<Function, T1, T2, T3, T4, T5>,
             (void *)&args, (size_t)sizeof(args));
    }

    template<typename Function, typename T1, typename T2, typename T3,
             typename T4, typename T5, typename T6>
    inline void operator()(Function kernel, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5,
                           T6 a6)
    {
      generic_arg6<Function, T1, T2, T3, T4, T5, T6>
        args(kernel, a1, a2, a3, a4, a5, a6);

      launch(async_wrapper6<Function, T1, T2, T3, T4, T5, T6>,
             (void *)&args, (size_t)sizeof(args));
    }

    template<typename Function, typename T1, typename T2, typename T3,
             typename T4, typename T5, typename T6, typename T7>
    inline void operator()(Function kernel, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5,
                           T6 a6, T7 a7)
    {
      generic_arg7<Function, T1, T2, T3, T4, T5, T6, T7>
        args(kernel, a1, a2, a3, a4, a5, a6, a7);

      launch(async_wrapper7<Function, T1, T2, T3, T4, T5, T6, T7>,
             (void *)&args, (size_t)sizeof(args));
    }

    template<typename Function, typename T1, typename T2, typename T3,
             typename T4, typename T5, typename T6, typename T7, typename T8>
    inline void operator()(Function kernel, T1 a1, T2 a2, T3 a3, T4 a4, T5 a5,
                           T6 a6, T7 a7, T8 a8)
    {
      generic_arg8<Function, T1, T2, T3, T4, T5, T6, T7, T8>
        args(kernel, a1, a2, a3, a4, a5, a6, a7, a8);

      launch(async_wrapper8<Function, T1, T2, T3, T4, T5, T6, T7, T8>,
             (void *)&args, (size_t)sizeof(args));
    }

  }; // gasnet_launcher

  /// \endcond

  /**
   * \ingroup asyncgroup
   *
   * Asynchronous function execution
   * Optionally signal the event "ack" for task completion
   *
   * ~~~~~~~~~~~~~~~{.cpp}
   * async(rank_t rank, event *ack)(function, arg1, arg2, ...);
   * ~~~~~~~~~~~~~~~
   * \see test_async.cpp
   *
   */
  inline gasnet_launcher<rank_t> async(rank_t rank,
                                       event *e = peek_event())
  {
    return gasnet_launcher<rank_t>(rank, e);
  }

  /*
  inline gasnet_launcher<rank_t> async(rank_t there,
                                       event *e = peek_event())
  {
    return gasnet_launcher<rank_t>(there, e);
  }
  */
  /**
   * \ingroup asyncgroup
   *
   * Asynchronous function execution
   *
   * ~~~~~~~~~~~~~~~{.cpp}
   * async(range ranks)(function, arg1, arg2, ...);
   * ~~~~~~~~~~~~~~~
   * \see test_am_bcast.cpp
   *
   */
  inline gasnet_launcher<range> async(range r,
                                      event *e = peek_event())
  {
    gasnet_launcher<range> launcher(r, e);
    launcher.set_group(group(r.count(), -1));
    return launcher;
  }

  /**
   * \ingroup asyncgroup
   *
   * Conditional asynchronous function execution
   * The task will be automatically enqueued for execution after
   * the event "after" is signalled.
   * Optionally signal the event "ack" for task completion
   *
   * ~~~~~~~~~~~~~~~{.cpp}
   * async_after(uint32_t rank, event *after, event *ack)(function, arg1, arg2, ...);
   * ~~~~~~~~~~~~~~~
   * \see test_async.cpp
   *
   */
  inline gasnet_launcher<rank_t> async_after(rank_t rank, event *after,
                                           event *ack = peek_event())
  {
    return gasnet_launcher<rank_t>(rank, ack, after);
  }

  // YZ: this overloaded async would cause compiler ambiguity
  // when calling async(rank, event)
  /*
  inline gasnet_launcher<rank_t> async(rank_t there, event *after,
                                     event *ack = peek_event())
  {
    return gasnet_launcher<rank_t>(there, ack, after);
  }
  */

  template<>
  void gasnet_launcher<rank_t>::launch(generic_fp fp,
                                       void *async_args,
                                       size_t arg_sz);
    
  template<>
  void gasnet_launcher<range>::launch(generic_fp fp,
                                      void *async_args,
                                      size_t arg_sz);

} // namespace upcxx
