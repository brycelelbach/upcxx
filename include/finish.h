#ifndef FINISH_H_
#define FINISH_H_

#include "event.h"

namespace upcxx {

  struct f_scope {
    int done;
    event e;
    inline f_scope():done(0) { }
    inline ~f_scope() 
    {
      e.wait();
    }
  };

#define finish for (f_scope _fs; _fs.done == 0; _fs.done = 1)

#define new_async(x) async(x, &_fs.e)

}

#endif
