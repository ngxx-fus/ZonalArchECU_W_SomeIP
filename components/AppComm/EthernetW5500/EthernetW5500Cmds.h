#ifndef __ETHERNET_W5500_CMDS_H__
#define __ETHERNET_W5500_CMDS_H__

#include <stdint.h>

/// @brief @brief RWB (Read/Write Access Block)
enum eW5500SPIReadWriteCmd {
    eW5500_Read  = 0, /* Read access */
    eW5500_Write = 1  /* Write access */
};

/// @brief OM[1:0] (SPI Operation Mode)
enum eW5500SPIOpModeCmd {
    eW5500_VDM  = 0, /* 00: Variable Data Length Mode */
    eW5500_FDM1 = 1, /* 01: Fixed Data Length Mode (1 Byte) */
    eW5500_FDM2 = 2, /* 10: Fixed Data Length Mode (2 Bytes) */
    eW5500_FDM4 = 3  /* 11: Fixed Data Length Mode (4 Bytes) */
};

/// @brief [U: Apr 23] W5500 Block Select Bits (BSB) definitions
enum eW5500_BSB {
    eBSB_CommonRegister  = 0x0000,

    /* For socket 0 */
    eBSB_Socket0Register = 0x0001,
    eBSB_Socket0TxBuffer = 0x0002,
    eBSB_Socket0RxBuffer = 0x0003,
    eBSB_Socket0Reserved = 0x0004,

    /* For socket 1 */
    eBSB_Socket1Register = 0x0005,
    eBSB_Socket1TxBuffer = 0x0006,
    eBSB_Socket1RxBuffer = 0x0007,
    eBSB_Socket1Reserved = 0x0008,

    /* For socket 2 */
    eBSB_Socket2Register = 0x0009,
    eBSB_Socket2TxBuffer = 0x000A,
    eBSB_Socket2RxBuffer = 0x000B,
    eBSB_Socket2Reserved = 0x000C,

    /* For socket 3 */
    eBSB_Socket3Register = 0x000D,
    eBSB_Socket3TxBuffer = 0x000E,
    eBSB_Socket3RxBuffer = 0x000F,
    eBSB_Socket3Reserved = 0x0010,

    /* For socket 4 */
    eBSB_Socket4Register = 0x0011,
    eBSB_Socket4TxBuffer = 0x0012,
    eBSB_Socket4RxBuffer = 0x0013,
    eBSB_Socket4Reserved = 0x0014,

    /* For socket 5 */
    eBSB_Socket5Register = 0x0015,
    eBSB_Socket5TxBuffer = 0x0016,
    eBSB_Socket5RxBuffer = 0x0017,
    eBSB_Socket5Reserved = 0x0018,

    /* For socket 6 */
    eBSB_Socket6Register = 0x0019,
    eBSB_Socket6TxBuffer = 0x001A,
    eBSB_Socket6RxBuffer = 0x001B,
    eBSB_Socket6Reserved = 0x001C,

    /* For socket 7 */
    eBSB_Socket7Register = 0x001D,
    eBSB_Socket7TxBuffer = 0x001E,
    eBSB_Socket7RxBuffer = 0x001F
    /* 0x0020 is out of 5-bit BSB range, so no reserved bit for Socket 7 */
};

/// @brief [U: Apr 23] W5500 Common Register offsets
enum eW5500_CommonRegister {
    eMR         = 0x0000,   ///< Mode Register
    eGAR0       = 0x0001,   ///< Gateway Address 0
    eGAR1       = 0x0002,   ///< Gateway Address 1
    eGAR2       = 0x0003,   ///< Gateway Address 2
    eGAR3       = 0x0004,   ///< Gateway Address 3
    eSUBR0      = 0x0005,   ///< Subnet Mask Address 0
    eSUBR1      = 0x0006,   ///< Subnet Mask Address 1
    eSUBR2      = 0x0007,   ///< Subnet Mask Address 2
    eSUBR3      = 0x0008,   ///< Subnet Mask Address 3
    eSHAR0      = 0x0009,   ///< Source Hardware Address 0
    eSHAR1      = 0x000A,   ///< Source Hardware Address 1
    eSHAR2      = 0x000B,   ///< Source Hardware Address 2
    eSHAR3      = 0x000C,   ///< Source Hardware Address 3
    eSHAR4      = 0x000D,   ///< Source Hardware Address 4
    eSHAR5      = 0x000E,   ///< Source Hardware Address 5
    eSIPR0      = 0x000F,   ///< Source IP Address 0
    eSIPR1      = 0x0010,   ///< Source IP Address 1
    eSIPR2      = 0x0011,   ///< Source IP Address 2
    eSIPR3      = 0x0012,   ///< Source IP Address 3
    eINTLEVEL0  = 0x0013,   ///< Interrupt Low Level Timer 0
    eINTLEVEL1  = 0x0014,   ///< Interrupt Low Level Timer 1
    eIR         = 0x0015,   ///< Interrupt Register
    eIMR        = 0x0016,   ///< Interrupt Mask Register
    eSIR        = 0x0017,   ///< Socket Interrupt Register
    eSIMR       = 0x0018,   ///< Socket Interrupt Mask Register
    eRTR0       = 0x0019,   ///< Retry Time 0
    eRTR1       = 0x001A,   ///< Retry Time 1
    eRCR        = 0x001B,   ///< Retry Count
    ePTIMER     = 0x001C,   ///< PPP LCP Request Timer
    ePMAGIC     = 0x001D,   ///< PPP LCP Magic number
    ePHAR0      = 0x001E,   ///< PPP Destination MAC Address 0
    ePHAR1      = 0x001F,   ///< PPP Destination MAC Address 1
    ePHAR2      = 0x0020,   ///< PPP Destination MAC Address 2
    ePHAR3      = 0x0021,   ///< PPP Destination MAC Address 3
    ePHAR4      = 0x0022,   ///< PPP Destination MAC Address 4
    ePHAR5      = 0x0023,   ///< PPP Destination MAC Address 5
    ePSID0      = 0x0024,   ///< PPP Session Identification 0
    ePSID1      = 0x0025,   ///< PPP Session Identification 1
    ePMRU0      = 0x0026,   ///< PPP Maximum Segment Size 0
    ePMRU1      = 0x0027,   ///< PPP Maximum Segment Size 1
    eUIPR0      = 0x0028,   ///< Unreachable IP address 0
    eUIPR1      = 0x0029,   ///< Unreachable IP address 1
    eUIPR2      = 0x002A,   ///< Unreachable IP address 2
    eUIPR3      = 0x002B,   ///< Unreachable IP address 3
    eUPORTR0    = 0x002C,   ///< Unreachable Port 0
    eUPORTR1    = 0x002D,   ///< Unreachable Port 1
    ePHYCFGR    = 0x002E,   ///< PHY Configuration
    eVERSIONR   = 0x0039    ///< Chip version
};

