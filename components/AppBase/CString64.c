#include "CString64.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

DefaultRet_t Str64Initialize(CString64_t* str, const char* initial_value) {
    /* Control flow: ensure string object pointer is valid */
    if (str == NULL) {
        /* Return error on invalid pointer */
        return STAT_ERR_NULL;
    }

    /* Control flow: handle NULL initialization gracefully */
    if (initial_value == NULL) {
        str->CString[0] = '\0';
        str->Length = 0;
        /* Return success after clearing state */
        return STAT_OKE;
    }

    /* Control flow: copy safely within fixed bounds */
    strncpy(str->CString, initial_value, sizeof(str->CString) - 1);
    str->CString[sizeof(str->CString) - 1] = '\0';
    str->Length = (uint32_t)strlen(str->CString);

    /* Return success */
    return STAT_OKE;
}

DefaultRet_t Str64Print(CString64_t* str, const char* format, ...) {
    va_list args;
    int len;

    /* Control flow: validate input parameters */
    if ((str == NULL) || (format == NULL)) {
        /* Return error on invalid arguments */
        return STAT_ERR_NULL;
    }

    va_start(args, format);
    len = vsnprintf(str->CString, sizeof(str->CString), format, args);
    va_end(args);

    /* Control flow: verify vsnprintf did not fail */
    if (len < 0) {
        str->Length = 0;
        str->CString[0] = '\0';
        /* Return format error */
        return STAT_ERR_INVALID_ARG;
    }

    /* Control flow: check if output was truncated */
    if (len >= (int)sizeof(str->CString)) {
        str->Length = sizeof(str->CString) - 1;
        /* Return overflow error but retain truncated data */
        return STAT_ERR_OVERFLOW;
    }

    str->Length = (uint32_t)len;

    /* Return success */
    return STAT_OKE;
}

DefaultRet_t Str64Copy(CString64_t* dest, const CString64_t* src) {
    /* Control flow: validate pointers */
    if ((dest == NULL) || (src == NULL)) {
        /* Return error on invalid arguments */
        return STAT_ERR_NULL;
    }

    strncpy(dest->CString, src->CString, sizeof(dest->CString));
    dest->CString[sizeof(dest->CString) - 1] = '\0';
    dest->Length = src->Length;

    /* Return success */
    return STAT_OKE;
}

int Str64Compare(const CString64_t* str1, const CString64_t* str2) {
    /* Control flow: handle NULL inputs safely */
    if ((str1 == NULL) || (str2 == NULL)) {
        /* Return -1 as an error code for invalid comparison */
        return -1;
    }

    /* Return the standard strcmp result */
    return strncmp(str1->CString, str2->CString, sizeof(str1->CString));
}

int Str64Find(const CString64_t* str, const char* substr) {
    char* pos;

    /* Control flow: validate inputs */
    if ((str == NULL) || (substr == NULL)) {
        /* Return -1 indicating not found / error */
        return -1;
    }

    pos = strstr(str->CString, substr);

    /* Control flow: check if substring was found */
    if (pos != NULL) {
        /* Return the calculated index */
        return (int)(pos - str->CString);
    }

    /* Return -1 indicating substring was not found */
    return -1;
}

DefaultRet_t Str64Append(CString64_t* str, const char* format, ...) {
    va_list args;
    int append_len;
    uint32_t remaining_space;

    /* Control flow: validate target string object */
    if ((str == NULL) || (format == NULL)) {
        /* Return error on invalid arguments */
        return STAT_ERR_NULL;
    }

    remaining_space = sizeof(str->CString) - str->Length;

    /* Control flow: check if buffer is already full */
    if (remaining_space <= 1) {
        /* Return overflow */
        return STAT_ERR_OVERFLOW;
    }

    va_start(args, format);
    append_len = vsnprintf(str->CString + str->Length, remaining_space, format, args);
    va_end(args);

    /* Control flow: check for formatting error */
    if (append_len < 0) {
        /* Return invalid arg error */
        return STAT_ERR_INVALID_ARG;
    }

    /* Control flow: check if appended text was truncated */
    if (append_len >= (int)remaining_space) {
        str->Length = sizeof(str->CString) - 1;
        /* Return overflow warning */
        return STAT_ERR_OVERFLOW;
    }

    str->Length += (uint32_t)append_len;

    /* Return success */
    return STAT_OKE;
}

bool Str64IsEmpty(const CString64_t* str) {
    /* Control flow: evaluate length */
    if ((str == NULL) || (str->Length == 0)) {
        /* Return true if strictly empty */
        return true;
    }

    /* Return false if content exists */
    return false;
}

