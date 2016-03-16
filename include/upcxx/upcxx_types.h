#ifndef UPCXX_TYPES_H_
#define UPCXX_TYPES_H_

/**
 * @brief Return status of UPCXX functions
 * \todo extend the set of return values to indicate different types of errors
 */
#define UPCXX_SUCCESS 0
#define UPCXX_ERROR 1

/**
 * @brief Built-in UPCXX operations for collectives
 */
typedef enum {
  UPCXX_MAX = 0,
  UPCXX_MIN,
  UPCXX_SUM,
  UPCXX_PROD,
  UPCXX_LAND,
  UPCXX_BAND,
  UPCXX_LOR,
  UPCXX_BOR,
  UPCXX_LXOR,
  UPCXX_BXOR,
  UPCXX_OP_COUNT
} upcxx_op_t;

/**
 * @brief Built-in UPCXX data types for collectives.
 */
typedef enum
{
  UPCXX_CHAR, /* signed character, 1 byte */
  UPCXX_UCHAR, /* unsigned character, 1 byte */
  UPCXX_SHORT, /* signed short, 2 bytes */
  UPCXX_USHORT, /* unsigned short, 2 bytes */
  UPCXX_INT, /* signed integer, 4 bytes */
  UPCXX_UINT, /* unsigned integer, 4 bytes */
  UPCXX_LONG, /* signed integer, 4 or 8 bytes */
  UPCXX_ULONG, /* unsigned integer, 4 or 8 bytes */
  UPCXX_LONG_LONG, /* signed integer, 8 bytes */
  UPCXX_ULONG_LONG, /* unsigned integer, 8 bytes */
  UPCXX_FLOAT, /* IEEE floating point, 4 bytes */
  UPCXX_DOUBLE, /* IEEE double precision, 8 bytes */
  UPCXX_COMPLEX_FLOAT, /* Pair of FLOATs {Re, Im}, 8 bytes */
  UPCXX_COMPLEX_DOUBLE, /* Pair of DOUBLEs {Re, Im}, 16 bytes */
  UPCXX_DATATYPE_COUNT
} upcxx_datatype_t;

namespace upcxx {
  typedef uint64_t flag_t;

  const flag_t UPCXX_FLAG_VAL_SET   = 1;
  const flag_t UPCXX_FLAG_VAL_UNSET = 0;
}

#endif // UPCXX_TYPES_H_
