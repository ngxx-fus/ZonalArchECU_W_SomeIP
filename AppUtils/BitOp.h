/// @file bit_op.h
/// @brief Bit manipulation utilities, including mask generation and variadic bitwise OR operations.

#ifndef __BIT_OP_H__
#define __BIT_OP_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef PRINT_HEADER_COMPILE_MESSAGE
#pragma message ("AppUtils/BitOp.h")
#endif


/// Standard input/output definitions
#include <stdio.h>
/// Fixed-width integer types (uint8_t, int32_t, etc.)
#include <stdint.h>
/// Variable arguments handling
#include <stdarg.h>
/// Boolean type definitions
#include <stdbool.h>
/// Standard atomic operations for thread safety
#include <stdatomic.h>

/* --- BASIC MASK GENERATION --- */

/// @brief Generate an 8-bit mask with the i-th bit set
/// @param i Bit position (0-7)
#define Mask8(i)           ((uint8_t)(1U << (i)))

/// @brief Generate a 16-bit mask with the i-th bit set
/// @param i Bit position (0-15)
#define Mask16(i)          ((uint16_t)(1U << (i)))

/// @brief Generate a 32-bit mask with the i-th bit set
/// @param i Bit position (0-31)
#define Mask32(i)          ((uint32_t)(1UL << (i)))

/// @brief Generate a 64-bit mask with the i-th bit set
/// @param i Bit position (0-63)
#define Mask64(i)          ((uint64_t)(1ULL << (i)))


/* --- INVERSE MASK GENERATION --- */

/// @brief Generate an 8-bit mask with the i-th bit CLEARED (all others set)
/// @param i Bit position (0-7)
#define InvMask8(i)        ((uint8_t)(~(1U << (i))))

/// @brief Generate a 16-bit mask with the i-th bit CLEARED (all others set)
/// @param i Bit position (0-15)
#define InvMask16(i)       ((uint16_t)(~(1U << (i))))

/// @brief Generate a 32-bit mask with the i-th bit CLEARED (all others set)
/// @param i Bit position (0-31)
#define InvMask32(i)       ((uint32_t)(~(1UL << (i))))

/// @brief Generate a 64-bit mask with the i-th bit CLEARED (all others set)
/// @param i Bit position (0-63)
#define InvMask64(i)       ((uint64_t)(~(1ULL << (i))))

// These macros are internal expansion helpers used to implement the variadic functionality.
// They apply function 'f' (e.g., Mask32) to each argument and bitwise-OR them together.

