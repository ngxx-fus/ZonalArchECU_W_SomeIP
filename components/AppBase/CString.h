#ifndef CSTRING_H
#define CSTRING_H

#pragma once

#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include "ReturnType.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Represents a dynamic string structure.
 */
typedef struct {
    char*       CString;
    uint32_t    Length;
} CString_t;

/*
 * Allocates a new CString_t object dynamically and initializes it.
 * Returns the pointer to the newly allocated object, or NULL on failure.
 */
CString_t* CString(const char* initial_value);

/*
 * Initializes the CString object with an initial value.
 */
DefaultRet_t StrInitialize(CString_t* str, const char* initial_value);

/*
 * Frees the allocated memory of the CString and resets properties.
 */
DefaultRet_t StrFree(CString_t* str);

/*
 * Formats and assigns data to the CString using variable arguments.
 */
DefaultRet_t StrPrint(CString_t* str, const char* format, ...);

/*
 * Deep copies the content from the source CString to the destination.
 */
DefaultRet_t StrCopy(CString_t* dest, const CString_t* src);

/*
 * Compares two CString objects. Returns standard strcmp result, or -1 if invalid.
 */
int StrCompare(const CString_t* str1, const CString_t* str2);

/*
 * Finds the first occurrence of a substring within the CString.
 * Returns the starting index, or -1 if not found.
 */
int StrFind(const CString_t* str, const char* substr);

/*
 * Appends a formatted string to the end of the current CString.
 */
DefaultRet_t StrAppend(CString_t* str, const char* format, ...);

/*
 * Checks if the CString is empty.
 */
bool StrIsEmpty(const CString_t* str);

/*
 * Clears the CString content (sets length to 0) without freeing memory.
 */
DefaultRet_t StrClear(CString_t* str);

/*
 * Assigns a new string value to the CString directly.
 */
DefaultRet_t StrAssign(CString_t* str, const char* new_value);

/*
 * Resizes the CString to a new length, cropping any excess characters.
 */
DefaultRet_t StrResize(CString_t* str, uint32_t new_length);

/*
 * Converts the CString content into an array of uint8_t values.
 * Returns the number of items successfully parsed, or STAT_ERR_NULL on error.
 */
DefaultRet_t StrConvertToUint8Arr(const CString_t* str, uint8_t* out_arr, uint32_t max_items);

/*
 * Converts the CString content into an array of uint16_t values.
 * Returns the number of items successfully parsed, or STAT_ERR_NULL on error.
 */
DefaultRet_t StrConvertToUint16Arr(const CString_t* str, uint16_t* out_arr, uint32_t max_items);

/*
 * Converts the CString content into an array of uint32_t values.
 * Returns the number of items successfully parsed, or STAT_ERR_NULL on error.
 */
DefaultRet_t StrConvertToUint32Arr(const CString_t* str, uint32_t* out_arr, uint32_t max_items);

/*
 * Converts the CString content into an array of uint64_t values.
 * Returns the number of items successfully parsed, or STAT_ERR_NULL on error.
 */
DefaultRet_t StrConvertToUint64Arr(const CString_t* str, uint64_t* out_arr, uint32_t max_items);

/*
 * Converts the CString content into an array of int8_t values.
 * Returns the number of items successfully parsed, or STAT_ERR_NULL on error.
 */
DefaultRet_t StrConvertToSInt8Arr(const CString_t* str, int8_t* out_arr, uint32_t max_items);

/*
 * Converts the CString content into an array of int16_t values.
 * Returns the number of items successfully parsed, or STAT_ERR_NULL on error.
 */
DefaultRet_t StrConvertToSInt16Arr(const CString_t* str, int16_t* out_arr, uint32_t max_items);

/*
 * Converts the CString content into an array of int32_t values.
 * Returns the number of items successfully parsed, or STAT_ERR_NULL on error.
 */
DefaultRet_t StrConvertToSInt32Arr(const CString_t* str, int32_t* out_arr, uint32_t max_items);

/*
 * Converts the CString content into an array of int64_t values.
 * Returns the number of items successfully parsed, or STAT_ERR_NULL on error.
 */
DefaultRet_t StrConvertToSInt64Arr(const CString_t* str, int64_t* out_arr, uint32_t max_items);

/*
 * Initializes the CString from an array of uint8_t values.
 */
DefaultRet_t StrInitializeFromUint8Arr(CString_t* str, const uint8_t* arr, uint32_t arr_len);

/*
 * Initializes the CString from an array of uint16_t values.
 */
DefaultRet_t StrInitializeFromUint16Arr(CString_t* str, const uint16_t* arr, uint32_t arr_len);

/*
 * Initializes the CString from an array of uint32_t values.
 */
DefaultRet_t StrInitializeFromUint32Arr(CString_t* str, const uint32_t* arr, uint32_t arr_len);

/*
 * Initializes the CString from an array of uint64_t values.
 */
DefaultRet_t StrInitializeFromUint64Arr(CString_t* str, const uint64_t* arr, uint32_t arr_len);

/*
 * Initializes the CString from an array of int8_t values.
 */
DefaultRet_t StrInitializeFromSInt8Arr(CString_t* str, const int8_t* arr, uint32_t arr_len);

/*
 * Initializes the CString from an array of int16_t values.
 */
DefaultRet_t StrInitializeFromSInt16Arr(CString_t* str, const int16_t* arr, uint32_t arr_len);

/*
 * Initializes the CString from an array of int32_t values.
 */
DefaultRet_t StrInitializeFromSInt32Arr(CString_t* str, const int32_t* arr, uint32_t arr_len);

/*
 * Initializes the CString from an array of int64_t values.
 */
DefaultRet_t StrInitializeFromSInt64Arr(CString_t* str, const int64_t* arr, uint32_t arr_len);

#ifdef __cplusplus
}
#endif

#endif /* CSTRING_H */