#include "upcxx.h"

/// \cond SHOW_INTERNAL

namespace upcxx
{
  void parallel_group_t::barrier()
  {
    if (size() == global_machine.node_count()) {
      barrier();
    }
    /// \Todo No default parallel group barrier yet
  }
} // upcxx

/// \endcond

