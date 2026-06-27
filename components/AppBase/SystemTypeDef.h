#ifndef __APP_UTILS_SYSTEM_TYPE_DEF_H__
#define __APP_UTILS_SYSTEM_TYPE_DEF_H__

#include <stdint.h>

#ifndef __SYSTEM_DEF_TYPE_UNSIGNED_INT__
#define __SYSTEM_DEF_TYPE_UNSIGNED_INT__

/// @brief 8-bit unsigned integer type
typedef uint8_t     Byte_t;

/// @brief 16-bit unsigned integer type
typedef uint16_t    Word_t;

/// @brief 32-bit unsigned integer type
typedef uint32_t    Dword_t;

/// @brief 64-bit unsigned integer type
typedef uint64_t    Qword_t;

/// @brief Ethernet size type (16-bit unsigned int32_t, but valid range from 1500 to 0xFFFF)
typedef Word_t      EthSize_t;

#endif /*__SYSTEM_DEF_TYPE_UNSIGNED_INT__*/

#ifndef __SYSTEM_DEF_TYPE_GENERIC_POINTER__
#define __SYSTEM_DEF_TYPE_GENERIC_POINTER__

/// @brief Generic pointer (consist of all casting type)
typedef union GenericPtr_t {
    void*       Void;

    int8_t*     Int8;
    int16_t*    Int16;
    int32_t*    Int32;
    int32_t*    Int64;

    uint8_t*    UInt8;
    uint16_t*   UInt16;
    uint32_t*   UInt32;
    uint32_t*   UInt64;

    uint8_t*    Byte;
    uint16_t*   Word;
    uint32_t*   DWord;
    uint32_t*   QWord;
} GenericPtr_t;

#define GenericNullPtr  ((GenericPtr_t)(NULL))

#endif /*__SYSTEM_DEF_TYPE_GENERIC_POINTER__*/

#ifndef __SYSTEM_DEF_TYPE_NETWORK__
#define __SYSTEM_DEF_TYPE_NETWORK__

/// @brief Ethernet MAC Address structure using 48-bit (6 bytes)
typedef union EthMACAddr_t {
    uint64_t    Qword;
    uint8_t     Byte[8];            ///< 48-bit MAC address array
} EthMACAddr_t;

/// @brief IPv4 Address structure for flexible access as a 32-bit word or 4 individual bytes
typedef union IPv4Addr_t {
    uint32_t Dword;             ///< 32-bit unsigned int32_t
    uint8_t Byte[4];            ///< 8-bit unsigned array
} IPv4Addr_t;

/// @brief Type of a IPv4 port
typedef union IPv4Port_t {
    uint16_t Word;              ///< 16-bit unsigned int32_t
    uint8_t Byte[2];            ///< 8-bit unsigned array
} IPv4Port_t;

/// @brief Type of a IPv4 endpoint (includes IPv4's Address and IPv4's Port)
typedef union IPv4EndPoint_t {
    struct __attribute__((packed)) {
        IPv4Addr_t Addr;        ///< IPv4 ddress
        IPv4Port_t Port;        ///< IPv4 port
    } Member;
    uint8_t Byte[6];            ///< Total 6 bytes 
} IPv4EndPoint_t;

/// @brief Generic payload structure
typedef struct __attribute__((packed)) GenericPayload_t{
    uint16_t Size;
    void*    Ptr;
} GenericPayload_t;


/// @brief IPv4Packet structure
typedef union IPv4Packet_t {
    struct __attribute__((packed)) {
        IPv4EndPoint_t      Src;    ///< Source end-point
        IPv4EndPoint_t      Dst;    ///< Destination end-point
        GenericPayload_t    Payload;
    };
    uint8_t ByteArr[
        (sizeof(IPv4EndPoint_t)<<1) + sizeof(GenericPayload_t) 
    ];                          ///< Access entire EthPacket_t as byte array
} IPv4Packet_t;

#endif /*__SYSTEM_DEF_TYPE_NETWORK__*/


#define __SYSTEM_DEF_TYPE_UNSIGNED_INT__