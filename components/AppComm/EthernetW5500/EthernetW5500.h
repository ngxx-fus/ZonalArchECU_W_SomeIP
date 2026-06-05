#ifndef __ETHERNET_W5500_H__
#define __ETHERNET_W5500_H__

#include <stdint.h>

#include "../__CommonHeaders.h"

#include "Network.h"
#include "EthernetW5500Cmds.h"

/// @brief Semaphore lock handler
extern SemaphoreHandle_t    W5500_Lock;

/*SPI TRANSACTION ***************************************************************************************************************/

/// @brief W5500 SPI Frame Header (Address + Control)
typedef union W5500Header_t {
    struct __attribute__((packed))  {
        /* Address Phase: 16 bits Offset Address */
        union {
            Word_t AddrPhase;
            struct {
                Byte_t AddrH; /* MSB: Sent first */
                Byte_t AddrL; /* LSB: Sent second */
            };
        };

        /* Control Phase: 1 Control Byte */
        union {
            Byte_t ControlPhase;
            struct {
                Byte_t OpMode   : 2; /* Bits 0-1: Operating Mode */
                Byte_t nRW      : 1; /* Bit 2: Read/Write (0:Read, 1:Write) */
                Byte_t BlockSel : 5; /* Bits 3-7: Socket/Register Block Select */
            };
        };

    };
    Byte_t Byte[3];
} W5500Header_t;

