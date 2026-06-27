#ifndef __APP_CONFIG_COMMON_HEADERS_H__
#define __APP_CONFIG_COMMON_HEADERS_H__

#include "../AppBase/All.h"

typedef struct __attribute__((packed)) ZECU_Info_t {
    CString_t       Label;
    IPv4EndPoint_t  EndPoint;
} ZECU_Info_t;

ZECU_Info_t ZECU_CCU;

#endif /// __APP_CONFIG_COMMON_HEADERS_H__