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
#define SRC_MAC_ADDR_STR "59:32:01:DC:08:00"
#define SUBNET_MASK_STR "255.255.255.0"
#define SRC_IP_ADDR_STR "10.0.0.59"
#define SOMEIP_PORT_STR "42"
extern Byte_t GW_IP_ADDR[];
extern Byte_t SRC_MAC_ADDR[];
extern Byte_t SUBNET_MASK[];
extern Byte_t SRC_IP_ADDR[];
extern Byte_t SOMEIP_PORT[];

/// GENERATED SECTION | END   /////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif

#endif /// __DEVICE_PINOUT_H__
