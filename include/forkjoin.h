#pragma once

/**
 * start_gasnet.h -- Single-thread start up emulation 
 *
 * This file should only be included in the "main" cpp file where the
 * user code defines the main entry function.  UPCXX starts a parallel
 * job in SPMD mode.  To emulate the fork-join (async) execution
 * model, we first redefine the main function entry point as the user
 * main function.  We supply a stub main function for job start-up and
 * setting up the fork-join (async) execution model.
 */

#include "upcxx.h"

extern int _user_main(int argc, char **argv);

namespace upcxx
{
  void signal_exit(); 
  void wait_for_incoming_tasks();
}

int main(int argc, char **argv)
{
  int rv = 0;

  upcxx::init(&argc, &argv);

  // The master spawns tasks and the slaves wait.
  if (upcxx::myrank() == 0) {
    // start the user main function
    rv = _user_main(argc, argv);
    upcxx::signal_exit();
  } else {
    upcxx::wait_for_incoming_tasks();
  }

  upcxx::finalize();

  return rv;
}

#define main _user_main

const char *forkjoin_h_include_error = "forkjoin.h should only be included once in the main cpp file where user's main() function is defined!";
