/// @file   espLogWrapper.h
/// @brief  Custom lightweight logging system for ESP32 with PascalCase macros and granular control.

#ifndef __ESP_CUSTOM_LOG_H__
#define __ESP_CUSTOM_LOG_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef PRINT_HEADER_COMPILE_MESSAGE
#pragma message ("AppESPWrap/ESPLogWrapper.h")
#endif

#include <stdio.h>              /// Standard input/output definitions
#include <stdarg.h>             /// Macros for variable arguments
#include "esp_timer.h"          /// ESP32 timer functions (esp_timer_get_time)
#include "esp_system.h"         /// ESP32 system APIs

/// This help the lib more readable!
#include "../AppConfig/All.h"

/// Auto-Config
#if (1 == 1)
    #ifndef SYSTEM_LOG_EN
        #define SYSTEM_LOG_EN               1
    #endif
    #ifndef SYSTEM_ERR_EN
        #define SYSTEM_ERR_EN               1
    #endif
    /// Added standalone control for Warn/Info
    #ifndef SYSTEM_WARN_EN
        #define SYSTEM_WARN_EN              1
    #endif
    #ifndef SYSTEM_INFO_EN
        #define SYSTEM_INFO_EN              1
    #endif
    #ifndef SYSTEM_LOG_L1_EN
        #define SYSTEM_LOG_L1_EN            0
    #endif
    #ifndef SYSTEM_LOG_L2_EN
        #define SYSTEM_LOG_L2_EN            0
    #endif
    #ifndef SYSTEM_LOG_EXIT_L1_EN
        #define SYSTEM_LOG_EXIT_L1_EN       1
    #endif 
    #ifndef SYSTEM_LOG_EXIT_L2_EN
        #define SYSTEM_LOG_EXIT_L2_EN       1
    #endif
    #ifndef SYSTEM_LOG_ENTRY_L1_EN
        #define SYSTEM_LOG_ENTRY_L1_EN      1
    #endif
    #ifndef SYSTEM_LOG_ENTRY_L2_EN
        #define SYSTEM_LOG_ENTRY_L2_EN      1
    #endif
#endif

#if (SYSTEM_SAFE_THREAD_LOG_EN == 1)
    #include "freertos/FreeRTOS.h"   /// Core FreeRTOS definitions
    #include "freertos/task.h"       /// Task management
    #include "freertos/semphr.h"     /// Semaphores and Mutexes

    /// @brief Spinlock to protect printing resource (UART)
    extern portMUX_TYPE __LogSpinLock;

    /// @brief Thread-safe logging wrapper function
    /// @param fmt Format string (printf style)
    /// @param ... Variable arguments
    void CoreLog(const char *fmt, ...);

#elif (SYSTEM_SAFE_THREAD_LOG_EN == 0)
    #define CoreLog ets_printf
#else 
    #ifdef SYSTEM_SAFE_THREAD_LOG_EN
        #undef SYSTEM_SAFE_THREAD_LOG_EN
    #endif 
    #define SYSTEM_SAFE_THREAD_LOG_EN 0
    #error("[AppESPWrap.c] SYSTEM_SAFE_THREAD_LOG_EN is not correct!")
#endif /// (SYSTEM_SAFE_THREAD_LOG_EN == 1)

/// ERROR LOGGING
#if (defined(SYSTEM_LOG_EN) && SYSTEM_LOG_EN == 1) && (defined(SYSTEM_ERR_EN) && SYSTEM_ERR_EN == 1)
    /// Log error message with timestamp and [err] tag
    #define SysErr(fmt, ...)    CoreLog("[%lld] [ERR] " fmt "\n", esp_timer_get_time(), ##__VA_ARGS__)
#else
    #define SysErr(fmt, ...)
#endif

/// WARNING LOGGING
#if (defined(SYSTEM_LOG_EN) && SYSTEM_LOG_EN == 1) && (defined(SYSTEM_WARN_EN) && SYSTEM_WARN_EN == 1)
    /// Log warning message with timestamp and [W] tag
    #define SysWarn(fmt, ...)   CoreLog("[%lld] [W] " fmt "\n", esp_timer_get_time(), ##__VA_ARGS__)
#else
    #define SysWarn(fmt, ...)
#endif