/// @cond INTERNAL
#define __mask_or1(f,a0)                                            (f(a0))
#define __mask_or2(f,a0,a1)                                         (f(a0) | f(a1))
#define __mask_or3(f,a0,a1,a2)                                      (f(a0) | f(a1) | f(a2))
#define __mask_or4(f,a0,a1,a2,a3)                                   (f(a0) | f(a1) | f(a2) | f(a3))
#define __mask_or5(f,a0,a1,a2,a3,a4)                                (f(a0) | f(a1) | f(a2) | f(a3) | f(a4))
#define __mask_or6(f,a0,a1,a2,a3,a4,a5)                             (f(a0) | f(a1) | f(a2) | f(a3) | f(a4) | f(a5))
#define __mask_or7(f,a0,a1,a2,a3,a4,a5,a6)                          (f(a0) | f(a1) | f(a2) | f(a3) | f(a4) | f(a5) | f(a6))
#define __mask_or8(f,a0,a1,a2,a3,a4,a5,a6,a7)                       (f(a0) | f(a1) | f(a2) | f(a3) | f(a4) | f(a5) | f(a6) | f(a7))
#define __mask_or9(f,a0,a1,a2,a3,a4,a5,a6,a7,a8)                    (f(a0) | f(a1) | f(a2) | f(a3) | f(a4) | f(a5) | f(a6) | f(a7) | f(a8))
#define __mask_or10(f,a0,a1,a2,a3,a4,a5,a6,a7,a8,a9)                (f(a0) | f(a1) | f(a2) | f(a3) | f(a4) | f(a5) | f(a6) | f(a7) | f(a8) | f(a9))
#define __mask_or11(f,a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10)            (f(a0) | f(a1) | f(a2) | f(a3) | f(a4) | f(a5) | f(a6) | f(a7) | f(a8) | f(a9) | f(a10))
#define __mask_or12(f,a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11)        (f(a0) | f(a1) | f(a2) | f(a3) | f(a4) | f(a5) | f(a6) | f(a7) | f(a8) | f(a9) | f(a10) | f(a11))
#define __mask_or13(f,a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12)    (f(a0) | f(a1) | f(a2) | f(a3) | f(a4) | f(a5) | f(a6) | f(a7) | f(a8) | f(a9) | f(a10) | f(a11) | f(a12))
#define __mask_or14(f,a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13)(f(a0) | f(a1) | f(a2) | f(a3) | f(a4) | f(a5) | f(a6) | f(a7) | f(a8) | f(a9) | f(a10) | f(a11) | f(a12) | f(a13))
#define __mask_or15(f,a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14) \
    (f(a0) | f(a1) | f(a2) | f(a3) | f(a4) | f(a5) | f(a6) | f(a7) | f(a8) | f(a9) | f(a10) | f(a11) | f(a12) | f(a13) | f(a14))
#define __mask_or16(f,a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15) \
    (f(a0) | f(a1) | f(a2) | f(a3) | f(a4) | f(a5) | f(a6) | f(a7) | f(a8) | f(a9) | f(a10) | f(a11) | f(a12) | f(a13) | f(a14) | f(a15))
#define __mask_or17(f,a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16) \
    (f(a0) | f(a1) | f(a2) | f(a3) | f(a4) | f(a5) | f(a6) | f(a7) | f(a8) | f(a9) | f(a10) | f(a11) | f(a12) | f(a13) | f(a14) | f(a15) | f(a16))
#define __mask_or18(f,a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16,a17) \
    (f(a0) | f(a1) | f(a2) | f(a3) | f(a4) | f(a5) | f(a6) | f(a7) | f(a8) | f(a9) | f(a10) | f(a11) | f(a12) | f(a13) | f(a14) | f(a15) | f(a16) | f(a17))
#define __mask_or19(f,a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16,a17,a18) \
    (f(a0) | f(a1) | f(a2) | f(a3) | f(a4) | f(a5) | f(a6) | f(a7) | f(a8) | f(a9) | f(a10) | f(a11) | f(a12) | f(a13) | f(a14) | f(a15) | f(a16) | f(a17) | f(a18))
#define __mask_or20(f,a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16,a17,a18,a19) \
    (f(a0) | f(a1) | f(a2) | f(a3) | f(a4) | f(a5) | f(a6) | f(a7) | f(a8) | f(a9) | f(a10) | f(a11) | f(a12) | f(a13) | f(a14) | f(a15) | f(a16) | f(a17) | f(a18) | f(a19))
#define __mask_or21(f,a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16,a17,a18,a19,a20) \
    (f(a0) | f(a1) | f(a2) | f(a3) | f(a4) | f(a5) | f(a6) | f(a7) | f(a8) | f(a9) | f(a10) | f(a11) | f(a12) | f(a13) | f(a14) | f(a15) | f(a16) | f(a17) | f(a18) | f(a19) | f(a20))
#define __mask_or22(f,a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16,a17,a18,a19,a20,a21) \
    (f(a0) | f(a1) | f(a2) | f(a3) | f(a4) | f(a5) | f(a6) | f(a7) | f(a8) | f(a9) | f(a10) | f(a11) | f(a12) | f(a13) | f(a14) | f(a15) | f(a16) | f(a17) | f(a18) | f(a19) | f(a20) | f(a21))
