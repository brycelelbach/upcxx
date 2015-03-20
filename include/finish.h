#pragma once

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

#define upcxx_finish UPCXX_finish_(UPCXX_UNIQUIFY(fs_))
#define UPCXX_finish_(name)                                     \
  for (upcxx::f_scope name; name.done == 0; name.done = 1)

#ifdef UPCXX_SHORT_MACROS
# define finish upcxx_finish
#endif

/* #define new_async(x) async(x, &_fs.e) */

}
