#ifndef __ETHERNET_W5500_H__
#define __ETHERNET_W5500_H__

#include <stdint.h>

#include "../../AppUtils/All.h"
#include "../../AppConfig/All.h"

#include "EthernetW550Cmds.h"


/*SPI Transaction*/

/// @brief W5500 SPI Frame Header (Address + Control)
/// @note Packed to ensure 3-byte size for SPI transmission
__attribute__((packed))
typedef struct W5500Header_t {
    /* Address Phase: 16 bits Offset Address */
    union {
        uint16_t Addr;
        struct {
            uint8_t AddrH; /* MSB: Sent first */
            uint8_t AddrL; /* LSB: Sent second */
        };
    };

    /* Control Phase: 1 Control Byte */
    union {
        uint8_t Control;
        struct {
            uint8_t OpMode   : 2; /* Bits 0-1: Operating Mode */
            uint8_t nRW      : 1; /* Bit 2: Read/Write (0:Read, 1:Write) */
            uint8_t BlockSel : 5; /* Bits 3-7: Socket/Register Block Select */
        };
    };
} W5500Header_t;

/// @brief Full Frame including data pointer
__attribute__((packed))
typedef union W5500Transaction_t {
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
ReturnCode_t*   W5500_Create(Pin_t MISO, Pin_t MOSI, Pin_t CLK, Pin_t SCS, Pin_t RST, Pin_t INT);

/// @brief Delete EthernetW5500_t object and set ptr to NULL to prevent dangling pointer
/// @param Ptr 
/// @return 
ReturnCode_t*   W5500_Delete(ReturnCode_t** Ptr);

/// @brief Pull SCS pin to down then send the 3 bytes of header, then keep the SCS low when exit the function
/// @param Ptr 
/// @param Header 
/// @return 
ReturnCode_t    W5500_SendHeader(ReturnCode_t* Ptr, W5500Header_t Header);

/// @brief Pull SCS pin to down; Send N bytes of payload; Set SCS to HIGH
/// @param Ptr 
/// @param Payload 
/// @param PayloadLength 
/// @return 
ReturnCode_t    W5500_SendPayload(ReturnCode_t* Ptr, void* Payload, uint16_t PayloadLength);

/// @brief Pull SCS pin to down then send the 3 bytes of header then send 1-4 byte of payload depend of header set SCS to HIGH
/// @param Ptr 
/// @param Header 
/// @return 
ReturnCode_t    W5500_SendCommand(ReturnCode_t* Ptr, W5500Header_t Header, void* Payload);

/// @brief Pull SCS pin to down then send the 3 bytes of header then send 1-4 byte of payload depend of header set SCS to HIGH
/// @param Ptr 
/// @param Header 
/// @return 
ReturnCode_t    W5500_SendData(ReturnCode_t* Ptr, W5500Header_t Header, void* Payload, uint16_t PayloadLength);


#endif __ETHERNET_W5500_H__