#define __mask_or23(f,a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16,a17,a18,a19,a20,a21,a22) \
    (f(a0) | f(a1) | f(a2) | f(a3) | f(a4) | f(a5) | f(a6) | f(a7) | f(a8) | f(a9) | f(a10) | f(a11) | f(a12) | f(a13) | f(a14) | f(a15) | f(a16) | f(a17) | f(a18) | f(a19) | f(a20) | f(a21) | f(a22))
#define __mask_or24(f,a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16,a17,a18,a19,a20,a21,a22,a23) \
    (f(a0) | f(a1) | f(a2) | f(a3) | f(a4) | f(a5) | f(a6) | f(a7) | f(a8) | f(a9) | f(a10) | f(a11) | f(a12) | f(a13) | f(a14) | f(a15) | f(a16) | f(a17) | f(a18) | f(a19) | f(a20) | f(a21) | f(a22) | f(a23))
#define __mask_or25(f,a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16,a17,a18,a19,a20,a21,a22,a23,a24) \
    (f(a0) | f(a1) | f(a2) | f(a3) | f(a4) | f(a5) | f(a6) | f(a7) | f(a8) | f(a9) | f(a10) | f(a11) | f(a12) | f(a13) | f(a14) | f(a15) | f(a16) | f(a17) | f(a18) | f(a19) | f(a20) | f(a21) | f(a22) | f(a23) | f(a24))
#define __mask_or26(f,a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16,a17,a18,a19,a20,a21,a22,a23,a24,a25) \
    (f(a0) | f(a1) | f(a2) | f(a3) | f(a4) | f(a5) | f(a6) | f(a7) | f(a8) | f(a9) | f(a10) | f(a11) | f(a12) | f(a13) | f(a14) | f(a15) | f(a16) | f(a17) | f(a18) | f(a19) | f(a20) | f(a21) | f(a22) | f(a23) | f(a24) | f(a25))
#define __mask_or27(f,a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16,a17,a18,a19,a20,a21,a22,a23,a24,a25,a26) \
    (f(a0) | f(a1) | f(a2) | f(a3) | f(a4) | f(a5) | f(a6) | f(a7) | f(a8) | f(a9) | f(a10) | f(a11) | f(a12) | f(a13) | f(a14) | f(a15) | f(a16) | f(a17) | f(a18) | f(a19) | f(a20) | f(a21) | f(a22) | f(a23) | f(a24) | f(a25) | f(a26))
#define __mask_or28(f,a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16,a17,a18,a19,a20,a21,a22,a23,a24,a25,a26,a27) \
    (f(a0) | f(a1) | f(a2) | f(a3) | f(a4) | f(a5) | f(a6) | f(a7) | f(a8) | f(a9) | f(a10) | f(a11) | f(a12) | f(a13) | f(a14) | f(a15) | f(a16) | f(a17) | f(a18) | f(a19) | f(a20) | f(a21) | f(a22) | f(a23) | f(a24) | f(a25) | f(a26) | f(a27))
#define __mask_or29(f,a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16,a17,a18,a19,a20,a21,a22,a23,a24,a25,a26,a27,a28) \
    (f(a0) | f(a1) | f(a2) | f(a3) | f(a4) | f(a5) | f(a6) | f(a7) | f(a8) | f(a9) | f(a10) | f(a11) | f(a12) | f(a13) | f(a14) | f(a15) | f(a16) | f(a17) | f(a18) | f(a19) | f(a20) | f(a21) | f(a22) | f(a23) | f(a24) | f(a25) | f(a26) | f(a27) | f(a28))
#define __mask_or30(f,a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16,a17,a18,a19,a20,a21,a22,a23,a24,a25,a26,a27,a28,a29) \
    (f(a0) | f(a1) | f(a2) | f(a3) | f(a4) | f(a5) | f(a6) | f(a7) | f(a8) | f(a9) | f(a10) | f(a11) | f(a12) | f(a13) | f(a14) | f(a15) | f(a16) | f(a17) | f(a18) | f(a19) | f(a20) | f(a21) | f(a22) | f(a23) | f(a24) | f(a25) | f(a26) | f(a27) | f(a28) | f(a29))
