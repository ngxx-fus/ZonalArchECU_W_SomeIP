#ifndef __DEVICE_PINOUT_H__
#define __DEVICE_PINOUT_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef PRINT_HEADER_COMPILE_MESSAGE
#pragma message ("AppConfig/Pinout.h")
#endif

#include <stdint.h>
#include <stdlib.h>

/// @brief Type of a GPIO
typedef int8_t Pin_t;

/// @brief States of pin
enum PIN_STATE_e {
    PIN_UNUSED = -1,
    PIN_INVALID = -2,
    PIN_NOT_FOUND = -3
};

/// @brief Check if pin is valid (0-63)
#define IsValidPin(p)       (((p) >= 0) && ((p) < 64))

/// @brief Check if pin is in standard range (0-31) for GPIO_OUT_REG
#define IsStandardPin(p)    (((p) >= 0) && ((p) < 32))

/// @brief Check if pin is in extended range (32-63) for GPIO_OUT1_REG
#define IsExtendedPin(p)    (((p) >= 32) && ((p) < 64))

/// GENERATED SECTION | BEGIN /////////////////////////////////////////////////
#define PIN_B_DC_IN0 ((Pin_t)2)
#define PIN_B_DC_IN1 ((Pin_t)4)
#define PIN_B_DC_IN2 ((Pin_t)16)
#define PIN_B_DC_IN3 ((Pin_t)17)
#define PIN_W5500_MOSI ((Pin_t)13)
#define PIN_W5500_MISO ((Pin_t)12)
#define PIN_W5500_CLK ((Pin_t)14)
#define PIN_W5500_SCS ((Pin_t)15)
#define PIN_W5500_INT ((Pin_t)26)
#define PIN_W5500_RST ((Pin_t)27)
#define PIN_US_TRIG ((Pin_t)25)
#define PIN_US_L_ECHO ((Pin_t)34)
#define PIN_US_C_ECHO ((Pin_t)35)
#define PIN_US_R_ECHO ((Pin_t)32)
/// GENERATED SECTION | END   /////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif

#endif /// __DEVICE_PINOUT_H__