DefaultRet_t Str64Clear(CString64_t* str) {
    /* Control flow: validate object pointer */
    if (str == NULL) {
        /* Return error on null pointer */
        return STAT_ERR_NULL;
    }

    str->CString[0] = '\0';
    str->Length = 0;

    /* Return success */
    return STAT_OKE;
}

DefaultRet_t Str64Assign(CString64_t* str, const char* new_value) {
    /* Control flow: validate input pointer */
    if (str == NULL) {
        /* Return error on null pointer */
        return STAT_ERR_NULL;
    }

    /* Control flow: handle NULL string assignment */
    if (new_value == NULL) {
        /* Return status from Str64Clear */
        return Str64Clear(str);
    }

    strncpy(str->CString, new_value, sizeof(str->CString) - 1);
    str->CString[sizeof(str->CString) - 1] = '\0';
    str->Length = (uint32_t)strlen(str->CString);
    
    /* Return success */
    return STAT_OKE;
}

DefaultRet_t Str64Resize(CString64_t* str, uint32_t new_length) {
    /* Control flow: validate inputs */
    if (str == NULL) {
        /* Return null or state error */
        return STAT_ERR_NULL;
    }

    /* Control flow: crop if new length is strictly smaller */
    if (new_length < str->Length) {
        str->CString[new_length] = '\0';
        str->Length = new_length;
    }

    /* Return success */
    return STAT_OKE;
}

DefaultRet_t Str64SelfUpdateLength(CString64_t* str) {
    /* Control flow: validate input pointer */
    if (str == NULL) {
        /* Return error on null pointer */
        return STAT_ERR_NULL;
    }

    /* Control flow: ensure safe null-termination before reading length */
    str->CString[sizeof(str->CString) - 1] = '\0';
    str->Length = (uint32_t)strlen(str->CString);

    /* Return success */
    return STAT_OKE;
}

DefaultRet_t Str64SelfShrink(CString64_t* str) {
    /* Control flow: validate input pointer */
    if (str == NULL) {
        /* Return error on null pointer */
        return STAT_ERR_NULL;
    }

    /* Force null-termination at the absolute boundary to cut off any buffer overflow */
    str->CString[sizeof(str->CString) - 1] = '\0';
    
    /* Update the length to match the newly shrinked string */
    str->Length = (uint32_t)strlen(str->CString);

    /* Return success */
    return STAT_OKE;
}

/* --- Array to CString Initializers --- */
/* Optimized for static buffer: directly formatting without dynamic allocation */

#define STR64_INIT_FROM_ARR(type, format_specifier, cast_type) \
    uint32_t offset = 0; \
    uint32_t i; \
    int written; \
    /* Control flow: validate inputs */ \
    if ((str == NULL) || (arr == NULL) || (arr_len == 0)) { \
        return STAT_ERR_NULL; \
    } \
    str->CString[0] = '\0'; \
    str->Length = 0; \
    /* Control flow: iterate over array items */ \
    for (i = 0; i < arr_len; i++) { \
        uint32_t remaining = sizeof(str->CString) - offset; \
        /* Control flow: ensure space remains */ \
        if (remaining <= 1) { \
            return STAT_ERR_OVERFLOW; \
        } \
        /* Control flow: format string depending on position */ \
        if (i == (arr_len - 1)) { \
            written = snprintf(str->CString + offset, remaining, format_specifier, (cast_type)arr[i]); \
        } else { \
            written = snprintf(str->CString + offset, remaining, format_specifier ", ", (cast_type)arr[i]); \
        } \
        /* Control flow: check truncation */ \
        if ((written < 0) || (written >= (int)remaining)) { \
            str->Length = sizeof(str->CString) - 1; \
            return STAT_ERR_OVERFLOW; \
        } \
        offset += (uint32_t)written; \
        str->Length = offset; \
    } \
    return STAT_OKE

DefaultRet_t Str64InitializeFromUint8Arr(CString64_t* str, const uint8_t* arr, uint32_t arr_len) {
    STR64_INIT_FROM_ARR(uint8_t, "%llu", unsigned long long);
}

DefaultRet_t Str64InitializeFromUint16Arr(CString64_t* str, const uint16_t* arr, uint32_t arr_len) {
    STR64_INIT_FROM_ARR(uint16_t, "%llu", unsigned long long);
}

DefaultRet_t Str64InitializeFromUint32Arr(CString64_t* str, const uint32_t* arr, uint32_t arr_len) {
    STR64_INIT_FROM_ARR(uint32_t, "%llu", unsigned long long);
}

DefaultRet_t Str64InitializeFromUint64Arr(CString64_t* str, const uint64_t* arr, uint32_t arr_len) {
    STR64_INIT_FROM_ARR(uint64_t, "%llu", unsigned long long);
}

DefaultRet_t Str64InitializeFromSInt8Arr(CString64_t* str, const int8_t* arr, uint32_t arr_len) {
    STR64_INIT_FROM_ARR(int8_t, "%lld", long long);
}

