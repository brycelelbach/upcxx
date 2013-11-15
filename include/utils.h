/**
 * utils.h - internal macro utilities
 */

#pragma once

#ifdef __COUNTER__
#define UNIQUIFY(prefix) CONCAT_(prefix, __COUNTER__)
#else
#define UNIQUIFY(prefix) CONCAT_(prefix, __LINE__)
#endif

#define UNIQUIFYN(prefix, name) UNIQUIFY(prefix ## name ## _)

#define CONCAT_(a, b) CONCAT__(a, b)
#define CONCAT__(a, b)  a ## b
