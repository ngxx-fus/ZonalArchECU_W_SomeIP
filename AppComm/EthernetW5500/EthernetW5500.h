#ifndef __ETHERNET_W5500_H__
#define __ETHERNET_W5500_H__

#include <stdint.h>

#include "../../AppUtils/All.h"
#include "../../AppConfig/All.h"
#include "../../AppESPWrap/All.h"

#include "EthernetW5500Cmds.h"

/*SPI Transaction*/

/// @brief W5500 SPI Frame Header (Address + Control)
/// @note Packed to ensure 3-byte size for SPI transmission
typedef union W5500Header_t {
    struct __attribute__((packed))  {
        /* Address Phase: 16 bits Offset Address */
        union {
            uint16_t AddrPhase;
            struct {
                uint8_t AddrH; /* MSB: Sent first */
                uint8_t AddrL; /* LSB: Sent second */
            };
        };

        /* Control Phase: 1 Control Byte */
        union {
            uint8_t ControlPhase;
            struct {
                uint8_t OpMode   : 2; /* Bits 0-1: Operating Mode */
                uint8_t nRW      : 1; /* Bit 2: Read/Write (0:Read, 1:Write) */
                uint8_t BlockSel : 5; /* Bits 3-7: Socket/Register Block Select */
            };
        };

    };
    uint8_t Byte[3];
} W5500Header_t;

/// @brief Full Frame including data pointer
typedef union W5500Transaction_t {
    struct  __attribute__((packed)) {
        W5500Header_t   Header;
        struct  __attribute__((packed))   {
            uint8_t         Byte[1500];
            uint16_t        Length;
        } Payload;
    };
    uint8_t Byte[sizeof(
        struct  __attribute__((packed)) {
            W5500Header_t   Header;
            struct  __attribute__((packed))  {
                uint8_t         Byte[1500];
                uint16_t        Length;
            } Payload;
        }
    ) - 2 /*Remove 2 bytes from `Length`*/];
} W5500Transaction_t;

/// @brief Comprehensive W5500 Management Structure
/// @note Packed union allows raw byte access (bypass) for serialization or debugging
typedef union __attribute__((packed)) EthernetW5500_t {
    struct {
        /* Hardware Pinout Mapping */
        struct {
            union {
                struct {
                    Pin_t MISO; /* Corrected from duplicate MOSI */
                    Pin_t MOSI;
                    Pin_t CLK;
                    Pin_t SCS;
                    Pin_t RST;
                    Pin_t INT;
                };
                Pin_t Arr[6];
            };
        } Pinout;

        /* SPI Transaction Buffers (Includes 1500-byte Data each) */
        W5500Transaction_t TxFrame;
        W5500Transaction_t RxFrame;

        /* System and Operation Flags */
        SafeFlag_t Flag;
    };

    /* Bypass array for raw memory access */
    uint8_t Raw[sizeof(
        struct {
            struct {
                union {
                    struct { Pin_t MISO; Pin_t MOSI; Pin_t CLK; Pin_t SCS; Pin_t RST; Pin_t INT; };
                    Pin_t Arr[6];
                };
            } Pinout;
            W5500Transaction_t TxFrame;
            W5500Transaction_t RxFrame;
            SafeFlag_t Flag;
        }
    )];
} EthernetW5500_t;

/*Public API*/

/// @brief Allocate new EthernetW5500_t with preset pin; Init HSPI (SCS is manually controled)
/// @param MISO 
/// @param MOSI 
/// @param CLK 
/// @param SCS 
/// @param RST 
/// @param INT 
/// @return 
EthernetW5500_t*W5500_Create(Pin_t MISO, Pin_t MOSI, Pin_t CLK, Pin_t SCS, Pin_t RST, Pin_t INT);

/// @brief Delete EthernetW5500_t object and set ptr to NULL to prevent dangling pointer
/// @param Ptr 
/// @return 
ReturnCode_t    W5500_Delete(EthernetW5500_t** Ptr);

/// @brief Pre-configures the SPI frame header in the transmission buffer
/// @param Ptr Pointer to the W5500 management structure
/// @param RegAddr 16-bit offset address of the target register
/// @param BlockSelBits 5-bit block selection (Socket or Common registers)
/// @param nRW Read/Write command (0 for Read, 1 for Write)
/// @param OpMode 2-bit operating mode (VDM or FDM)
/// @return STAT_OKE on success, STAT_ERR_NULL if Ptr is invalid
ReturnCode_t W5500_SetHeader(EthernetW5500_t* Ptr, uint8_t BlockSelBits, uint16_t RegAddr);

