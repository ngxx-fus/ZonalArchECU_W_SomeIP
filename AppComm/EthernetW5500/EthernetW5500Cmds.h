#ifndef __ETHERNET_W5500_CMDS_H__
#define __ETHERNET_W5500_CMDS_H__

enum eW5500SPIBlockSelectBitsCmd {
    eW5500_SelectsCommonRegister        = 0,
    eW5500_SelectsSocket0Register       = 1,
    eW5500_SelectsSocket0TXBuffer       = 2,
    eW5500_SelectsSocket0RXBuffer       = 3,
    eW5500_Reserved_0                   = 4,
    eW5500_SelectsSocket1Register       = 5,
    eW5500_SelectsSocket1TXBuffer       = 6,
    eW5500_SelectsSocket1RXBuffer       = 7,
    eW5500_Reserved_1                   = 8,
    eW5500_SelectsSocket2Register       = 9,
    eW5500_SelectsSocket2TXBuffer       = 10,
    eW5500_SelectsSocket2RXBuffer       = 11,
    eW5500_Reserved_2                   = 12,
    eW5500_SelectsSocket3Register       = 13,
    eW5500_SelectsSocket3TXBuffer       = 14,
    eW5500_SelectsSocket3RXBuffer       = 15,
    eW5500_Reserved_3                   = 16,
    eW5500_SelectsSocket4Register       = 17,
    eW5500_SelectsSocket4TXBuffer       = 18,
    eW5500_SelectsSocket4RXBuffer       = 19,
    eW5500_Reserved_4                   = 20,
    eW5500_SelectsSocket5Register       = 21,
    eW5500_SelectsSocket5TXBuffer       = 22,
    eW5500_SelectsSocket5RXBuffer       = 23,
    eW5500_Reserved_5                   = 24,
    eW5500_SelectsSocket6Register       = 25,
    eW5500_SelectsSocket6TXBuffer       = 26,
    eW5500_SelectsSocket6RXBuffer       = 27,
    eW5500_Reserved_6                   = 28,
    eW5500_SelectsSocket7Register       = 29,
    eW5500_SelectsSocket7TXBuffer       = 30,
    eW5500_SelectsSocket7RXBuffer       = 31
};

enum eW5500SPIReadWriteCmd {
    eW5500_Read = 0,
    eW5500_Write = 1
};

enum eW5500SPIOpModeCmd {
    eW5500_VDM  = 0,
    eW5500_FDM1 = 1,
    eW5500_FDM2 = 2,
    eW5500_FDM4 = 3,
};

enum eW5500CommonRegisterAddress {
    eW5500_CommonRegister    = 0x0,
};

enum eW5500CommonRegisterOffset {
    eW5500_ModeMR           = 0x0000,
    /* Gateway IP Address (4 Bytes) */
    eW5500_GWAddr0          = 0x0001,
    eW5500_GWAddr1          = 0x0002,
    eW5500_GWAddr2          = 0x0003,
    eW5500_GWAddr3          = 0x0004,
    /* Subnet Mask Address (4 Bytes) */
    eW5500_SubnetMask0      = 0x0005, 
    eW5500_SubnetMask1      = 0x0006,
    eW5500_SubnetMask2      = 0x0007,
    eW5500_SubnetMask3      = 0x0008,
    /* Source Hardware Address / MAC (6 Bytes) */
    eW5500_SrcMACAddr0      = 0x0009,
    eW5500_SrcMACAddr1      = 0x000A,
    eW5500_SrcMACAddr2      = 0x000B,
    eW5500_SrcMACAddr3      = 0x000C,
    eW5500_SrcMACAddr4      = 0x000D,
    eW5500_SrcMACAddr5      = 0x000E,
    /* Source IP Address (4 Bytes) */
    eW5500_SrcIPAddr0       = 0x000F,
    eW5500_SrcIPAddr1       = 0x0010,
    eW5500_SrcIPAddr2       = 0x0011,
    eW5500_SrcIPAddr3       = 0x0012,
    /* Interrupt Low Level Timer (2 Bytes) */
    eW5500_IntLowLevel0     = 0x0013,
    eW5500_IntLowLevel1     = 0x0014,
    /* Interrupt Registers */
    eW5500_IntReg           = 0x0015, /* IR */
    eW5500_IntMaskReg       = 0x0016, /* IMR */
    eW5500_SocketIntReg     = 0x0017, /* SIR */
    eW5500_SocketIntMaskReg = 0x0018, /* SIMR */
    /* Retry Time-out and Count */
    eW5500_RetryTime0       = 0x0019, /* RTR (2 Bytes) */
    eW5500_RetryTime1       = 0x001A,
    eW5500_RetryCount       = 0x001B, /* RCR */
    /* PPP LCP Registers */
    eW5500_PPPEchoTimer     = 0x001C, /* PTIMER */
    eW5500_PPPMagicNumber   = 0x001D, /* PMAGIC */
    eW5500_PPPDestMACAddr0  = 0x001E, /* PHAR (6 Bytes) */
    eW5500_PPPDestMACAddr1  = 0x001F,
    eW5500_PPPDestMACAddr2  = 0x0020,
    eW5500_PPPDestMACAddr3  = 0x0021,
    eW5500_PPPDestMACAddr4  = 0x0022,
    eW5500_PPPDestMACAddr5  = 0x0023,
    eW5500_PPPSessionID0    = 0x0024, /* PSID (2 Bytes) */
    eW5500_PPPSessionID1    = 0x0025,
    eW5500_PPPMaxRecvUnit0  = 0x0026, /* PMRU (2 Bytes) */
    eW5500_PPPMaxRecvUnit1  = 0x0027,
    /* Unreachable IP and Port (Used in UDP) */
    eW5500_UnreachIPAddr0   = 0x0028, /* UIPR (4 Bytes) */
    eW5500_UnreachIPAddr1   = 0x0029,
    eW5500_UnreachIPAddr2   = 0x002A,
    eW5500_UnreachIPAddr3   = 0x002B,
    eW5500_UnreachPort0     = 0x002C, /* UPORT (2 Bytes) */
    eW5500_UnreachPort1     = 0x002D,
    /* PHY and Version Registers */
    eW5500_PhyConfigReg     = 0x002E, /* PHYCFGR */
    eW5500_VersionReg       = 0x0039  /* VERSIONR (Read Only: 0x04) */
};

#endif /// __ETHERNET_W5500_H__