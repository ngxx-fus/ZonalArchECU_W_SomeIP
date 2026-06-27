#ifndef __DEVICE_COMM_H__
#define __DEVICE_COMM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>

/// @brief 8-bit unsigned integer type
typedef uint8_t Byte_t;

/// @brief 16-bit unsigned integer type
typedef uint16_t Word_t;

/// @brief 32-bit unsigned integer type
typedef uint32_t Dword_t;

/// @brief 64-bit unsigned integer type
typedef uint64_t Qword_t;

/// GENERATED SECTION | BEGIN /////////////////////////////////////////////////
#define GW_IP_ADDR_STR "10.0.0.1"
#define GW_IP_ADDR_HEX 0x0100000A
#define GW_IP_ADDR_INITIALIZER {10, 0, 0, 1}
extern const Byte_t GW_IP_ADDR[];

#define SUBNET_MASK_STR "255.255.255.0"
#define SUBNET_MASK_HEX 0x00FFFFFF
#define SUBNET_MASK_INITIALIZER {255, 255, 255, 0}
extern const Byte_t SUBNET_MASK[];

#define CCU_IPV4_ADDR_STR "10.0.0.102"
#define CCU_IPV4_ADDR_HEX 0x6600000A
#define CCU_IPV4_ADDR_INITIALIZER {10, 0, 0, 102}
extern const Byte_t CCU_IPV4_ADDR[];

#define CCU_IPV4_PORT_STR "30490"
#define CCU_IPV4_PORT_VAL 30490
#define CCU_IPV4_PORT_HEX 0x771A
extern const Word_t CCU_IPV4_PORT;

#define ECU_NAME_STR "Back-ZECU"

#define SRC_IP_ADDR_STR "10.0.0.47"
#define SRC_IP_ADDR_HEX 0x2F00000A
#define SRC_IP_ADDR_INITIALIZER {10, 0, 0, 47}
extern const Byte_t SRC_IP_ADDR[];

#define DEFAULT_PORT_STR "51786"
#define DEFAULT_PORT_VAL 51786
#define DEFAULT_PORT_HEX 0xCA4A
extern const Word_t DEFAULT_PORT;

#define SRC_MAC_ADDR_STR "3c:8a:1f:a2:c7:49"
#define SRC_MAC_ADDR_HEX 0x3C8A1FA2C749
#define SRC_MAC_ADDR_INITIALIZER {0x3c, 0x8a, 0x1f, 0xa2, 0xc7, 0x49}
extern const Byte_t SRC_MAC_ADDR[];

#define SERVICE_ETH_RUNTIME_EN eSERVICE_ENABLED
#define SERVICE_HEART_BEAT_EN eSERVICE_ENABLED
#define SERVICE_MOTOR_CONTROL_EN eSERVICE_ENABLED
#define SERVICE_MEAS_DISTANCE_EN eSERVICE_ENABLED
#define SERVICE_MEAS_LOCATION_EN eSERVICE_DISABLED
/// GENERATED SECTION | END   /////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif

#endif /// __DEVICE_PINOUT_H__