/// @brief [U: Apr 23] W5500 Socket n Register offsets
enum eW5500_SocketNRegister {
    eSn_MR          = 0x0000,   ///< Socket n Mode
    eSn_CR          = 0x0001,   ///< Socket n Command
    eSn_IR          = 0x0002,   ///< Socket n Interrupt
    eSn_SR          = 0x0003,   ///< Socket n Status
    eSn_PORT0       = 0x0004,   ///< Socket n Source Port 0
    eSn_PORT1       = 0x0005,   ///< Socket n Source Port 1
    eSn_DHAR0       = 0x0006,   ///< Socket n Destination Hardware Address 0
    eSn_DHAR1       = 0x0007,   ///< Socket n Destination Hardware Address 1
    eSn_DHAR2       = 0x0008,   ///< Socket n Destination Hardware Address 2
    eSn_DHAR3       = 0x0009,   ///< Socket n Destination Hardware Address 3
    eSn_DHAR4       = 0x000A,   ///< Socket n Destination Hardware Address 4
    eSn_DHAR5       = 0x000B,   ///< Socket n Destination Hardware Address 5
    eSn_DIPR0       = 0x000C,   ///< Socket n Destination IP Address 0
    eSn_DIPR1       = 0x000D,   ///< Socket n Destination IP Address 1
    eSn_DIPR2       = 0x000E,   ///< Socket n Destination IP Address 2
    eSn_DIPR3       = 0x000F,   ///< Socket n Destination IP Address 3
    eSn_DPORT0      = 0x0010,   ///< Socket n Destination Port 0
    eSn_DPORT1      = 0x0011,   ///< Socket n Destination Port 1
    eSn_MSSR0       = 0x0012,   ///< Socket n Maximum Segment Size 0
    eSn_MSSR1       = 0x0013,   ///< Socket n Maximum Segment Size 1
    eSn_TOS         = 0x0015,   ///< Socket n IP TOS
    eSn_TTL         = 0x0016,   ///< Socket n IP TTL
    eSn_RXBUF_SIZE  = 0x001E,   ///< Socket n Receive Buffer Size
    eSn_TXBUF_SIZE  = 0x001F,   ///< Socket n Transmit Buffer Size
    eSn_TX_FSR0     = 0x0020,   ///< Socket n TX Free Size 0
    eSn_TX_FSR1     = 0x0021,   ///< Socket n TX Free Size 1
    eSn_TX_RD0      = 0x0022,   ///< Socket n TX Read Pointer 0
    eSn_TX_RD1      = 0x0023,   ///< Socket n TX Read Pointer 1
    eSn_TX_WR0      = 0x0024,   ///< Socket n TX Write Pointer 0
    eSn_TX_WR1      = 0x0025,   ///< Socket n TX Write Pointer 1
    eSn_RX_RSR0     = 0x0026,   ///< Socket n RX Received Size 0
    eSn_RX_RSR1     = 0x0027,   ///< Socket n RX Received Size 1
    eSn_RX_RD0      = 0x0028,   ///< Socket n RX Read Pointer 0
    eSn_RX_RD1      = 0x0029,   ///< Socket n RX Read Pointer 1
    eSn_RX_WR0      = 0x002A,   ///< Socket n RX Write Pointer 0
    eSn_RX_WR1      = 0x002B,   ///< Socket n RX Write Pointer 1
    eSn_IMR         = 0x002C,   ///< Socket n Interrupt Mask
    eSn_FRAG0       = 0x002D,   ///< Socket n Fragment Offset in IP header 0
    eSn_FRAG1       = 0x002E,   ///< Socket n Fragment Offset in IP header 1
    eSn_KPALVTR     = 0x002F    ///< Keep alive timer
};


#endif /// __ETHERNET_W5500_CMDS_H__