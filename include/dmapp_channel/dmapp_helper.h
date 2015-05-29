#include <dmapp.h>

extern dmapp_seg_desc_t upcxx_dmapp_myseg;
extern dmapp_seg_desc_t *upcxx_dmapp_segs;
extern dmapp_rma_attrs_ext_t upcxx_dmapp_rma_args;
// dmapp_queue_handle_t upcxx_dmapp_queue;

namespace upcxx
{
  #define DMAPP_SAFE(FUNCALL)                 \
    do {                                      \
      dmapp_return_t rv = FUNCALL;            \
      if (rv != DMAPP_RC_SUCCESS) {           \
        const char *err_str;                  \
        dmapp_explain_error(rv, &err_str);    \
        gasneti_fatalerror("DMAPP fatal error (%s) at %s:%d\n", err_str, __FILE__, __LINE__); \
      } \
    } while(0)

  /* convert data size in bytes to nelems of dmapp_type_t */
  inline
  void bytes_to_dmapp_type(void *dst, void *src, size_t nbytes,
                           uint64_t *nelems, dmapp_type_t *type)
  {
    size_t d = (size_t)dst;
    size_t s = (size_t)src;
    if (nbytes % 16 == 0 && d % 16 == 0 && s % 16 == 0) { /* nbytes % 16 */
      *type = DMAPP_DQW;
      *nelems = nbytes >> 4; /* nbytes / 16 */
    } else if (nbytes % 8 == 0 && d % 8 == 0 && s % 8 ==0) { /* nbytes % 8 */
      *type = DMAPP_QW;
      *nelems = nbytes >> 3; /* nbytes / 8 */
    } else if (nbytes % 4  == 0 && d % 4 == 0 && s % 4 ==0) { /* nbytes % 4 */
      *type = DMAPP_DW;
      *nelems = nbytes >> 2; /* nbytes / 4 */
    } else {
      *type = DMAPP_BYTE;
      *nelems = nbytes;
    }
  }

  inline
  dmapp_seg_desc_t *get_dmapp_seg(uint32_t rank)
  {
    assert(rank < upcxx::ranks());
    return &upcxx_dmapp_segs[rank];
  }

  // Init Cray DMAPP for UPC++ w. GASNet
  // Should be called after gasnet_attach but before using any DMAPP features
  void init_dmapp();

} // end of upcxx name space
