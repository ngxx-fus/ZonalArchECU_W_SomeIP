#ifndef __APP_UTILS_LOOP_H__
#define __APP_UTILS_LOOP_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef PRINT_HEADER_COMPILE_MESSAGE
#pragma message ("AppUtils/Loop.h")
#endif


#include <stdint.h>

/// @brief Loop forward from `a` to `b-1`.
/// @param type Loop variable type.
/// @param i Loop variable.
/// @param a Start value (inclusive).
/// @param b End value (exclusive).
#define REPT(type, i, a, b) for(type i = (a); (i) < (b); ++(i))

/// @brief Loop backward from `a` to `b+1`.
/// @param type Loop variable type.
/// @param i Loop variable.
/// @param a Start value (inclusive).
/// @param b End value (exclusive).
#define REVT(type, i, a, b) for(type i = (a); (i) > (b); --(i))

/// @brief Loop forward using int32_t.
/// @param i Loop variable.
/// @param a Start value (inclusive).
/// @param b End value (exclusive).
#define REP(i, a, b) for(int32_t i = (a); (i) < (b); ++(i))

/// @brief Loop backward using int32_t.
/// @param i Loop variable.
/// @param a Start value (inclusive).
/// @param b End value (exclusive).
#define REV(i, a, b) for(int32_t i = (a); (i) > (b); --(i))

/// @brief Loop forward using int32_t.
/// @param i Loop variable.
/// @param N End value (exclusive).
#define REPN(i, N) for(int32_t i = (0); (i) < (N); ++(i))


#ifdef __cplusplus
}
#endif

#endif /// __APP_UTILS_LOOP_H__