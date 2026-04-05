#ifndef __ETHERNET_W5500_H__
#define __ETHERNET_W5500_H__

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
    eW5500_Read = 1
};

enum eW5500SPIOpModeCmd {
    eW5500_VDM  = 0,
    eW5500_FDM1 = 1,
    eW5500_FDM2 = 2,
    eW5500_FDM4 = 3,
};

#endif /// __ETHERNET_W5500_H__