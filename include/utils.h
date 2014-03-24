/**
 * utils.h - internal macro utilities
 */

#pragma once

#ifdef __COUNTER__
#define UPCXX_UNIQUIFY(prefix) UPCXX_CONCAT_(prefix, __COUNTER__)
#else
#define UPCXX_UNIQUIFY(prefix) UPCXX_CONCAT_(prefix, __LINE__)
#endif

#define UPCXX_UNIQUIFYN(prefix, name)           \
  UPCXX_UNIQUIFY(prefix ## name ## _)

#define UPCXX_CONCAT_(a, b) UPCXX_CONCAT__(a, b)
#define UPCXX_CONCAT__(a, b)  a ## b