/// INFO LOGGING
#if (defined(SYSTEM_LOG_EN) && SYSTEM_LOG_EN == 1) && (defined(SYSTEM_INFO_EN) && SYSTEM_INFO_EN == 1)
    /// Log info message with timestamp and [Info] tag
    #define SysInfo(fmt, ...)   CoreLog("[%lld] [Info] " fmt "\n", esp_timer_get_time(), ##__VA_ARGS__)
#else
    #define SysInfo(fmt, ...)
#endif

/// LEVEL 1 LOGGING
#if (defined(SYSTEM_LOG_EN) && SYSTEM_LOG_EN == 1) && (defined(SYSTEM_LOG_L1_EN) && SYSTEM_LOG_L1_EN == 1)
    /// Log standard message with timestamp and [LOG] tag
    #define SysLog(fmt, ...)            CoreLog("[%lld] [LOG] " fmt "\n", esp_timer_get_time(), ##__VA_ARGS__)
    
    /// Log message with custom tag string (Mapped to L1)
    #define SysTagLog(tag, fmt, ...)    CoreLog("[%lld] [LOG] [%s] " fmt "\n", esp_timer_get_time(), tag, ##__VA_ARGS__)
#else
    #define SysLog(fmt, ...)
    #define SysTagLog(tag, fmt, ...)
#endif

/// LEVEL 2 LOGGING (VERBOSE)
#if (defined(SYSTEM_LOG_EN) && SYSTEM_LOG_EN == 1) && (defined(SYSTEM_LOG_L2_EN) && SYSTEM_LOG_L2_EN == 1)
    /// Log verbose/detailed message with timestamp and [verb] tag
    #define SysLogVer(fmt, ...)             CoreLog("[%lld] [verb] " fmt "\n", esp_timer_get_time(), ##__VA_ARGS__)
    
    /// Log verbose message with custom tag
    #define SysTagLogVer(tag, fmt, ...)     CoreLog("[%lld] [%s] " fmt "\n", esp_timer_get_time(), tag, ##__VA_ARGS__)
#else
    #define SysLogVer(fmt, ...)
    #define SysTagLogVer(tag, fmt, ...)
#endif

/// ENTRY/EXIT TRACES
#if (defined(SYSTEM_LOG_EN) && SYSTEM_LOG_EN == 1) && (defined(SYSTEM_LOG_ENTRY_L1_EN) && SYSTEM_LOG_ENTRY_L1_EN == 1)
    /// Log function entry trace (Level 1) with [>>>] tag
    #define SysEntry(fmt, ...)      CoreLog("[%lld] [>>>] " fmt "\n", esp_timer_get_time(), ##__VA_ARGS__)
#else
    #define SysEntry(fmt, ...)
#endif

#if (defined(SYSTEM_LOG_EN) && SYSTEM_LOG_EN == 1) && (defined(SYSTEM_LOG_ENTRY_L2_EN) && SYSTEM_LOG_ENTRY_L2_EN == 1)
    /// Log function entry trace (Level 2/Verbose) with [>>>] tag
    #define SysEntryVer(fmt, ...)   CoreLog("[%lld] [>>>] " fmt "\n", esp_timer_get_time(), ##__VA_ARGS__)
#else
    #define SysEntryVer(fmt, ...)
#endif

#if (defined(SYSTEM_LOG_EN) && SYSTEM_LOG_EN == 1) && (defined(SYSTEM_LOG_EXIT_L1_EN) && SYSTEM_LOG_EXIT_L1_EN == 1)
    /// Log function exit trace (Level 1) with [<<<] tag
    #define SysExit(fmt, ...)       CoreLog("[%lld] [<<<] " fmt "\n", esp_timer_get_time(), ##__VA_ARGS__)
#else
    #define SysExit(fmt, ...)
#endif

#if (defined(SYSTEM_LOG_EN) && SYSTEM_LOG_EN == 1) && (defined(SYSTEM_LOG_EXIT_L2_EN) && SYSTEM_LOG_EXIT_L2_EN == 1)
    /// Log function exit trace (Level 2/Verbose) with [<<<] tag
    #define SysExitVer(fmt, ...)    CoreLog("[%lld] [<<<] " fmt "\n", esp_timer_get_time(), ##__VA_ARGS__)
#else
    #define SysExitVer(fmt, ...)
#endif

#ifdef __cplusplus
}
#endif

#endif // __ESP_CUSTOM_LOG_H__
