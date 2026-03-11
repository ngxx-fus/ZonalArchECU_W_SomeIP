/// @file   flag.h
/// @brief  Bit-flag utilities; provides fast non-atomic flag ops and thread-safe atomic flag manipulation.

#ifndef __FLAG_H__
#define __FLAG_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef PRINT_HEADER_COMPILE_MESSAGE
#pragma message ("AppUtils/FlagControl.h")
#endif

/// Standard input/output definitions
#include <stdio.h>
/// Fixed-width integer types (uint8_t, int32_t, etc.)
#include <stdint.h>
/// Variable arguments handling
#include <stdarg.h>
/// Standard atomic operations for thread safety
#include <stdatomic.h>

/* --- HELPERS --- */

#ifndef Mask32
    /// @brief  Generate a 32-bit mask with bit 'i' set (1 << i).
    #define Mask32(i)            (1UL << (i))
#endif

#ifndef InvMask32
    /// @brief  Generate a 32-bit mask with bit 'i' cleared (~(1 << i)).
    #define InvMask32(i)         (~(1UL << (i)))
#endif

/* --- UNSAFE FLAGS (Fast, Single Thread) --- */

/// @brief Standard 32-bit flag type for non-atomic bitwise operations.
typedef uint32_t Flag_t;

/// @brief  Set the bit at position i in a 32-bit flag (Unsafe).
/// @param  flag Variable of type Flag_t.
/// @param  i    Bit position (0-31).
#define FlagSet(flag, i)        ((flag) |= Mask32(i))

/// @brief  Clear the bit at position i in a 32-bit flag (Unsafe).
/// @param  flag Variable of type Flag_t.
/// @param  i    Bit position (0-31).
#define FlagClear(flag, i)      ((flag) &= InvMask32(i))

/// @brief  Toggle the bit at position i in a 32-bit flag (Unsafe).
/// @param  flag Variable of type Flag_t.
/// @param  i    Bit position (0-31).
#define FlagToggle(flag, i)     ((flag) ^= Mask32(i))

/// @brief  Check if bit i is SET (returns true/non-zero).
/// @param  flag Variable of type Flag_t.
/// @param  i    Bit position (0-31).
#define FlagHas(flag, i)        (((flag) & Mask32(i)) != 0U)

/// @brief  Check if bit i is CLEARED (returns true/non-zero).
/// @param  flag Variable of type Flag_t.
/// @param  i    Bit position (0-31).
#define FlagIsClear(flag, i)    (((flag) & Mask32(i)) == 0U)


/* --- SAFE FLAGS (Thread-Safe, ISR-Safe) --- */

/// @brief Atomic 32-bit flag type, safe for use in ISRs and Multi-threaded environments.
typedef volatile atomic_uint_fast32_t SafeFlag_t;

/// @brief  Atomically set a bit (Thread-safe).
/// @param  f Pointer to the SafeFlag_t variable.
/// @param  i Bit position (0-31).
void SafeFlagSet(SafeFlag_t *f, uint32_t i);

/// @brief  Atomically clear a bit (Thread-safe).
/// @param  f Pointer to the SafeFlag_t variable.
/// @param  i Bit position (0-31).
void SafeFlagClear(SafeFlag_t *f, uint32_t i);

/// @brief  Atomically toggle a bit (Thread-safe).
/// @param  f Pointer to the SafeFlag_t variable.
/// @param  i Bit position (0-31).
void SafeFlagToggle(SafeFlag_t *f, uint32_t i);

/// @brief  Atomically check if a bit is set (Thread-safe).
/// @param  f Pointer to the SafeFlag_t variable.
/// @param  i Bit position (0-31).
/// @return 1 if set, 0 otherwise.
DefaultRet_t SafeFlagHas(SafeFlag_t *f, uint32_t i);

#ifdef __cplusplus
}
#endif

#endif // __FLAG_H__