/// @brief Copies data from an external buffer into the TX frame payload
/// @param Ptr Pointer to the W5500 management structure
/// @param Payload Pointer to the source data buffer
/// @param N Number of bytes to copy (clamped to 1500)
/// @return STAT_OKE on success
ReturnCode_t W5500_SetTxPayload(EthernetW5500_t* Ptr, void* Payload, int32_t N);

/// @brief Retrieves data from the TX frame payload into an external buffer
/// @param Ptr Pointer to the W5500 management structure
/// @param Payload Pointer to the destination buffer
/// @param N Number of bytes to copy
/// @return STAT_OKE on success
ReturnCode_t W5500_GetTxPayload(EthernetW5500_t* Ptr, void* Payload, int32_t N);

/// @brief Manually sets data into the RX frame payload buffer
/// @param Ptr Pointer to the W5500 management structure
/// @param Payload Pointer to the source data buffer
/// @param N Number of bytes to copy
/// @return STAT_OKE on success
ReturnCode_t W5500_SetRxPayload(EthernetW5500_t* Ptr, void* Payload, int32_t N);

/// @brief Retrieves received data from the RX frame payload into an external buffer
/// @param Ptr Pointer to the W5500 management structure
/// @param Payload Pointer to the destination buffer
/// @param N Number of bytes to copy
/// @return STAT_OKE on success
ReturnCode_t W5500_GetRxPayload(EthernetW5500_t* Ptr, void* Payload, int32_t N);

/// @brief Executes a complete SPI transaction based on pre-configured frames
/// @param Ptr Pointer to the W5500 management structure
/// @param nRW Operation mode for the current transaction
/// @return STAT_OKE on success, error code otherwise
ReturnCode_t W5500_AccessNByte(EthernetW5500_t* Ptr, uint8_t nRW);

/// @brief Read an arbitrary number of bytes from W5500
/// @param Ptr Pointer to EthernetW5500_t object
/// @param Buffer Destination buffer to store received data
/// @param Len Number of bytes to read
/// @return ReturnCode_t Status of the operation
ReturnCode_t W5500_ReadNByte(EthernetW5500_t* Ptr, uint8_t* Buffer, uint16_t Len);

/// @brief Reads a single byte from a pre-configured register
/// @param Ptr Pointer to the W5500 management structure
/// @return Received byte value (8-bit)
ReturnCode_t W5500_ReadByte(EthernetW5500_t* Ptr);

/// @brief Reads two bytes from a pre-configured register
/// @param Ptr Pointer to the W5500 management structure
/// @return 16-bit combined value (Big-Endian)
ReturnCode_t W5500_ReadDoubleByte(EthernetW5500_t* Ptr);

/// @brief Reads four bytes from a pre-configured register
/// @param Ptr Pointer to the W5500 management structure
/// @return 32-bit combined value (Big-Endian)
ReturnCode_t W5500_ReadQuartByte(EthernetW5500_t* Ptr);

/// @brief Write an arbitrary number of bytes to W5500
/// @param Ptr Pointer to EthernetW5500_t object
/// @param Data Source buffer containing data to be sent
/// @param Len Number of bytes to write
/// @return ReturnCode_t Status of the operation
ReturnCode_t W5500_WriteNByte(EthernetW5500_t* Ptr, uint8_t* Data, uint16_t Len);

/// @brief Writes a single byte to a pre-configured register
/// @param Ptr Pointer to the W5500 management structure
/// @param Data 8-bit value to transmit
/// @return STAT_OKE on success
ReturnCode_t W5500_WriteByte(EthernetW5500_t* Ptr, uint8_t Data);

/// @brief Writes two bytes to a pre-configured register
/// @param Ptr Pointer to the W5500 management structure
/// @param Data 16-bit value to transmit (will be converted to Big-Endian)
/// @return STAT_OKE on success
ReturnCode_t W5500_WriteDoubleByte(EthernetW5500_t* Ptr, uint16_t Data);

/// @brief Writes four bytes to a pre-configured register
/// @param Ptr Pointer to the W5500 management structure
/// @param Data 32-bit value to transmit (will be converted to Big-Endian)
/// @return STAT_OKE on success
ReturnCode_t W5500_WriteQuartByte(EthernetW5500_t* Ptr, uint32_t Data);

/* COMPOSE FN *******************************************************************************************************************/

/// @brief Read the hardware version code from VERSIONR (0x0039)
/// @param Ptr Pointer to the W5500 controller structure
/// @return STAT_OKE if successful, STAT_ERR_NULL if pointer is invalid
ReturnCode_t W5500_GetModuleVersion(EthernetW5500_t* Ptr);

/* UTILS ************************************************************************************************************************/

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
#endif /// __ETHERNET_W5500_H__