/// @brief Full Frame including data pointer
typedef union W5500Transaction_t {
    struct  __attribute__((packed)) {
        W5500Header_t   Header;
        struct  __attribute__((packed))   {
            Byte_t         Byte[1500];
            Word_t        Length;
        } Payload;
    };
    Byte_t Byte[sizeof(
        struct  __attribute__((packed)) {
            W5500Header_t   Header;
            struct  __attribute__((packed))  {
                Byte_t         Byte[1500];
                Word_t        Length;
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
    Byte_t Raw[sizeof(
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

/*IsrHandler function pointer ****************************************************************************************************/

/* W5500 Isr Tracking task handle*/
extern TaskHandle_t W5500_TaskComm_TaskHandler;

/* W5500 Isr Tracking task function pointer */
extern void (*W5500_TaskComming_Function)(void*);

/*Public API *********************************************************************************************************************/

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

/// @brief Check if the W5500 is currently asserting an interrupt (Active Low)
/// @param Ptr Pointer to the EthernetW5500_t structure
/// @return STAT_OKE if the interrupt pin is LOW (asserted), STAT_ERR otherwise
ReturnCode_t    W5500_IsInInterruptPhase(EthernetW5500_t* Ptr);

/// @brief Pre-configures the SPI frame header in the transmission buffer
/// @param Ptr Pointer to the W5500 management structure
/// @param RegAddr 16-bit offset address of the target register
/// @param BlockSelBits 5-bit block selection (Socket or Common registers)
/// @param nRW Read/Write command (0 for Read, 1 for Write)
/// @param OpMode 2-bit operating mode (VDM or FDM)
/// @return STAT_OKE on success, STAT_ERR_NULL if Ptr is invalid
ReturnCode_t W5500_SetHeader(EthernetW5500_t* Ptr, Byte_t BlockSelBits, Word_t RegAddr);

/// @brief Copies data from an external buffer into the TX frame payload
/// @param Ptr Pointer to the W5500 management structure
/// @param Payload Pointer to the source data buffer
/// @param N Number of bytes to copy (clamped to 1500)
/// @return STAT_OKE on success
ReturnCode_t W5500_SetTxPayload(EthernetW5500_t* Ptr, void* Payload, Dword_t N);

/// @brief Retrieves data from the TX frame payload into an external buffer
/// @param Ptr Pointer to the W5500 management structure
/// @param Payload Pointer to the destination buffer
/// @param N Number of bytes to copy
/// @return STAT_OKE on success
ReturnCode_t W5500_GetTxPayload(EthernetW5500_t* Ptr, void* Payload, Dword_t N);

/// @brief Manually sets data into the RX frame payload buffer
/// @param Ptr Pointer to the W5500 management structure
/// @param Payload Pointer to the source data buffer
/// @param N Number of bytes to copy
/// @return STAT_OKE on success
ReturnCode_t W5500_SetRxPayload(EthernetW5500_t* Ptr, void* Payload, Dword_t N);

/// @brief Retrieves received data from the RX frame payload into an external buffer
/// @param Ptr Pointer to the W5500 management structure
/// @param Payload Pointer to the destination buffer
/// @param N Number of bytes to copy
/// @return STAT_OKE on success
ReturnCode_t W5500_GetRxPayload(EthernetW5500_t* Ptr, void* Payload, Dword_t N);

/// @brief Executes a complete SPI transaction based on pre-configured frames
/// @param Ptr Pointer to the W5500 management structure
/// @param nRW Operation mode for the current transaction
/// @return STAT_OKE on success, error code otherwise
ReturnCode_t W5500_AccessNByte(EthernetW5500_t* Ptr, Byte_t nRW);

/// @brief Read an arbitrary number of bytes from W5500
/// @param Ptr Pointer to EthernetW5500_t object
/// @param Buffer Destination buffer to store received data
/// @param Len Number of bytes to read
/// @return ReturnCode_t Status of the operation
ReturnCode_t W5500_ReadNByte(EthernetW5500_t* Ptr, Byte_t* Buffer, Word_t Len);

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
ReturnCode_t W5500_WriteNByte(EthernetW5500_t* Ptr, Byte_t* Data, Word_t Len);

/// @brief Writes a single byte to a pre-configured register
/// @param Ptr Pointer to the W5500 management structure
/// @param Data 8-bit value to transmit
/// @return STAT_OKE on success
ReturnCode_t W5500_WriteByte(EthernetW5500_t* Ptr, Byte_t Data);

/// @brief Writes two bytes to a pre-configured register
/// @param Ptr Pointer to the W5500 management structure
/// @param Data 16-bit value to transmit (will be converted to Big-Endian)
/// @return STAT_OKE on success
ReturnCode_t W5500_WriteDoubleByte(EthernetW5500_t* Ptr, Word_t Data);

/// @brief Writes four bytes to a pre-configured register
/// @param Ptr Pointer to the W5500 management structure
/// @param Data 32-bit value to transmit (will be converted to Big-Endian)
/// @return STAT_OKE on success
ReturnCode_t W5500_WriteQuartByte(EthernetW5500_t* Ptr, Dword_t Data);

/* COMPOSE FN *******************************************************************************************************************/

/// @brief Read a byte from a specific W5500 register with thread-safety
/// @param Ptr Pointer to the W5500 controller structure
/// @param BLockSelNum Block select bits (e.g., Common or Socket n)
/// @param RegAddr 16-bit register offset address
/// @return ReturnCode_t containing the read byte, or STAT_ERR_NULL
ReturnCode_t W5500_ReadByteReg(EthernetW5500_t* Ptr, Word_t BLockSelNum, Word_t RegAddr);

/// @brief Write a byte to a specific W5500 register with thread-safety
/// @param Ptr Pointer to the W5500 controller structure
/// @param BLockSelNum Block select bits (e.g., Common or Socket n)
/// @param RegAddr 16-bit register offset address
/// @param ByteValue 8-bit value to write
/// @return ReturnCode_t STAT_OKE if successful, or error status
ReturnCode_t W5500_WriteByteReg(EthernetW5500_t* Ptr, Word_t BLockSelNum, Word_t RegAddr, Byte_t ByteValue);

/// @brief Read two bytes from a specific W5500 register with thread-safety
/// @param Ptr Pointer to the W5500 controller structure
/// @param BLockSelNum Block select bits (e.g., Common or Socket n)
/// @param RegAddr 16-bit register offset address
/// @return ReturnCode_t containing the 16-bit value, or STAT_ERR_NULL
ReturnCode_t W5500_ReadDoubleByteReg(EthernetW5500_t* Ptr, Word_t BLockSelNum, Word_t RegAddr);

/// @brief Write two bytes to a specific W5500 register with thread-safety
/// @param Ptr Pointer to the W5500 controller structure
/// @param BLockSelNum Block select bits (e.g., Common or Socket n)
/// @param RegAddr 16-bit register offset address
/// @param Data 16-bit value to write
/// @return ReturnCode_t STAT_OKE if successful, or error status
ReturnCode_t W5500_WriteDoubleByteReg(EthernetW5500_t* Ptr, Word_t BLockSelNum, Word_t RegAddr, Word_t Data);

/// @brief Read four bytes from a specific W5500 register with thread-safety
/// @param Ptr Pointer to the W5500 controller structure
/// @param BLockSelNum Block select bits (e.g., Common or Socket n)
/// @param RegAddr 16-bit register offset address
/// @return ReturnCode_t containing the 32-bit value (Dword_t), or STAT_ERR_NULL
ReturnCode_t W5500_ReadQuartByteReg(EthernetW5500_t* Ptr, Word_t BLockSelNum, Word_t RegAddr);

/// @brief Write four bytes to a specific W5500 register with thread-safety
/// @param Ptr Pointer to the W5500 controller structure
/// @param BLockSelNum Block select bits (e.g., Common or Socket n)
/// @param RegAddr 16-bit register offset address
/// @param Data 32-bit value (Dword_t) to write
/// @return ReturnCode_t STAT_OKE if successful, or error status
ReturnCode_t W5500_WriteQuartByteReg(EthernetW5500_t* Ptr, Word_t BLockSelNum, Word_t RegAddr, Dword_t Data);

/// @brief Read N bytes from a specific W5500 block/address with thread-safety
/// @param Ptr Pointer to the W5500 controller structure
/// @param BLockSelNum Block select bits
/// @param RegAddr 16-bit register offset address
/// @param Buffer Destination buffer
/// @param Len Number of bytes to read
/// @return ReturnCode_t STAT_OKE if successful
ReturnCode_t W5500_ReadNByteReg(EthernetW5500_t* Ptr, Word_t BLockSelNum, Word_t RegAddr, Byte_t* Buffer, Word_t Len);

/// @brief Write N bytes to a specific W5500 block/address with thread-safety
/// @param Ptr Pointer to the W5500 controller structure
/// @param BLockSelNum Block select bits
/// @param RegAddr 16-bit register offset address
/// @param Data Source data buffer
/// @param Len Number of bytes to write
/// @return ReturnCode_t STAT_OKE if successful
ReturnCode_t W5500_WriteNByteReg(EthernetW5500_t* Ptr, Word_t BLockSelNum, Word_t RegAddr, Byte_t* Data, Word_t Len);

/// @brief Forcefully clears the Socket 0 receive buffer by syncing pointers
/// @param Ptr Pointer to the W5500 controller structure
/// @return ReturnCode_t STAT_OKE if successful, or error code
ReturnCode_t W5500_ClearRxBuffer(EthernetW5500_t* Ptr);

/// @brief Read the hardware version code from VERSIONR (0x0039)
/// @param Ptr Pointer to the W5500 controller structure
/// @return STAT_OKE if successful, STAT_ERR_NULL if pointer is invalid
ReturnCode_t W5500_GetModuleVersion(EthernetW5500_t* Ptr);

#endif /// __ETHERNET_W5500_H__

/* EOF **************************************************************************************************************************/
