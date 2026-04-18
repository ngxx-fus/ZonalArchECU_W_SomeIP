#ifndef __ETHERNET_W5500_CMDS_H__
#define __ETHERNET_W5500_CMDS_H__

#include <stdint.h>

/* --- SPI Control Phase Definitions --- */

/** * @brief BSB[4:0] (Block Select Bits) 
 * Used to select the register or buffer block during the SPI Control Phase.
 */
enum eW5500SPIBlockSelectBitsCmd {
    eW5500_SelectsCommonRegister        = 0x00, /* 00000: Common Register */
    eW5500_SelectsSocket0Register       = 0x01, /* 00001: Socket 0 Register */
    eW5500_SelectsSocket0TXBuffer       = 0x02, /* 00010: Socket 0 TX Buffer */
    eW5500_SelectsSocket0RXBuffer       = 0x03, /* 00011: Socket 0 RX Buffer */
    eW5500_SelectsSocket1Register       = 0x05, /* 00101: Socket 1 Register */
    eW5500_SelectsSocket1TXBuffer       = 0x06, /* 00110: Socket 1 TX Buffer */
    eW5500_SelectsSocket1RXBuffer       = 0x07, /* 00111: Socket 1 RX Buffer */
    eW5500_SelectsSocket2Register       = 0x09, /* 01001: Socket 2 Register */
    eW5500_SelectsSocket2TXBuffer       = 0x0A, /* 01010: Socket 2 TX Buffer */
    eW5500_SelectsSocket2RXBuffer       = 0x0B, /* 01011: Socket 2 RX Buffer */
    eW5500_SelectsSocket3Register       = 0x0D, /* 01101: Socket 3 Register */
    eW5500_SelectsSocket3TXBuffer       = 0x0E, 
    eW5500_SelectsSocket3RXBuffer       = 0x0F,
    eW5500_SelectsSocket4Register       = 0x11, /* 10001: Socket 4 Register */
    eW5500_SelectsSocket4TXBuffer       = 0x12, 
    eW5500_SelectsSocket4RXBuffer       = 0x13,
    eW5500_SelectsSocket5Register       = 0x15, /* 10101: Socket 5 Register */
    eW5500_SelectsSocket5TXBuffer       = 0x16, 
    eW5500_SelectsSocket5RXBuffer       = 0x17,
    eW5500_SelectsSocket6Register       = 0x19, /* 11001: Socket 6 Register */
    eW5500_SelectsSocket6TXBuffer       = 0x1A, 
    eW5500_SelectsSocket6RXBuffer       = 0x1B,
    eW5500_SelectsSocket7Register       = 0x1D, /* 11101: Socket 7 Register */
    eW5500_SelectsSocket7TXBuffer       = 0x1E, 
    eW5500_SelectsSocket7RXBuffer       = 0x1F
};

/** * @brief RWB (Read/Write Access Block)
 * Specifies if the current SPI frame is a Read or Write operation.
 */
enum eW5500SPIReadWriteCmd {
    eW5500_Read  = 0, /* Read access */
    eW5500_Write = 1  /* Write access */
};

/** * @brief OM[1:0] (SPI Operation Mode)
 * Defines the data length for the SPI frame.
 */
enum eW5500SPIOpModeCmd {
    eW5500_VDM  = 0, /* 00: Variable Data Length Mode */
    eW5500_FDM1 = 1, /* 01: Fixed Data Length Mode (1 Byte) */
    eW5500_FDM2 = 2, /* 10: Fixed Data Length Mode (2 Bytes) */
    eW5500_FDM4 = 3  /* 11: Fixed Data Length Mode (4 Bytes) */
};

/* --- Common Register Offsets (Block 0x00) --- */

enum eW5500CommonRegisterOffset {
    eW5500_ModeMR           = 0x0000, /* MR (Mode Register) [R/W] [0x0000] [0x00] */
    eW5500_GWAddr0          = 0x0001, /* GAR (Gateway IP Address Register) [R/W] [0x0001-0x0004] [0x00000000] */
    eW5500_SubnetMask0      = 0x0005, /* SUBR (Subnet Mask Register) [R/W] [0x0005-0x0008] [0x00000000] */
    eW5500_SrcMACAddr0      = 0x0009, /* SHAR (Source Hardware Address Register) [R/W] [0x0009-0x000E] [00:00:00:00:00:00] */
    eW5500_SrcIPAddr0       = 0x000F, /* SIPR (Source IP Address Register) [R/W] [0x000F-0x0012] [0x00000000] */
    eW5500_IntLowLevel0     = 0x0013, /* INTLEVEL (Interrupt Low Level Timer Register) [R/W] [0x0013-0x0014] [0x0000] */
    eW5500_IntReg           = 0x0015, /* IR (Interrupt Register) [R/W] [0x0015] [0x00] */
    eW5500_IntMaskReg       = 0x0016, /* IMR (Interrupt Mask Register) [R/W] [0x0016] [0x00] */
    eW5500_SocketIntReg     = 0x0017, /* SIR (Socket Interrupt Register) [R] [0x0017] [0x00] */
    eW5500_SocketIntMaskReg = 0x0018, /* SIMR (Socket Interrupt Mask Register) [R/W] [0x0018] [0x00] */
    eW5500_RetryTime0       = 0x0019, /* RTR (Retry Time-value Register) [R/W] [0x0019-0x001A] [0x07D0] */
    eW5500_RetryCount       = 0x001B, /* RCR (Retry Count Register) [R/W] [0x001B] [0x08] */
    eW5500_PPPEchoTimer     = 0x001C, /* PTIMER (PPP Link Control Protocol Request Timer Register) [R/W] [0x001C] [0x0028] */
    eW5500_PhyConfigReg     = 0x002E, /* PHYCFGR (PHY Configuration Register) [R/W] [0x002E] [0xBF] */
    eW5500_VersionReg       = 0x0039  /* VERSIONR (Chip Version Register) [R] [0x0039] [0x04] */
};

