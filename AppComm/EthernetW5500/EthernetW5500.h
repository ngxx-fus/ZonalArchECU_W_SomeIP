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
typedef struct __attribute__((packed)) W5500Header_t {
    /* Control Phase: 1 Control Byte */
    union {
        uint8_t ControlPhase;
        struct {
            uint8_t OpMode   : 2; /* Bits 0-1: Operating Mode */
            uint8_t nRW      : 1; /* Bit 2: Read/Write (0:Read, 1:Write) */
            uint8_t BlockSel : 5; /* Bits 3-7: Socket/Register Block Select */
        };
    };

    /* Address Phase: 16 bits Offset Address */
    union {
        uint16_t AddrPhase;
        struct {
            uint8_t AddrH; /* MSB: Sent first */
            uint8_t AddrL; /* LSB: Sent second */
        };
    };
} W5500Header_t;

/// @brief Full Frame including data pointer
typedef union __attribute__((packed)) W5500Transaction_t {
    struct {
        W5500Header_t   Header;
        uint8_t         Data[1500];
        uint16_t        PayloadLength;
    };
    uint8_t Byte[sizeof(
        struct {
            W5500Header_t   Header;
            uint8_t         Data[1500];
            uint16_t        PayloadLength;
        }
    ) - 2 /*Remove 2 bytes from `Length`*/];
} W5500Transaction_t;

/// @brief Comprehensive W5500 Management Structure
/// @note Packed union allows raw byte access (bypass) for serialization or debugging
typedef union __attribute__((packed)) {
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

/// @brief Pull SCS pin to down then send the 3 bytes of header, then keep the SCS low when exit the function
/// @param Ptr 
/// @param Header 
/// @return 
ReturnCode_t    W5500_SendHeader(EthernetW5500_t* Ptr, W5500Header_t Header);

/// @brief Pull SCS pin to down; Send N bytes of payload; Set SCS to HIGH
/// @param Ptr 
/// @param Payload 
/// @param PayloadLength 
/// @return 
ReturnCode_t    W5500_SendPayload(EthernetW5500_t* Ptr, void* Payload, uint16_t PayloadLength);

/// @brief Pull SCS pin to down then send the 3 bytes of header then send 1-4 byte of payload depend of header set SCS to HIGH
/// @param Ptr 
/// @param Header 
/// @return 
ReturnCode_t    W5500_SendCommand(EthernetW5500_t* Ptr, W5500Header_t Header, void* Payload);

/// @brief Pull SCS pin to down then send the 3 bytes of header then send 1-4 byte of payload depend of header set SCS to HIGH
/// @param Ptr 
/// @param Header 
/// @return 
ReturnCode_t    W5500_SendData(EthernetW5500_t* Ptr, W5500Header_t Header, void* Payload, uint16_t PayloadLength);

/// @brief Pull SCS pin to down then READ the 3 bytes of header then read 1-4 byte of payload depend of header set SCS to HIGH
/// @param Ptr 
/// @param Header 
/// @return 
ReturnCode_t    W5500_ReceiveData(EthernetW5500_t* Ptr, W5500Header_t Header, void* Payload, uint16_t PayloadLength);

/*Test*/

ReturnCode_t W5500_LoopbackTest(EthernetW5500_t* Ptr, uint8_t tx_data[2], uint8_t rx_full_frame[5]);

/*Public utils*/

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

#endif /// __ETHERNET_W5500_H__