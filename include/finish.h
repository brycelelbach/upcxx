#ifndef FINISH_H_
#define FINISH_H_

#include "event.h"
#include "utils.h"

namespace upcxx {

  struct f_scope {
    int done;
    event e;
    inline f_scope():done(0)
    {
      push_event(&e);
    }
    inline ~f_scope() 
    {
      assert(peek_event() == &e);
      pop_event();
      e.wait();
    }
  };

#define finish finish_(UNIQUIFY(fs_))
#define finish_(name) for (f_scope name; name.done == 0; name.done = 1)

/* #define new_async(x) async(x, &_fs.e) */

}

#endif