DefaultRet_t Str64InitializeFromSInt16Arr(CString64_t* str, const int16_t* arr, uint32_t arr_len) {
    STR64_INIT_FROM_ARR(int16_t, "%lld", long long);
}

DefaultRet_t Str64InitializeFromSInt32Arr(CString64_t* str, const int32_t* arr, uint32_t arr_len) {
    STR64_INIT_FROM_ARR(int32_t, "%lld", long long);
}

DefaultRet_t Str64InitializeFromSInt64Arr(CString64_t* str, const int64_t* arr, uint32_t arr_len) {
    STR64_INIT_FROM_ARR(int64_t, "%lld", long long);
}

/* --- CString to Array Parsers --- */
/* Utilizing strtoull and strtoll on the static buffer */

#define STR64_CONV_TO_ARR(cast_type, conv_func) \
    char* end_ptr; \
    char* curr_ptr; \
    uint32_t count = 0; \
    /* Control flow: validate inputs */ \
    if ((str == NULL) || (out_arr == NULL) || (max_items == 0)) { \
        return STAT_ERR_NULL; \
    } \
    curr_ptr = (char*)str->CString; \
    /* Control flow: parse while string has characters and limits are not hit */ \
    while ((*curr_ptr != '\0') && (count < max_items)) { \
        auto val = conv_func(curr_ptr, &end_ptr, 0); \
        /* Control flow: check if a valid conversion was made */ \
        if (curr_ptr != end_ptr) { \
            out_arr[count] = (cast_type)val; \
            count++; \
            curr_ptr = end_ptr; \
        } else { \
            curr_ptr++; \
        } \
    } \
    return (DefaultRet_t)count

DefaultRet_t Str64ConvertToUint8Arr(const CString64_t* str, uint8_t* out_arr, uint32_t max_items) {
    char* end_ptr;
    char* curr_ptr;
    uint32_t count = 0;

    /* Control flow: validate inputs */
    if ((str == NULL) || (out_arr == NULL) || (max_items == 0)) {
        return STAT_ERR_NULL;
    }

    curr_ptr = (char*)str->CString;

    /* Control flow: parse while string has characters and limits are not hit */
    while ((*curr_ptr != '\0') && (count < max_items)) {
        unsigned long long val = strtoull(curr_ptr, &end_ptr, 0);

        /* Control flow: check if a valid conversion was made */
        if (curr_ptr != end_ptr) {
            out_arr[count] = (uint8_t)val;
            count++;
            curr_ptr = end_ptr;
        } else {
            curr_ptr++;
        }
    }

    /* Return total parsed elements casted to status code type */
    return (DefaultRet_t)count;
}

DefaultRet_t Str64ConvertToUint16Arr(const CString64_t* str, uint16_t* out_arr, uint32_t max_items) {
    char* end_ptr;
    char* curr_ptr;
    uint32_t count = 0;

    /* Control flow: validate inputs */
    if ((str == NULL) || (out_arr == NULL) || (max_items == 0)) {
        return STAT_ERR_NULL;
    }

    curr_ptr = (char*)str->CString;

    /* Control flow: parse while string has characters and limits are not hit */
    while ((*curr_ptr != '\0') && (count < max_items)) {
        unsigned long long val = strtoull(curr_ptr, &end_ptr, 0);

        /* Control flow: check if a valid conversion was made */
        if (curr_ptr != end_ptr) {
            out_arr[count] = (uint16_t)val;
            count++;
            curr_ptr = end_ptr;
        } else {
            curr_ptr++;
        }
    }

    /* Return total parsed elements casted to status code type */
    return (DefaultRet_t)count;
}

DefaultRet_t Str64ConvertToUint32Arr(const CString64_t* str, uint32_t* out_arr, uint32_t max_items) {
    char* end_ptr;
    char* curr_ptr;
    uint32_t count = 0;

    /* Control flow: validate inputs */
    if ((str == NULL) || (out_arr == NULL) || (max_items == 0)) {
        return STAT_ERR_NULL;
    }

    curr_ptr = (char*)str->CString;

    /* Control flow: parse while string has characters and limits are not hit */
    while ((*curr_ptr != '\0') && (count < max_items)) {
        unsigned long long val = strtoull(curr_ptr, &end_ptr, 0);

        /* Control flow: check if a valid conversion was made */
        if (curr_ptr != end_ptr) {
            out_arr[count] = (uint32_t)val;
            count++;
            curr_ptr = end_ptr;
        } else {
            curr_ptr++;
        }
    }

    /* Return total parsed elements casted to status code type */
    return (DefaultRet_t)count;
}