#define __mask_or31(f,a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16,a17,a18,a19,a20,a21,a22,a23,a24,a25,a26,a27,a28,a29,a30) \
    (f(a0) | f(a1) | f(a2) | f(a3) | f(a4) | f(a5) | f(a6) | f(a7) | f(a8) | f(a9) | f(a10) | f(a11) | f(a12) | f(a13) | f(a14) | f(a15) | f(a16) | f(a17) | f(a18) | f(a19) | f(a20) | f(a21) | f(a22) | f(a23) | f(a24) | f(a25) | f(a26) | f(a27) | f(a28) | f(a29) | f(a30))
#define __mask_or32(f,a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16,a17,a18,a19,a20,a21,a22,a23,a24,a25,a26,a27,a28,a29,a30,a31) \
    (f(a0) | f(a1) | f(a2) | f(a3) | f(a4) | f(a5) | f(a6) | f(a7) | f(a8) | f(a9) | f(a10) | f(a11) | f(a12) | f(a13) | f(a14) | f(a15) | f(a16) | f(a17) | f(a18) | f(a19) | f(a20) | f(a21) | f(a22) | f(a23) | f(a24) | f(a25) | f(a26) | f(a27) | f(a28) | f(a29) | f(a30) | f(a31))

/// @endcond

/// @brief Macro overload selector (Internal helper)
/// @details Selects the correct __mask_orX macro based on the number of arguments provided.
#define __mask_overload( \
    _1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16, \
    _17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32, NAME, ...) NAME

/// @brief Generic mask dispatcher (Internal helper)
/// @details Applies a mask function 'f' to a variable list of arguments.
#define __masks_generic(f, ...) \
    (__mask_overload(__VA_ARGS__, \
        __mask_or32, __mask_or31, __mask_or30, __mask_or29, \
        __mask_or28, __mask_or27, __mask_or26, __mask_or25, \
        __mask_or24, __mask_or23, __mask_or22, __mask_or21, \
        __mask_or20, __mask_or19, __mask_or18, __mask_or17, \
        __mask_or16, __mask_or15, __mask_or14, __mask_or13, \
        __mask_or12, __mask_or11, __mask_or10, __mask_or9,  \
        __mask_or8,  __mask_or7,  __mask_or6,  __mask_or5,  \
        __mask_or4,  __mask_or3,  __mask_or2,  __mask_or1)(f, __VA_ARGS__))


/* --- USER FACING VARIADIC MACROS --- */

/// @brief Create a composite 8-bit mask from multiple bit positions
/// @param ... Variable number of bit positions (1 to 32 arguments)
/// @return Combined mask (OR operation)
/// @example Masks8(0, 2, 7) expands to ((1<<0) | (1<<2) | (1<<7))
#define Masks8(...)     __masks_generic(Mask8,  __VA_ARGS__)

/// @brief Create a composite 16-bit mask from multiple bit positions
/// @param ... Variable number of bit positions (1 to 32 arguments)
/// @return Combined mask (OR operation)
#define Masks16(...)    __masks_generic(Mask16, __VA_ARGS__)

/// @brief Create a composite 32-bit mask from multiple bit positions
/// @param ... Variable number of bit positions (1 to 32 arguments)
/// @return Combined mask (OR operation)
#define Masks32(...)    __masks_generic(Mask32, __VA_ARGS__)

/// @brief Create a composite 64-bit mask from multiple bit positions
/// @param ... Variable number of bit positions (1 to 32 arguments)
/// @return Combined mask (OR operation)
#define Masks64(...)    __masks_generic(Mask64, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /// __BIT_OP_H__