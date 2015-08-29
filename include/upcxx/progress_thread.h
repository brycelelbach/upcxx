#pragma once

namespace upcxx
{
  // begin progress thread execution (executes progress_helper())
  void progress_thread_start();

  // signal to stop the progress thread and wait on it
  void progress_thread_stop();
}
