#include "async.h"
#include "active_coll.h"

using namespace upcxx;

template<>
void gasnet_launcher<rank_t>::launch(generic_fp fp,
                                     void *async_args,
                                     size_t arg_sz)
{
  async_task task;
  task.init_async_task(global_myrank(),
                       _there,
                       _ack,
                       fp,
                       arg_sz,
                       async_args);
  submit_task(&task, _after);
}

template<>
void gasnet_launcher<range>::launch(generic_fp fp,
                                    void *async_args,
                                    size_t arg_sz)
{
#if 0
  for (int i = 0; i < _there.count(); i++) {
    async_task task;
    task.init_async_task(global_myrank(),
                         _there[i],
                         _ack,
                         fp,
                         arg_sz,
                         async_args);
    submit_task(&task, _after);

  }
#else
  if (_ack != NULL) {
    if (_there.contains(global_myrank()))
      _ack->incref(_there.count()-1);
    else
      _ack->incref(_there.count());
  }
  am_bcast(_there, _ack, fp, arg_sz, async_args, _after, global_myrank());
#endif
}

