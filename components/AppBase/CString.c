#include "CString.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

CString_t* CString(const char* initial_value) {
    CString_t* new_obj;

    new_obj = (CString_t*)malloc(sizeof(CString_t));

    /* Control flow: verify object allocation success */
    if (new_obj == NULL) {
        /* Return NULL on memory error */
        return NULL;
    }

    /* Control flow: attempt string initialization */
    if (StrInitialize(new_obj, initial_value) != STAT_OKE) {
        free(new_obj);
        /* Return NULL if internal initialization fails */
        return NULL;
    }

    /* Return fully allocated and initialized object */
    return new_obj;
}

DefaultRet_t StrInitialize(CString_t* str, const char* initial_value) {
    /* Control flow: ensure string object pointer is valid */
    if (str == NULL) {
        /* Return error on invalid pointer */
        return STAT_ERR_NULL;
    }

    /* Control flow: handle NULL initialization gracefully */
    if (initial_value == NULL) {
        str->CString = NULL;
        str->Length = 0;
        /* Return success after clearing state */
        return STAT_OKE;
    }

    str->Length = (uint32_t)strlen(initial_value);
    str->CString = (char*)malloc(str->Length + 1);

    /* Control flow: check if memory allocation succeeded */
    if (str->CString == NULL) {
        str->Length = 0;
        /* Return memory error */
        return STAT_ERR_MALLOC_FAILED;
    }

    strcpy(str->CString, initial_value);
    
    /* Return success */
    return STAT_OKE;
}

DefaultRet_t StrFree(CString_t* str) {
    /* Control flow: ensure string object pointer is valid */
    if (str == NULL) {
        /* Return error on invalid pointer */
        return STAT_ERR_NULL;
    }

    /* Control flow: free allocated memory if exists */
    if (str->CString != NULL) {
        free(str->CString);
        str->CString = NULL;
        str->Length = 0;
    }

    /* Return success */
    return STAT_OKE;
}

DefaultRet_t StrPrint(CString_t* str, const char* format, ...) {
    va_list args;
    int len;

    /* Control flow: validate input parameters */
    if ((str == NULL) || (format == NULL)) {
        /* Return error on invalid arguments */
        return STAT_ERR_NULL;
    }

    va_start(args, format);
    len = vsnprintf(NULL, 0, format, args);
    va_end(args);

    /* Control flow: verify vsnprintf did not fail */
    if (len < 0) {
        /* Return format error */
        return STAT_ERR_INVALID_ARG;
    }

    /* Control flow: free existing memory before reallocating */
    if (str->CString != NULL) {
        free(str->CString);
    }

    str->Length = (uint32_t)len;
    str->CString = (char*)malloc(str->Length + 1);

    /* Control flow: verify memory allocation */
    if (str->CString == NULL) {
        str->Length = 0;
        /* Return memory error */
        return STAT_ERR_MALLOC_FAILED;
    }

    va_start(args, format);
    vsnprintf(str->CString, str->Length + 1, format, args);
    va_end(args);

    /* Return success */
    return STAT_OKE;
}

DefaultRet_t StrCopy(CString_t* dest, const CString_t* src) {
    /* Control flow: validate source and destination pointers */
    if ((dest == NULL) || (src == NULL)) {
        /* Return error on invalid arguments */
        return STAT_ERR_NULL;
    }

    /* Control flow: free old destination memory */
    if (dest->CString != NULL) {
        free(dest->CString);
    }

    dest->Length = src->Length;
    
    /* Control flow: check if source is empty */
    if (src->CString == NULL) {
        dest->CString = NULL;
        /* Return success after clearing destination */
        return STAT_OKE;
    }

    dest->CString = (char*)malloc(dest->Length + 1);

    /* Control flow: check allocation success */
    if (dest->CString == NULL) {
        dest->Length = 0;
        /* Return memory error */
        return STAT_ERR_MALLOC_FAILED;
    }

    strcpy(dest->CString, src->CString);
    
    /* Return success */
    return STAT_OKE;
}

int StrCompare(const CString_t* str1, const CString_t* str2) {
    /* Control flow: handle NULL inputs safely */
    if ((str1 == NULL) || (str2 == NULL) || (str1->CString == NULL) || (str2->CString == NULL)) {
        /* Return -1 as an error code for invalid comparison */
        return -1;
    }

    /* Return the standard strcmp result */
    return strcmp(str1->CString, str2->CString);
}

