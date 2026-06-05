/// @file arithmetic.h
/// @brief Basic arithmetic macros, value checks, and pseudo-random generation.

#ifndef __ARITHMETIC_H__
#define __ARITHMETIC_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef PRINT_HEADER_COMPILE_MESSAGE
#pragma message ("AppUtils/Arithmetic.h")
#endif

/// Standard definitions for NULL, size_t
#include <stddef.h>
/// Fixed-width integer types
#include <stdint.h>

/// @brief Return maximum of a and b
/// @warning Avoid side effects in arguments (e.g., Max(a++, b))
#define Max(a, b)           ((a) > (b) ? (a) : (b))

/// @brief Return minimum of a and b
/// @warning Avoid side effects in arguments
#define Min(a, b)           ((a) < (b) ? (a) : (b))

/// @brief Return absolute value of x
#define Abs(x)              ((x) >= 0 ? (x) : -(x))

/// @brief Check if pointer is NULL
#define IsNull(ptr)         ((ptr) == NULL)

/// @brief Check if pointer is NOT NULL
#define IsNotNull(ptr)      ((ptr) != NULL)

/// @brief Check if x is zero
#define IsZero(x)           ((x) == 0)

/// @brief Check if x is strictly positive (> 0)
#define IsPos(x)            ((x) > 0)

/// @brief Check if x is strictly negative (< 0)
#define IsNeg(x)            ((x) < 0)

/// @brief Check if x is non-zero
#define IsNotZero(x)        ((x) != 0)

/// @brief Check if x is not positive (<= 0)
#define IsNotPos(x)         ((x) <= 0)

/// @brief Check if x is not negative (>= 0)
#define IsNotNeg(x)         ((x) >= 0)


/// @brief Convert macro argument x to string literal
#define STR(x)              #x

/// @brief Cast x to strictly 1 (true) or 0 (false)
#define BooleanCast(x)      (((x) != 0) ? (1) : (0))


/// @brief Generates a pseudo-random number.
/// @details If the input seed is negative, the function reuses the last known random state.
///          Otherwise, it reseeds the generator with the new value.
/// @param SeedInput The input seed. Pass a negative value (e.g., -1) to continue the sequence.
/// @return A pseudo-random integer in the range [0, 9999] (or customized range).
uint32_t GenerateRandomNumber(uint32_t SeedInput);

#ifdef __cplusplus
}
#endif

#endif /// __ARITHMETIC_H__