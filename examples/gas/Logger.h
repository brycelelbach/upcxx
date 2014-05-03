#pragma once

#include "execution_log.h"

class Logger {

  static string bb;
  static string eb;

  static bool logging;  // are you using the logger?

  static execution_log log;

public:
  static int barrierCount;

  static void barrier() { 
    if (logging) {
      barrierCount++;
      log.append(bb);
      upcxx::barrier();
      log.append(eb);
    } else {
      upcxx::barrier();
    }
  }

  static void begin() {
    log = execution_log();
    log.init();
    barrierCount = 0;
    logging = true;
  }

  static void end() {
    if (logging) {
      execution_log::end(log);
    }
  }

  static void append(const string &s) {
    if (logging) {
      log.append(s);
    }
  }
};
