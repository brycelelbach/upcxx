/*
 * active_message.h - implement a wrapper of GASNet active messages
 */

#pragma once

#include <iostream>

#include "gasnet_api.h"
#include "upcxx_runtime.h"

namespace upcxx
{

  inline size_t max_am_payload_size() { return gasnet_AMMaxMedium() - 32; };
  typedef void (*am_handler2i_t)(uint32_t, void*, size_t, int, int);
  void am_send(rank_t dst_rank, am_handler2i_t am_handler_fp, void *payload, size_t nbytes,
               int arg0, int arg1);

  typedef void (*am_handler4i_t)(uint32_t, void*, size_t, int, int, int, int);
  void am_send(rank_t dst_rank, am_handler4i_t am_handler_fp, void *payload, size_t nbytes,
               int arg0, int arg1, int arg2, int arg3);

  typedef void (*am_handler2p2i_t)(uint32_t, void*, size_t, void *, void *, int, int);
  void am_send(rank_t dst_rank, am_handler2p2i_t am_handler_fp, void *payload, size_t nbytes,
               void *p0, void *p1, int arg0, int arg1);
}
