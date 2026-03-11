#ifndef __UTILS_ALL_H__
#define __UTILS_ALL_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef PRINT_HEADER_COMPILE_MESSAGE
#pragma message ("AppUtils/All.h")
#endif

#include <stdio.h>              /// for printf-like formatting
#include <stdarg.h>             /// for variadic macros
#include <stdint.h>             /// fixed-width integer types (uint8_t, int32_t, etc.)
#include <stdlib.h>             /// malloc...
#include <stdbool.h>            /// boolean type (true/false)
#include <stdarg.h>             /// variable arguments (va_list, va_start, va_end)
#include <string.h>             /// memset, strlen
#include <math.h>

#include "./BitOp.h"
#include "./Arithmetic.h"
#include "./ReturnType.h"
#include "./FlagControl.h"
#include "./Loop.h"

#ifdef __cplusplus
}
#endif

#endif /// __UTILS_ALL_H__