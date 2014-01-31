#include "async.h"
#include "active_coll.h"

using namespace upcxx;

template<>
void gasnet_launcher<node>::launch(generic_fp fp,
                                   void *async_args, 
                                   size_t arg_sz)
{
  async_task task;
  task.init_async_task(my_node.id(),
                       _there.id(),
                       _ack,
                       fp,
                       arg_sz,
                       async_args);
  if (_ack != NULL) {
    _ack->incref();
  }

  submit_task(&task, _after);
}

template<>
void gasnet_launcher<range>::launch(generic_fp fp,
                                    void *async_args,
                                    size_t arg_sz)
{
  if (_ack != NULL) {
    // _ack->incref(_there.count());
    _ack->incref(_g.size());
  }

  int remain_tasks = _g.size();
  while (remain_tasks > 0) {
    range target = _there;
    if (remain_tasks < _there.count()) {
      target = target - (_there.count() - remain_tasks);
    }
    // cout << target << "\n";
    am_bcast(target, _ack, fp, arg_sz, async_args, _after,
             _g.size()-remain_tasks,
             (_g.index() == -1 ? 0 : 1));
    remain_tasks -= target.count();
  }
}

