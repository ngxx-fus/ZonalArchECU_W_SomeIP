#ifndef __APP_ESP_WRAP_ALL_H__
#define __APP_ESP_WRAP_ALL_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef PRINT_HEADER_COMPILE_MESSAGE
#pragma message ("AppESPWrap/All.h")
#endif

#include <stdint.h>
#include <stdlib.h>

#include <rom/ets_sys.h>
#include <esp_random.h>
#include <esp_heap_caps.h>
#include <driver/ledc.h>

/// This help the lib more readable!
/// #include "../AppConfig/All.h"

#include "sdkconfig.h"
#include "./ESPGPIOWrapper.h"
#include "./ESPLogWrapper.h"
#include "./ESPFreeRTOSWrapper.h"


#ifdef __cplusplus
}
#endif

#endif /// __APP_ESP_WRAP_ALL_H__