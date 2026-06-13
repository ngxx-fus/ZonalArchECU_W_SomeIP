#ifndef CSTRING64_H
#define CSTRING64_H

#pragma once

#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include "ReturnType.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Represents a static string structure with a fixed 64-byte buffer.
 */
typedef struct {
    char        CString[64];
    uint32_t    Length;
} CString64_t;

/*
 * Initializes the CString64 object with an initial value.
 */
DefaultRet_t Str64Initialize(CString64_t* str, const char* initial_value);

/*
 * Formats and assigns data to the CString64 using variable arguments.
 */
DefaultRet_t Str64Print(CString64_t* str, const char* format, ...);

/*
 * Deep copies the content from the source CString64 to the destination.
 */
DefaultRet_t Str64Copy(CString64_t* dest, const CString64_t* src);

/*
 * Compares two CString64 objects. Returns standard strcmp result, or -1 if invalid.
 */
int Str64Compare(const CString64_t* str1, const CString64_t* str2);

/*
 * Finds the first occurrence of a substring within the CString64.
 * Returns the starting index, or -1 if not found.
 */
int Str64Find(const CString64_t* str, const char* substr);

/*
 * Appends a formatted string to the end of the current CString64.
 */
DefaultRet_t Str64Append(CString64_t* str, const char* format, ...);

/*
 * Checks if the CString64 is empty.
 */
bool Str64IsEmpty(const CString64_t* str);

/*
 * Clears the CString64 content (sets length to 0). No memory is freed.
 */
DefaultRet_t Str64Clear(CString64_t* str);

/*
 * Assigns a new string value to the CString64 directly.
 */
DefaultRet_t Str64Assign(CString64_t* str, const char* new_value);

/*
 * Resizes the CString64 to a new length, cropping any excess characters.
 */
DefaultRet_t Str64Resize(CString64_t* str, uint32_t new_length);

/*
 * Recalculates and updates the Length property based on current CString content.
 */
DefaultRet_t Str64SelfUpdateLength(CString64_t* str);

/*
 * Ensures the string is null-terminated within limits and corrects the Length.
 */
DefaultRet_t Str64SelfShrink(CString64_t* str);

/*
 * Converts the CString64 content into an array of uint8_t values.
 */
DefaultRet_t Str64ConvertToUint8Arr(const CString64_t* str, uint8_t* out_arr, uint32_t max_items);

/*
 * Converts the CString64 content into an array of uint16_t values.
 */
DefaultRet_t Str64ConvertToUint16Arr(const CString64_t* str, uint16_t* out_arr, uint32_t max_items);

/*
 * Converts the CString64 content into an array of uint32_t values.
 */
DefaultRet_t Str64ConvertToUint32Arr(const CString64_t* str, uint32_t* out_arr, uint32_t max_items);

/*
 * Converts the CString64 content into an array of uint64_t values.
 */
DefaultRet_t Str64ConvertToUint64Arr(const CString64_t* str, uint64_t* out_arr, uint32_t max_items);

/*
 * Converts the CString64 content into an array of int8_t values.
 */
DefaultRet_t Str64ConvertToSInt8Arr(const CString64_t* str, int8_t* out_arr, uint32_t max_items);

/*
 * Converts the CString64 content into an array of int16_t values.
 */
DefaultRet_t Str64ConvertToSInt16Arr(const CString64_t* str, int16_t* out_arr, uint32_t max_items);

/*
 * Converts the CString64 content into an array of int32_t values.
 */
DefaultRet_t Str64ConvertToSInt32Arr(const CString64_t* str, int32_t* out_arr, uint32_t max_items);

/*
 * Converts the CString64 content into an array of int64_t values.
 */
DefaultRet_t Str64ConvertToSInt64Arr(const CString64_t* str, int64_t* out_arr, uint32_t max_items);

/*
 * Initializes the CString64 from an array of uint8_t values.
 */
DefaultRet_t Str64InitializeFromUint8Arr(CString64_t* str, const uint8_t* arr, uint32_t arr_len);

/*
 * Initializes the CString64 from an array of uint16_t values.
 */
DefaultRet_t Str64InitializeFromUint16Arr(CString64_t* str, const uint16_t* arr, uint32_t arr_len);

/*
 * Initializes the CString64 from an array of uint32_t values.
 */
DefaultRet_t Str64InitializeFromUint32Arr(CString64_t* str, const uint32_t* arr, uint32_t arr_len);

/*
 * Initializes the CString64 from an array of uint64_t values.
 */
DefaultRet_t Str64InitializeFromUint64Arr(CString64_t* str, const uint64_t* arr, uint32_t arr_len);

/*
 * Initializes the CString64 from an array of int8_t values.
 */
DefaultRet_t Str64InitializeFromSInt8Arr(CString64_t* str, const int8_t* arr, uint32_t arr_len);

/*
 * Initializes the CString64 from an array of int16_t values.
 */
DefaultRet_t Str64InitializeFromSInt16Arr(CString64_t* str, const int16_t* arr, uint32_t arr_len);

/*
 * Initializes the CString64 from an array of int32_t values.
 */
DefaultRet_t Str64InitializeFromSInt32Arr(CString64_t* str, const int32_t* arr, uint32_t arr_len);

/*
 * Initializes the CString64 from an array of int64_t values.
 */
DefaultRet_t Str64InitializeFromSInt64Arr(CString64_t* str, const int64_t* arr, uint32_t arr_len);

#ifdef __cplusplus
}
#endif

#endif /* CSTRING64_H */