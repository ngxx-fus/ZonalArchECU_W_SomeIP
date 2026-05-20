#ifndef __NETWORK_H__
#define __NETWORK_H__

#include "../__CommonHeaders.h"

/* NETWORK DEFINITION ***********************************************************************************************************/

/// @brief 8-bit unsigned integer type
typedef uint8_t     Byte_t;

/// @brief 16-bit unsigned integer type
typedef uint16_t    Word_t;

/// @brief 32-bit unsigned integer type
typedef uint32_t    Dword_t;

/// @brief 64-bit unsigned integer type
typedef uint64_t    Qword_t;

/// @brief Ethernet size type (16-bit unsigned int, but valid range from 1500 to 0xFFFF)
typedef Word_t      EthSize_t;

// /// @brief Generic pointer
// typedef void*       GenericPtr_t;

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

/// @brief Ethernet MAC Address structure using 48-bit (6 bytes)
typedef union EthMACAddr_t {
    uint64_t    Qword;
    uint8_t     Byte[8];            ///< 48-bit MAC address array
} EthMACAddr_t;

/// @brief IPv4 Address structure for flexible access as a 32-bit word or 4 individual bytes
typedef union IPv4Addr_t {
    uint32_t Dword;             ///< 32-bit unsigned int
    uint8_t Byte[4];            ///< 8-bit unsigned array
} IPv4Addr_t;

/// @brief Type of a IPv4 port
typedef union IPv4Port_t {
    uint16_t Word;              ///< 16-bit unsigned int
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

/* GENERIC PAYLOAD UTILS *********************************************************************************************************/

/// @brief Initialize a new generic payload by allocating memory based on existing Size
/// @param GP_Ptr Pointer to GenericPayload_t structure
/// @return void
void NewGenericPayload(GenericPayload_t * GP_Ptr, EthSize_t Size);

/// @brief Free allocated memory and reset payload structure
/// @param GP_Ptr Pointer to GenericPayload_t structure
/// @return void
void DeleteGenericPayload(GenericPayload_t * GP_Ptr);

/// @brief Copy data from source payload to destination payload
/// @param GP_Ptr_Src Pointer to source GenericPayload_t
/// @param GP_Ptr_Dst Pointer to destination GenericPayload_t
/// @return uint16_t Number of bytes successfully copied
uint16_t CopyGenericPayload(GenericPayload_t * GP_Ptr_Src, GenericPayload_t * GP_Ptr_Dst);

/* NETWORK UTILS ****************************************************************************************************************/

/// @brief Convert a 4-byte array to an IPv4 string
/// @param IPv4Addr Input array of 4 bytes
/// @param IPv4Str Output buffer (minimum 16 bytes)
/// @return ReturnCode_t STAT_OKE if successful
ReturnCode_t ConvertByteArr2IPvToAddress(uint8_t IPv4Addr[], char IPv4Str[]);

/// @brief Convert an IPv4 string to a 4-byte array
/// @param IPv4Str Input dotted-decimal string
/// @param IPv4Addr Output array of 4 bytes
/// @return ReturnCode_t STAT_OKE if successful
ReturnCode_t ConvertIPv4AddressToByteArr(char IPv4Str[], uint8_t IPv4Addr[]);

/// @brief Convert an IPv4 string to a 64-bit integer
/// @param IPv4Str Input dotted-decimal string
/// @param IPv4Addr Output pointer to uint64_t
/// @return ReturnCode_t STAT_OKE if successful
ReturnCode_t ConvertIPv4AddressToUInt64(char IPv4Str[], uint64_t *IPv4Addr);

/// @brief Convert a 64-bit integer to an IPv4 string
/// @param IPv4Addr Input 64-bit integer representing IP
/// @param IPv4Str Output buffer (minimum 16 bytes)
/// @return ReturnCode_t STAT_OKE if successful
ReturnCode_t ConvertUInt64ToIPv4Address(uint64_t IPv4Addr, char IPv4Str[]);

/// @brief Convert an IPv4 string to a 32-bit integer
/// @param IPv4Str Input dotted-decimal string
/// @param IPv4Addr Output pointer to uint32_t
/// @return ReturnCode_t STAT_OKE if successful
ReturnCode_t ConvertIPv4AddressToUInt32(char IPv4Str[], uint32_t *IPv4Addr);

/// @brief Convert a 32-bit integer to an IPv4 string
/// @param IPv4Addr Input 32-bit integer representing IP
/// @param IPv4Str Output buffer (minimum 16 bytes)
/// @return ReturnCode_t STAT_OKE if successful
ReturnCode_t ConvertUInt32ToIPv4Address(uint32_t IPv4Addr, char IPv4Str[]);

/// @brief Convert IPv4 octets to a 32-bit integer in Big-endian (Network) order
/// @param Octet0 First octet (e.g., 192)
/// @param Octet1 Second octet (e.g., 168)
/// @param Octet2 Third octet (e.g., 1)
/// @param Octet3 Fourth octet (e.g., 10)
/// @return uint32_t The packed 32-bit IP address
uint32_t IPv4ToUint32(uint8_t Octet0, uint8_t Octet1, uint8_t Octet2, uint8_t Octet3);

/// @brief Convert 6 MAC address octets to a 64-bit integer
/// @param Octet0 High byte of MAC (Most Significant Byte)
/// @param Octet1 Second byte
/// @param Octet2 Third byte
/// @param Octet3 Fourth byte
/// @param Octet4 Fifth byte
/// @param Octet5 Low byte of MAC (Least Significant Byte)
/// @return uint64_t The packed MAC address
uint64_t MACToUint64(uint8_t Octet0, uint8_t Octet1, uint8_t Octet2, uint8_t Octet3, uint8_t Octet4, uint8_t Octet5);

/// @brief Convert a byte array to a 32-bit unsigned integer using Big Endian
/// @param ByteArr Pointer to the source byte array
/// @param Size Number of bytes to process (max 4)
/// @return The converted 32-bit unsigned integer
uint32_t ConvertByteArrayToInt32_BigEndian(const uint8_t* ByteArr, uint8_t Size);

/// @brief Convert a byte array to a 64-bit unsigned integer using Big Endian
/// @param ByteArr Pointer to the source byte array
/// @param Size Number of bytes to process (max 8)
/// @return The converted 64-bit unsigned integer
uint64_t ConvertByteArrayToInt64_BigEndian(const uint8_t* ByteArr, uint8_t Size);

/// @brief Convert a byte array to a 32-bit unsigned integer using Little Endian
/// @param ByteArr Pointer to the source byte array
/// @param Size Number of bytes to process (max 4)
/// @return The converted 32-bit unsigned integer
uint32_t ConvertByteArrayToInt32_LittleEndian(const uint8_t* ByteArr, uint8_t Size);

/// @brief Convert a byte array to a 64-bit unsigned integer using Little Endian
/// @param ByteArr Pointer to the source byte array
/// @param Size Number of bytes to process (max 8)
/// @return The converted 64-bit unsigned integer
uint64_t ConvertByteArrayToInt64_LittleEndian(const uint8_t* ByteArr, uint8_t Size);

/// @brief Reverse the byte order of an array in-place (Endianness conversion)
/// @param Arr Generic pointer to the data array
/// @param Size Total number of bytes to reverse
void ReverseByteEndian(GenericPtr_t Arr, EthSize_t Size);

#endif /// __NETWORK_H__

/* EOF **************************************************************************************************************************/