int StrFind(const CString_t* str, const char* substr) {
    char* pos;

    /* Control flow: validate inputs */
    if ((str == NULL) || (str->CString == NULL) || (substr == NULL)) {
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

DefaultRet_t StrAppend(CString_t* str, const char* format, ...) {
    va_list args;
    int append_len;
    uint32_t new_length;
    char* new_str;

    /* Control flow: validate target string object */
    if ((str == NULL) || (format == NULL)) {
        /* Return error on invalid arguments */
        return STAT_ERR_NULL;
    }

    va_start(args, format);
    append_len = vsnprintf(NULL, 0, format, args);
    va_end(args);

    /* Control flow: check for formatting error */
    if (append_len < 0) {
        /* Return invalid arg error */
        return STAT_ERR_INVALID_ARG;
    }

    new_length = str->Length + (uint32_t)append_len;
    new_str = (char*)malloc(new_length + 1);

    /* Control flow: check memory allocation */
    if (new_str == NULL) {
        /* Return memory error */
        return STAT_ERR_MALLOC_FAILED;
    }

    /* Control flow: preserve old content if it exists */
    if (str->CString != NULL) {
        strcpy(new_str, str->CString);
        free(str->CString);
    } else {
        new_str[0] = '\0';
    }

    va_start(args, format);
    vsnprintf(new_str + str->Length, append_len + 1, format, args);
    va_end(args);

    str->CString = new_str;
    str->Length = new_length;

    /* Return success */
    return STAT_OKE;
}

bool StrIsEmpty(const CString_t* str) {
    /* Control flow: evaluate length and content */
    if ((str == NULL) || (str->Length == 0) || (str->CString == NULL)) {
        /* Return true if strictly empty */
        return true;
    }

    /* Return false if content exists */
    return false;
}

DefaultRet_t StrClear(CString_t* str) {
    /* Control flow: validate object pointer */
    if (str == NULL) {
        /* Return error on null pointer */
        return STAT_ERR_NULL;
    }

    /* Control flow: clear content if allocated */
    if (str->CString != NULL) {
        str->CString[0] = '\0';
        str->Length = 0;
    }

    /* Return success */
    return STAT_OKE;
}

DefaultRet_t StrAssign(CString_t* str, const char* new_value) {
    uint32_t new_len;

    /* Control flow: validate input pointer */
    if (str == NULL) {
        /* Return error on null pointer */
        return STAT_ERR_NULL;
    }

    /* Control flow: handle NULL string assignment */
    if (new_value == NULL) {
        /* Return status from StrClear */
        return StrClear(str);
    }

    new_len = (uint32_t)strlen(new_value);

    /* Control flow: free existing memory if present */
    if (str->CString != NULL) {
        free(str->CString);
    }

    str->Length = new_len;
    str->CString = (char*)malloc(str->Length + 1);

    /* Control flow: check allocation success */
    if (str->CString == NULL) {
        str->Length = 0;
        /* Return memory error */
        return STAT_ERR_MALLOC_FAILED;
    }

    strcpy(str->CString, new_value);
    
    /* Return success */
    return STAT_OKE;
}

DefaultRet_t StrResize(CString_t* str, uint32_t new_length) {
    /* Control flow: validate inputs */
    if ((str == NULL) || (str->CString == NULL)) {
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

/* --- Array to CString Initializers --- */

DefaultRet_t StrInitializeFromUint8Arr(CString_t* str, const uint8_t* arr, uint32_t arr_len) {
    uint32_t buf_size = arr_len * 6 + 1;
    uint32_t offset = 0;
    char* temp_buf;
    uint32_t i;
    DefaultRet_t ret_stat;

    /* Control flow: validate inputs */
    if ((str == NULL) || (arr == NULL) || (arr_len == 0)) {
        /* Return error on invalid arguments */
        return STAT_ERR_NULL;
    }

    temp_buf = (char*)malloc(buf_size);

    /* Control flow: verify allocation */
    if (temp_buf == NULL) {
        /* Return memory error */
        return STAT_ERR_MALLOC_FAILED;
    }

    /* Control flow: iterate over array items */
    for (i = 0; i < arr_len; i++) {
        /* Control flow: exclude comma for the final element */
        if (i == (arr_len - 1)) {
            offset += (uint32_t)snprintf(temp_buf + offset, buf_size - offset, "%llu", (unsigned long long)arr[i]);
        } else {
            offset += (uint32_t)snprintf(temp_buf + offset, buf_size - offset, "%llu, ", (unsigned long long)arr[i]);
        }
    }

    ret_stat = StrInitialize(str, temp_buf);
    free(temp_buf);

    /* Return the status passed down from StrInitialize */
    return ret_stat;
}

DefaultRet_t StrInitializeFromUint16Arr(CString_t* str, const uint16_t* arr, uint32_t arr_len) {
    uint32_t buf_size = arr_len * 8 + 1;
    uint32_t offset = 0;
    char* temp_buf;
    uint32_t i;
    DefaultRet_t ret_stat;

    /* Control flow: validate inputs */
    if ((str == NULL) || (arr == NULL) || (arr_len == 0)) {
        /* Return error on invalid arguments */
        return STAT_ERR_NULL;
    }

    temp_buf = (char*)malloc(buf_size);

    /* Control flow: verify allocation */
    if (temp_buf == NULL) {
        /* Return memory error */
        return STAT_ERR_MALLOC_FAILED;
    }

    /* Control flow: iterate over array items */
    for (i = 0; i < arr_len; i++) {
        /* Control flow: exclude comma for the final element */
        if (i == (arr_len - 1)) {
            offset += (uint32_t)snprintf(temp_buf + offset, buf_size - offset, "%llu", (unsigned long long)arr[i]);
        } else {
            offset += (uint32_t)snprintf(temp_buf + offset, buf_size - offset, "%llu, ", (unsigned long long)arr[i]);
        }
    }

    ret_stat = StrInitialize(str, temp_buf);
    free(temp_buf);

    /* Return the status passed down from StrInitialize */
    return ret_stat;
}

DefaultRet_t StrInitializeFromUint32Arr(CString_t* str, const uint32_t* arr, uint32_t arr_len) {
    uint32_t buf_size = arr_len * 13 + 1;
    uint32_t offset = 0;
    char* temp_buf;
    uint32_t i;
    DefaultRet_t ret_stat;

    /* Control flow: validate inputs */
    if ((str == NULL) || (arr == NULL) || (arr_len == 0)) {
        /* Return error on invalid arguments */
        return STAT_ERR_NULL;
    }

    temp_buf = (char*)malloc(buf_size);

    /* Control flow: verify allocation */
    if (temp_buf == NULL) {
        /* Return memory error */
        return STAT_ERR_MALLOC_FAILED;
    }

    /* Control flow: iterate over array items */
    for (i = 0; i < arr_len; i++) {
        /* Control flow: exclude comma for the final element */
        if (i == (arr_len - 1)) {
            offset += (uint32_t)snprintf(temp_buf + offset, buf_size - offset, "%llu", (unsigned long long)arr[i]);
        } else {
            offset += (uint32_t)snprintf(temp_buf + offset, buf_size - offset, "%llu, ", (unsigned long long)arr[i]);
        }
    }

    ret_stat = StrInitialize(str, temp_buf);
    free(temp_buf);

    /* Return the status passed down from StrInitialize */
    return ret_stat;
}

DefaultRet_t StrInitializeFromUint64Arr(CString_t* str, const uint64_t* arr, uint32_t arr_len) {
    uint32_t buf_size = arr_len * 23 + 1;
    uint32_t offset = 0;
    char* temp_buf;
    uint32_t i;
    DefaultRet_t ret_stat;

    /* Control flow: validate inputs */
    if ((str == NULL) || (arr == NULL) || (arr_len == 0)) {
        /* Return error on invalid arguments */
        return STAT_ERR_NULL;
    }

    temp_buf = (char*)malloc(buf_size);

    /* Control flow: verify allocation */
    if (temp_buf == NULL) {
        /* Return memory error */
        return STAT_ERR_MALLOC_FAILED;
    }

    /* Control flow: iterate over array items */
    for (i = 0; i < arr_len; i++) {
        /* Control flow: exclude comma for the final element */
        if (i == (arr_len - 1)) {
            offset += (uint32_t)snprintf(temp_buf + offset, buf_size - offset, "%llu", (unsigned long long)arr[i]);
        } else {
            offset += (uint32_t)snprintf(temp_buf + offset, buf_size - offset, "%llu, ", (unsigned long long)arr[i]);
        }
    }

    ret_stat = StrInitialize(str, temp_buf);
    free(temp_buf);

    /* Return the status passed down from StrInitialize */
    return ret_stat;
}

DefaultRet_t StrInitializeFromSInt8Arr(CString_t* str, const int8_t* arr, uint32_t arr_len) {
    uint32_t buf_size = arr_len * 7 + 1;
    uint32_t offset = 0;
    char* temp_buf;
    uint32_t i;
    DefaultRet_t ret_stat;

    /* Control flow: validate inputs */
    if ((str == NULL) || (arr == NULL) || (arr_len == 0)) {
        /* Return error on invalid arguments */
        return STAT_ERR_NULL;
    }

    temp_buf = (char*)malloc(buf_size);

    /* Control flow: verify allocation */
    if (temp_buf == NULL) {
        /* Return memory error */
        return STAT_ERR_MALLOC_FAILED;
    }

    /* Control flow: iterate over array items */
    for (i = 0; i < arr_len; i++) {
        /* Control flow: exclude comma for the final element */
        if (i == (arr_len - 1)) {
            offset += (uint32_t)snprintf(temp_buf + offset, buf_size - offset, "%lld", (long long)arr[i]);
        } else {
            offset += (uint32_t)snprintf(temp_buf + offset, buf_size - offset, "%lld, ", (long long)arr[i]);
        }
    }

    ret_stat = StrInitialize(str, temp_buf);
    free(temp_buf);

    /* Return the status passed down from StrInitialize */
    return ret_stat;
}

DefaultRet_t StrInitializeFromSInt16Arr(CString_t* str, const int16_t* arr, uint32_t arr_len) {
    uint32_t buf_size = arr_len * 9 + 1;
    uint32_t offset = 0;
    char* temp_buf;
    uint32_t i;
    DefaultRet_t ret_stat;

    /* Control flow: validate inputs */
    if ((str == NULL) || (arr == NULL) || (arr_len == 0)) {
        /* Return error on invalid arguments */
        return STAT_ERR_NULL;
    }

    temp_buf = (char*)malloc(buf_size);

    /* Control flow: verify allocation */
    if (temp_buf == NULL) {
        /* Return memory error */
        return STAT_ERR_MALLOC_FAILED;
    }

    /* Control flow: iterate over array items */
    for (i = 0; i < arr_len; i++) {
        /* Control flow: exclude comma for the final element */
        if (i == (arr_len - 1)) {
            offset += (uint32_t)snprintf(temp_buf + offset, buf_size - offset, "%lld", (long long)arr[i]);
        } else {
            offset += (uint32_t)snprintf(temp_buf + offset, buf_size - offset, "%lld, ", (long long)arr[i]);
        }
    }

    ret_stat = StrInitialize(str, temp_buf);
    free(temp_buf);

    /* Return the status passed down from StrInitialize */
    return ret_stat;
}

DefaultRet_t StrInitializeFromSInt32Arr(CString_t* str, const int32_t* arr, uint32_t arr_len) {
    uint32_t buf_size = arr_len * 14 + 1;
    uint32_t offset = 0;
    char* temp_buf;
    uint32_t i;
    DefaultRet_t ret_stat;

    /* Control flow: validate inputs */
    if ((str == NULL) || (arr == NULL) || (arr_len == 0)) {
        /* Return error on invalid arguments */
        return STAT_ERR_NULL;
    }

    temp_buf = (char*)malloc(buf_size);

    /* Control flow: verify allocation */
    if (temp_buf == NULL) {
        /* Return memory error */
        return STAT_ERR_MALLOC_FAILED;
    }

    /* Control flow: iterate over array items */
    for (i = 0; i < arr_len; i++) {
        /* Control flow: exclude comma for the final element */
        if (i == (arr_len - 1)) {
            offset += (uint32_t)snprintf(temp_buf + offset, buf_size - offset, "%lld", (long long)arr[i]);
        } else {
            offset += (uint32_t)snprintf(temp_buf + offset, buf_size - offset, "%lld, ", (long long)arr[i]);
        }
    }

    ret_stat = StrInitialize(str, temp_buf);
    free(temp_buf);

    /* Return the status passed down from StrInitialize */
    return ret_stat;
}

DefaultRet_t StrInitializeFromSInt64Arr(CString_t* str, const int64_t* arr, uint32_t arr_len) {
    uint32_t buf_size = arr_len * 24 + 1;
    uint32_t offset = 0;
    char* temp_buf;
    uint32_t i;
    DefaultRet_t ret_stat;

    /* Control flow: validate inputs */
    if ((str == NULL) || (arr == NULL) || (arr_len == 0)) {
        /* Return error on invalid arguments */
        return STAT_ERR_NULL;
    }

    temp_buf = (char*)malloc(buf_size);

    /* Control flow: verify allocation */
    if (temp_buf == NULL) {
        /* Return memory error */
        return STAT_ERR_MALLOC_FAILED;
    }

    /* Control flow: iterate over array items */
    for (i = 0; i < arr_len; i++) {
        /* Control flow: exclude comma for the final element */
        if (i == (arr_len - 1)) {
            offset += (uint32_t)snprintf(temp_buf + offset, buf_size - offset, "%lld", (long long)arr[i]);
        } else {
            offset += (uint32_t)snprintf(temp_buf + offset, buf_size - offset, "%lld, ", (long long)arr[i]);
        }
    }

    ret_stat = StrInitialize(str, temp_buf);
    free(temp_buf);

    /* Return the status passed down from StrInitialize */
    return ret_stat;
}

/* --- CString to Array Parsers --- */

DefaultRet_t StrConvertToUint8Arr(const CString_t* str, uint8_t* out_arr, uint32_t max_items) {
    char* end_ptr;
    char* curr_ptr;
    uint32_t count = 0;

    /* Control flow: validate inputs */
    if ((str == NULL) || (str->CString == NULL) || (out_arr == NULL) || (max_items == 0)) {
        /* Return error on null pointers */
        return STAT_ERR_NULL;
    }

    curr_ptr = str->CString;

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

DefaultRet_t StrConvertToUint16Arr(const CString_t* str, uint16_t* out_arr, uint32_t max_items) {
    char* end_ptr;
    char* curr_ptr;
    uint32_t count = 0;

    /* Control flow: validate inputs */
    if ((str == NULL) || (str->CString == NULL) || (out_arr == NULL) || (max_items == 0)) {
        /* Return error on null pointers */
        return STAT_ERR_NULL;
    }

    curr_ptr = str->CString;

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

DefaultRet_t StrConvertToUint32Arr(const CString_t* str, uint32_t* out_arr, uint32_t max_items) {
    char* end_ptr;
    char* curr_ptr;
    uint32_t count = 0;

    /* Control flow: validate inputs */
    if ((str == NULL) || (str->CString == NULL) || (out_arr == NULL) || (max_items == 0)) {
        /* Return error on null pointers */
        return STAT_ERR_NULL;
    }

    curr_ptr = str->CString;

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

DefaultRet_t StrConvertToUint64Arr(const CString_t* str, uint64_t* out_arr, uint32_t max_items) {
    char* end_ptr;
    char* curr_ptr;
    uint32_t count = 0;

    /* Control flow: validate inputs */
    if ((str == NULL) || (str->CString == NULL) || (out_arr == NULL) || (max_items == 0)) {
        /* Return error on null pointers */
        return STAT_ERR_NULL;
    }

    curr_ptr = str->CString;

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

DefaultRet_t StrConvertToSInt8Arr(const CString_t* str, int8_t* out_arr, uint32_t max_items) {
    char* end_ptr;
    char* curr_ptr;
    uint32_t count = 0;

    /* Control flow: validate inputs */
    if ((str == NULL) || (str->CString == NULL) || (out_arr == NULL) || (max_items == 0)) {
        /* Return error on null pointers */
        return STAT_ERR_NULL;
    }

    curr_ptr = str->CString;

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

DefaultRet_t StrConvertToSInt16Arr(const CString_t* str, int16_t* out_arr, uint32_t max_items) {
    char* end_ptr;
    char* curr_ptr;
    uint32_t count = 0;

    /* Control flow: validate inputs */
    if ((str == NULL) || (str->CString == NULL) || (out_arr == NULL) || (max_items == 0)) {
        /* Return error on null pointers */
        return STAT_ERR_NULL;
    }

    curr_ptr = str->CString;

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

DefaultRet_t StrConvertToSInt32Arr(const CString_t* str, int32_t* out_arr, uint32_t max_items) {
    char* end_ptr;
    char* curr_ptr;
    uint32_t count = 0;

    /* Control flow: validate inputs */
    if ((str == NULL) || (str->CString == NULL) || (out_arr == NULL) || (max_items == 0)) {
        /* Return error on null pointers */
        return STAT_ERR_NULL;
    }

    curr_ptr = str->CString;

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

DefaultRet_t StrConvertToSInt64Arr(const CString_t* str, int64_t* out_arr, uint32_t max_items) {
    char* end_ptr;
    char* curr_ptr;
    uint32_t count = 0;

    /* Control flow: validate inputs */
    if ((str == NULL) || (str->CString == NULL) || (out_arr == NULL) || (max_items == 0)) {
        /* Return error on null pointers */
        return STAT_ERR_NULL;
    }

    curr_ptr = str->CString;

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