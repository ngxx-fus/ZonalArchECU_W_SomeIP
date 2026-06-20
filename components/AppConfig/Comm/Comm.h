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

#define ECU_NAME_STR "Front-ZECU"

#define SRC_IP_ADDR_STR "10.0.0.58"
#define SRC_IP_ADDR_HEX 0x3A00000A
#define SRC_IP_ADDR_INITIALIZER {10, 0, 0, 58}
extern const Byte_t SRC_IP_ADDR[];

#define DEFAULT_PORT_STR "51786"
#define DEFAULT_PORT_VAL 51786
#define DEFAULT_PORT_HEX 0xCA4A
extern const Word_t DEFAULT_PORT;

#define SRC_MAC_ADDR_STR "b4:bf:e9:33:e0:6c"
#define SRC_MAC_ADDR_HEX 0xB4BFE933E06C
#define SRC_MAC_ADDR_INITIALIZER {0xb4, 0xbf, 0xe9, 0x33, 0xe0, 0x6c}
extern const Byte_t SRC_MAC_ADDR[];

/// GENERATED SECTION | END   /////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif

#endif /// __DEVICE_PINOUT_H__
