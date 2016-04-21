/*
 * active_message.cpp - implement a wrapper of GASNet active messages
 */
#include <iostream>

#include "upcxx.h"
#include "upcxx/upcxx_internal.h"

namespace upcxx
{
  GASNETT_INLINE(am_request2_inner)
  void am_request2_inner(gasnet_token_t token,
                         void *addr,
                         size_t nbytes,
                         void *fp,
                         int arg0,
                         int arg1)
  {
    gasnet_node_t src_rank;
    int rv = gasnet_AMGetMsgSource(token, &src_rank);
    assert(rv == GASNET_OK);
    assert(fp != NULL);
    am_handler2i_t handler_fp = (am_handler2i_t)fp;
    (*handler_fp)(src_rank, addr, nbytes, arg0, arg1);
  }
  MEDIUM_HANDLER(am_request2, 3, 4,
                 (token, addr, nbytes, UNPACK(a0),      a1, a2),
                 (token, addr, nbytes, UNPACK2(a0, a1), a2, a3));

  GASNETT_INLINE(am_request4_inner)
  void am_request4_inner(gasnet_token_t token,
                         void *addr,
                         size_t nbytes,
                         void *fp,
                         int arg0,
                         int arg1,
                         int arg2,
                         int arg3)
  {
    gasnet_node_t src_rank;
    int rv = gasnet_AMGetMsgSource(token, &src_rank);
    assert(rv == GASNET_OK);
    assert(fp != NULL);
    am_handler4i_t handler_fp = (am_handler4i_t)fp;
    (*handler_fp)(src_rank, addr, nbytes, arg0, arg1, arg2, arg3);
  }
  MEDIUM_HANDLER(am_request4, 5, 6,
                 (token, addr, nbytes, UNPACK(a0),      a1, a2, a3, a4),
                 (token, addr, nbytes, UNPACK2(a0, a1), a2, a3, a4, a5));

  GASNETT_INLINE(am_request2p2i_inner)
  void am_request2p2i_inner(gasnet_token_t token,
                            void *addr,
                            size_t nbytes,
                            void *fp,
                            void *p0,
                            void *p1,
                            int arg0,
                            int arg1)
    {
      gasnet_node_t src_rank;
      int rv = gasnet_AMGetMsgSource(token, &src_rank);
      assert(rv == GASNET_OK);
      assert(fp != NULL);
      am_handler2p2i_t handler_fp = (am_handler2p2i_t)fp;
      (*handler_fp)(src_rank, addr, nbytes, p0, p1, arg0, arg1);
    }
    MEDIUM_HANDLER(am_request2p2i, 5, 8,
                   (token, addr, nbytes, UNPACK(a0),      UNPACK(a1),      UNPACK(a2),      a3, a4),
                   (token, addr, nbytes, UNPACK2(a0, a1), UNPACK2(a2, a3), UNPACK2(a4, a5), a6, a7));

  void am_send(rank_t dst_rank, am_handler2i_t fp, void *payload, size_t nbytes, int arg0, int arg1)
  {
    UPCXX_CALL_GASNET(
        GASNET_CHECK_RV(MEDIUM_REQ(3, 4, (dst_rank, GENERIC_AM_REQUEST2,
                                          payload, nbytes,
                                          PACK(fp),
                                          arg0, arg1))));
  }

  void am_send(rank_t dst_rank, am_handler4i_t fp, void *payload, size_t nbytes,
               int arg0, int arg1, int arg2, int arg3)
  {
    UPCXX_CALL_GASNET(
        GASNET_CHECK_RV(MEDIUM_REQ(5, 6, (dst_rank, GENERIC_AM_REQUEST4,
                                          payload, nbytes,
                                          PACK(fp),
                                          arg0, arg1, arg2, arg3))));
  }

  void am_send(rank_t dst_rank, am_handler2p2i_t fp, void *payload, size_t nbytes,
               void *p0, void *p1, int arg0, int arg1)
  {
    UPCXX_CALL_GASNET(
           GASNET_CHECK_RV(MEDIUM_REQ(5, 8, (dst_rank, GENERIC_AM_REQUEST2P2I,
                                             payload, nbytes,
                                             PACK(fp), PACK(p0), PACK(p1),
                                             arg0, arg1))));
  }
}

