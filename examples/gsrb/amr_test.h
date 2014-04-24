#pragma once

#include <string>
#include "globals.h"
#include "execution_log.h"

class amr_test {
public:
  static execution_log log;
  static string bb;
  static string eb;

  static inline void barrier() {
    //    log.append(2, bb);
    upcxx::barrier();
    //    log.append(2, eb);
  }
};