DefaultRet_t Str64ConvertToUint64Arr(const CString64_t* str, uint64_t* out_arr, uint32_t max_items) {
    char* end_ptr;
    char* curr_ptr;
    uint32_t count = 0;

    /* Control flow: validate inputs */
    if ((str == NULL) || (out_arr == NULL) || (max_items == 0)) {
        return STAT_ERR_NULL;
    }

    curr_ptr = (char*)str->CString;

    /* Control flow: parse while string has characters and limits are not hit */
    while ((*curr_ptr != '\0') && (count < max_items)) {
        unsigned long long val = strtoull(curr_ptr, &end_ptr, 0);

        /* Control flow: check if a valid conversion was made */
        if (curr_ptr != end_ptr) {
            out_arr[count] = (uint64_t)val;
            count++;
            curr_ptr = end_ptr;
        } else {
            curr_ptr++;
        }
    }

    /* Return total parsed elements casted to status code type */
    return (DefaultRet_t)count;
}

DefaultRet_t Str64ConvertToSInt8Arr(const CString64_t* str, int8_t* out_arr, uint32_t max_items) {
    char* end_ptr;
    char* curr_ptr;
    uint32_t count = 0;

    /* Control flow: validate inputs */
    if ((str == NULL) || (out_arr == NULL) || (max_items == 0)) {
        return STAT_ERR_NULL;
    }

    curr_ptr = (char*)str->CString;

    /* Control flow: parse while string has characters and limits are not hit */
    while ((*curr_ptr != '\0') && (count < max_items)) {
        long long val = strtoll(curr_ptr, &end_ptr, 0);

        /* Control flow: check if a valid conversion was made */
        if (curr_ptr != end_ptr) {
            out_arr[count] = (int8_t)val;
            count++;
            curr_ptr = end_ptr;
        } else {
            curr_ptr++;
        }
    }

    /* Return total parsed elements casted to status code type */
    return (DefaultRet_t)count;
}

DefaultRet_t Str64ConvertToSInt16Arr(const CString64_t* str, int16_t* out_arr, uint32_t max_items) {
    char* end_ptr;
    char* curr_ptr;
    uint32_t count = 0;

    /* Control flow: validate inputs */
    if ((str == NULL) || (out_arr == NULL) || (max_items == 0)) {
        return STAT_ERR_NULL;
    }

    curr_ptr = (char*)str->CString;

    /* Control flow: parse while string has characters and limits are not hit */
    while ((*curr_ptr != '\0') && (count < max_items)) {
        long long val = strtoll(curr_ptr, &end_ptr, 0);

        /* Control flow: check if a valid conversion was made */
        if (curr_ptr != end_ptr) {
            out_arr[count] = (int16_t)val;
            count++;
            curr_ptr = end_ptr;
        } else {
            curr_ptr++;
        }
    }

    /* Return total parsed elements casted to status code type */
    return (DefaultRet_t)count;
}

DefaultRet_t Str64ConvertToSInt32Arr(const CString64_t* str, int32_t* out_arr, uint32_t max_items) {
    char* end_ptr;
    char* curr_ptr;
    uint32_t count = 0;

    /* Control flow: validate inputs */
    if ((str == NULL) || (out_arr == NULL) || (max_items == 0)) {
        return STAT_ERR_NULL;
    }

    curr_ptr = (char*)str->CString;

    /* Control flow: parse while string has characters and limits are not hit */
    while ((*curr_ptr != '\0') && (count < max_items)) {
        long long val = strtoll(curr_ptr, &end_ptr, 0);

        /* Control flow: check if a valid conversion was made */
        if (curr_ptr != end_ptr) {
            out_arr[count] = (int32_t)val;
            count++;
            curr_ptr = end_ptr;
        } else {
            curr_ptr++;
        }
    }

    /* Return total parsed elements casted to status code type */
    return (DefaultRet_t)count;
}

DefaultRet_t Str64ConvertToSInt64Arr(const CString64_t* str, int64_t* out_arr, uint32_t max_items) {
    char* end_ptr;
    char* curr_ptr;
    uint32_t count = 0;

    /* Control flow: validate inputs */
    if ((str == NULL) || (out_arr == NULL) || (max_items == 0)) {
        return STAT_ERR_NULL;
    }

    curr_ptr = (char*)str->CString;

    /* Control flow: parse while string has characters and limits are not hit */
    while ((*curr_ptr != '\0') && (count < max_items)) {
        long long val = strtoll(curr_ptr, &end_ptr, 0);

        /* Control flow: check if a valid conversion was made */
        if (curr_ptr != end_ptr) {
            out_arr[count] = (int64_t)val;
            count++;
            curr_ptr = end_ptr;
        } else {
            curr_ptr++;
        }
    }

    /* Return total parsed elements casted to status code type */
    return (DefaultRet_t)count;
}