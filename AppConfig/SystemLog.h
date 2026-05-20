#ifndef __CONFIG_SYS_LOG_H__
#define __CONFIG_SYS_LOG_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef PRINT_HEADER_COMPILE_MESSAGE
#pragma message ("AppConfig/SystemLog.h")
#endif

#include <stdint.h>
#include <stdlib.h>

#define SYSTEM_LOG_EN               1
#define SYSTEM_ERR_EN               1
#define SYSTEM_LOG_L1_EN            1
#define SYSTEM_LOG_L2_EN            1
#define SYSTEM_LOG_EXIT_L1_EN       1
#define SYSTEM_LOG_EXIT_L2_EN       1
#define SYSTEM_LOG_ENTRY_L1_EN      1
#define SYSTEM_LOG_ENTRY_L2_EN      1

#define SYSTEM_SAFE_THREAD_LOG_EN   1

#ifdef __cplusplus
}
#endif

#endif /// __CONFIG_SYS_LOG_H__