/* --- Register Bit-field Unions --- */

/**
 * @brief MR (Mode Register) [R/W] [0x0000] [0x00] 
 * Used for W5500 reset, Ping block, and PPPoE mode configuration.
 */
typedef union W5500_Reg_MR {
    uint8_t Byte;
    struct __attribute__((packed)) {
        uint8_t Reserved0 : 1; 
        uint8_t FARP      : 1; /* Force ARP: 0-Disable, 1-Enable */
        uint8_t Reserved1 : 1; 
        uint8_t PPPOE     : 1; /* PPPoE Mode: 0-Disable, 1-Enable */
        uint8_t PB        : 1; /* Ping Block Mode: 0-Disable, 1-Enable */
        uint8_t WOL       : 1; /* Wake on LAN: 0-Disable, 1-Enable */
        uint8_t Reserved2 : 1; 
        uint8_t RST       : 1; /* Software Reset: 0-Normal, 1-Reset */
    };
} W5500_Reg_MR;

/**
 * @brief IR (Interrupt Register) [R/W] [0x0015] [0x00]
 * Indicates the source of system interrupts like IP conflict or destination unreachable.
 */
typedef union W5500_Reg_IR {
    uint8_t Byte;
    struct __attribute__((packed)) {
        uint8_t Reserved : 4; 
        uint8_t MP       : 1; /* Magic Packet: Received MP when WOL is enabled */
        uint8_t PPPoE    : 1; /* PPPoE Close interrupt */
        uint8_t UNREACH  : 1; /* Destination Unreachable */
        uint8_t CONFLICT : 1; /* IP Conflict: Source IP is same as another */
    };
} W5500_Reg_IR;

/**
 * @brief PHYCFGR (PHY Configuration Register) [R/W] [0x002E] [0xBF]
 * Configures PHY operation mode and displays link status.
 */
typedef union W5500_Reg_PHYCFGR {
    uint8_t Byte;
    struct __attribute__((packed)) {
        uint8_t LNK   : 1; /* Link status: 0-Down, 1-Up (Read Only) */
        uint8_t SPD   : 1; /* Speed status: 0-10Mbps, 1-100Mbps (Read Only) */
        uint8_t DPX   : 1; /* Duplex status: 0-Half, 1-Full (Read Only) */
        uint8_t OPMDC : 3; /* Operation Mode Config Bits */
        uint8_t OPMD  : 1; /* Operation Mode: 0-H/W, 1-S/W */
        uint8_t RST   : 1; /* PHY Reset: 0-Reset, 1-Normal */
    };
} W5500_Reg_PHYCFGR;

/**
 * @brief Sn_MR (Socket n Mode Register) [R/W] [Offset 0x0000] [0x00]
 * Configures the protocol type and options for Socket n.
 */
typedef union W5500_Reg_Sn_MR {
    uint8_t Byte;
    struct __attribute__((packed)) {
        uint8_t Protocol : 4; /* Socket Protocol bits P[3:0] (TCP, UDP, etc.) */
        uint8_t UCASTB   : 1; /* Unicast Blocking in UDP mode */
        uint8_t ND       : 1; /* No Delayed ACK in TCP mode */
        uint8_t BCASTB   : 1; /* Broadcast Blocking in UDP mode */
        uint8_t MULTI    : 1; /* Multicasting in UDP mode */
    };
} W5500_Reg_Sn_MR;

/**
 * @brief Sn_IR (Socket n Interrupt Register) [R/W] [Offset 0x0002] [0x00]
 * Indicates socket-specific interrupt events like received data or connection established.
 */
typedef union W5500_Reg_Sn_IR {
    uint8_t Byte;
    struct __attribute__((packed)) {
        uint8_t CON      : 1; /* Connection successful */
        uint8_t DISCON   : 1; /* Disconnect occurred */
        uint8_t RECV     : 1; /* Data received */
        uint8_t TIMEOUT  : 1; /* Transmission Timeout */
        uint8_t SEND_OK  : 1; /* SEND command completed */
        uint8_t Reserved : 3;
    };
} W5500_Reg_Sn_IR;

/* --- Main Unified Union --- */

/// @brief Unified union for accessing any 1-byte W5500 register
typedef union W5500_CommonReg {
    uint8_t           Byte;
    W5500_Reg_MR      MR;       
    W5500_Reg_IR      IR;       
    W5500_Reg_PHYCFGR PHYCFGR;  
    W5500_Reg_Sn_MR   Sn_MR;    
    W5500_Reg_Sn_IR   Sn_IR;    
    uint8_t           Sn_SR;    /* Sn_SR (Socket n Status Register) [R] [Offset 0x0003] [0x00] */
} W5500_CommonReg;

#endif /// __ETHERNET_W5500_CMDS_H__