/// @file   espGPIOWrapper.h
/// @brief  GPIO wrapper for ESP32; provides config helpers and fast direct-register macros.

#ifndef __ESP_GPIO_WRAPPER_H__
#define __ESP_GPIO_WRAPPER_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef PRINT_HEADER_COMPILE_MESSAGE
#pragma message ("AppESPWrap/ESPGPIOWrapper.h")
#endif

#include <stdint.h>             /// Standard fixed-width integer types
#include <stdlib.h>             /// Standard library definitions (malloc, free, etc.)
#include <stdbool.h>            /// Boolean type definitions
#include <stdarg.h>             /// Variable argument handling

#include "driver/gpio.h"        /// ESP-IDF High-level GPIO driver
#include "esp_log.h"            /// ESP logging utilities
#include "esp_timer.h"          /// High-resolution timer API

#include "rom/ets_sys.h"        /// ROM functions (ets_printf, ets_delay_us)
#include "hal/gpio_ll.h"        /// Low-level GPIO hardware abstraction layer
#include "soc/gpio_struct.h"    /// GPIO hardware register structure
#include "soc/gpio_reg.h"       /// GPIO register addresses and bit masks

/// @brief Get input level of all GPIO 0-31 (Atomic, Fast)
/// @return 32-bit raw value (Bit N = GPIO N status)
#define IOStandardGet()              (GPIO.in)

/// @brief Get input level of all GPIO 32-39+ (Atomic, Fast)
/// @return 32-bit raw value (Bit N = GPIO 32+N status)
#define IOExtendedGet()              (GPIO.in1.val)

/// @brief Set output level High for GPIO 0-31 (Atomic, Fast)
/// @param mask Bitmask of pins to set (1 bit = 1 pin)
#define IOStandardSet(mask)          (GPIO.out_w1ts = (uint32_t)(mask))

/// @brief Set output level Low for GPIO 0-31 (Atomic, Fast)
/// @param mask Bitmask of pins to clear
#define IOStandardClr(mask)          (GPIO.out_w1tc = (uint32_t)(mask))

/// @brief Set output level High for GPIO 32-39+ (Atomic, Fast)
/// @param mask Bitmask relative to the high bank (bit 0 = GPIO 32)
#define IOExtendedSet(mask)          (GPIO.out1_w1ts.val = (uint32_t)(mask))

/// @brief Set output level Low for GPIO 32-39+ (Atomic, Fast)
/// @param mask Bitmask relative to the high bank
#define IOExtendedClr(mask)          (GPIO.out1_w1tc.val = (uint32_t)(mask))

/// @brief Get input level of ALL GPIOs (0-39+) combined
/// @details Reads both low and high banks and combines them. 
///          Note: Not strictly atomic across banks (2 separate reads).
/// @return 64-bit raw value (Bit N = GPIO N status)
#define IOGet()                      ( ((uint64_t)IOExtendedGet() << 32) | (uint64_t)IOStandardGet() )

/// @brief Set output level High for ANY GPIOs (0-39+)
/// @param mask64 64-bit Bitmask of pins to set (must use 1ULL << pin)
#define IOSet(mask64)                do { \
                                        IOStandardSet((uint32_t)(mask64)); \
                                        IOExtendedSet((uint32_t)((uint64_t)(mask64) >> 32)); \
                                     } while(0)

/// @brief Set output level Low for ANY GPIOs (0-39+)
/// @param mask64 64-bit Bitmask of pins to clear (must use 1ULL << pin)
#define IOClr(mask64)                do { \
                                        IOStandardClr((uint32_t)(mask64)); \
                                        IOExtendedClr((uint32_t)((uint64_t)(mask64) >> 32)); \
                                     } while(0)

/// @brief Configure a GPIO pin with specific mode, pull-up/down settings, and interrupt type
/// @param pin_bit_mask Bitmask of the GPIO(s) to configure
/// @param mode         GPIO mode (e.g., GPIO_MODE_INPUT, GPIO_MODE_OUTPUT)
/// @param pull_up_en   Enable/Disable pull-up resistor
/// @param pull_down_en Enable/Disable pull-down resistor
/// @param intr_type    Interrupt type (e.g., GPIO_INTR_POSEDGE, GPIO_INTR_DISABLE)
void IOConfig(uint64_t pin_bit_mask, gpio_mode_t mode, gpio_pullup_t pull_up_en, gpio_pulldown_t pull_down_en, gpio_int_type_t intr_type);

/// @brief Configure GPIO(s) as output with no internal pull resistors and interrupts disabled
/// @param pin_bit_mask Bitmask of the GPIO(s) to configure
void IOConfigAsOutput(uint64_t pin_bit_mask, gpio_pullup_t pull_up_en, gpio_pulldown_t pull_down_en);

/// @brief Configure GPIO(s) as input with no internal pull resistors and interrupts disabled
/// @param pin_bit_mask Bitmask of the GPIO(s) to configure
void IOConfigAsInput(uint64_t pin_bit_mask, gpio_pullup_t pull_up_en, gpio_pulldown_t pull_down_en);

/// @brief Configure GPIO as Input/Output Open-Drain with Internal Pull-up
/// @param pin_bit_mask Bit mask of the pin
void IOConfigAsInputOutputODPullUp(uint64_t pin_bit_mask);

/// @brief Configure GPIO as Input and Output (Bidirectional)
/// @param pin_bit_mask Bit mask of the pin
/// @param pull_up_en Enable internal pull-up
/// @param pull_down_en Enable internal pull-down
void IOConfigAsInputOutput(uint64_t pin_bit_mask, gpio_pullup_t pull_up_en, gpio_pulldown_t pull_down_en);

/// @brief Configure GPIO as Output Open-Drain with Internal Pull-up enabled
/// @param pin_bit_mask Bit mask of the pin
void IOConfigAsOutputODPullUp(uint64_t pin_bit_mask);

/// @brief Fast set High for GPIO 0-31 (Write 1 to Set)
#define GPIOSetHigh(GPIO_MASK)  GPIO.out_w1ts = (GPIO_MASK)

/// @brief Fast set Low for GPIO 0-31 (Write 1 to Clear)
#define GPIOSetLow(GPIO_MASK)   GPIO.out_w1tc = (GPIO_MASK)

/// @brief Fast set High for GPIO 32-64 (Write 1 to Set - High Register)
#define GPIOSetHigh1(GPIO_MASK) GPIO.out1_w1ts = (GPIO_MASK)  

/// @brief Fast set Low for GPIO 32-64 (Write 1 to Clear - High Register)
#define GPIOSetLow1(GPIO_MASK)  GPIO.out1_w1tc = (GPIO_MASK)  

// typedef void IRAM_ATTR (isrFunc_t)(void *pv);

#ifdef __cplusplus
}
#endif

#endif