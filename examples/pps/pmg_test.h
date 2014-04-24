#pragma once

#include <string>
#include "globals.h"
#include "execution_log.h"
#include "charge.h"

class pmg_test {
private:
  static int barrierCount;
  static string bb;
  static string eb;
public:
  static execution_log log;

  static void barrier() { 
    barrierCount++;
    log.append(2, bb);
    upcxx::barrier();
    log.append(2, eb);
  }

  static void main(int argc, char **argv);

  static ndarray<charge, 1> parse_input_file(const string &filename);
};
