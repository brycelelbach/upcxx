#pragma once

#ifdef __cplusplus
extern "C" {
#endif
#include <gasnet_handler.h>
#ifdef __cplusplus
} /* extern "C" */
#endif
#include "gasnet_api.h"

namespace upcxx {
  /*
    initialize prealloc and pipelining by reading console environment
    variables
  */
  void array_bulk_init();

  /* handler indices */
  enum array_am_index_t {
    gasneti_handleridx(misc_delete_request) =
    ARRAY_MISC_DELETE_REQUEST,
    gasneti_handleridx(misc_alloc_request) =
    ARRAY_MISC_ALLOC_REQUEST,
    gasneti_handleridx(misc_alloc_reply) =
    ARRAY_MISC_ALLOC_REPLY,
    gasneti_handleridx(strided_pack_request) =
    ARRAY_STRIDED_PACK_REQUEST,
    gasneti_handleridx(strided_pack_reply) =
    ARRAY_STRIDED_PACK_REPLY,
    gasneti_handleridx(strided_unpackAll_request) =
    ARRAY_STRIDED_UNPACKALL_REQUEST,
    gasneti_handleridx(strided_unpack_reply) =
    ARRAY_STRIDED_UNPACK_REPLY,
    gasneti_handleridx(strided_unpackOnly_request) =
    ARRAY_STRIDED_UNPACKONLY_REQUEST,
    gasneti_handleridx(sparse_simpleScatter_request) =
    ARRAY_SPARSE_SIMPLESCATTER_REQUEST,
    gasneti_handleridx(sparse_done_reply) =
    ARRAY_SPARSE_DONE_REPLY,
    gasneti_handleridx(sparse_generalScatter_request) =
    ARRAY_SPARSE_GENERALSCATTER_REQUEST,
    gasneti_handleridx(sparse_simpleGather_request) =
    ARRAY_SPARSE_SIMPLEGATHER_REQUEST,
    gasneti_handleridx(sparse_simpleGather_reply) =
    ARRAY_SPARSE_SIMPLEGATHER_REPLY,
    gasneti_handleridx(sparse_generalGather_request) =
    ARRAY_SPARSE_GENERALGATHER_REQUEST,
  };

  /* handler declarations */

  SHORT_HANDLER_DECL(misc_delete_request, 1, 2);
  SHORT_HANDLER_DECL(misc_alloc_request, 2, 4);
  SHORT_HANDLER_DECL(misc_alloc_reply, 2, 4);

  MEDIUM_HANDLER_DECL(strided_pack_request, 3, 6);
  MEDIUM_HANDLER_DECL(strided_pack_reply, 4, 8);
  MEDIUM_HANDLER_DECL(strided_unpackAll_request, 3, 6);
  SHORT_HANDLER_DECL(strided_unpack_reply, 1, 2);
  MEDIUM_HANDLER_DECL(strided_unpackOnly_request, 3, 6);

  MEDIUM_HANDLER_DECL(sparse_simpleScatter_request, 3, 6);
  SHORT_HANDLER_DECL(sparse_done_reply, 1, 2);
  SHORT_HANDLER_DECL(sparse_generalScatter_request, 4, 8);
  MEDIUM_HANDLER_DECL(sparse_simpleGather_request, 4, 8);
  MEDIUM_HANDLER_DECL(sparse_simpleGather_reply, 4, 8);
  SHORT_HANDLER_DECL(sparse_generalGather_request, 4, 8);
} //namespace upcxx
