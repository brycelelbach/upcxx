#pragma once

namespace upcxx
{
  // begin progress thread execution (executes progress_helper())
  void progress_thread_start();

  // signal to pause the progress thread (block it on a condition variable)
  void progress_thread_pause();

  // signal to stop the progress thread and call pthread_join()
  void progress_thread_stop();
}
