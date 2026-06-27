/* AppFrame.h */
#ifndef __APP_BASE_UDP_FRAME_DEF_H__
#define __APP_BASE_UDP_FRAME_DEF_H__

#pragma once

#include "stdint.h"
#include "stdarg.h"
#include "ReturnType.h"
#include "NodeAuth.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ZECU_FRAME_VERSION               (0x04U)
#define ZECU_CRC_POLYNOMIAL              (0xEDB88320U)
#define ZECU_FRAME_HEADER_MAGIC_BYTE     (0xAAU)
#define ZECU_FRAME_HEADER_MAGIC_DWORD    (0xAAAAAAAAU)
#define ZECU_FRAME_SERVICE_NUM_MAX       (8U)
#define ZECU_FRAME_SERVICE_NUM           (6U)

/* Safely concatenate two 8-bit values into a 16-bit unsigned integer */
#define CONCAT16(ByteHigh, ByteLow)      ( (uint16_t)( ((uint16_t)(ByteHigh) << 8) | (uint8_t)(ByteLow) ) )

/*
 * Represents the complete ZECU Frame Header.
 * Uses exact-width integer types for safe cross-platform network transmission.
 * Total size: 84 bytes (4-byte aligned).
 */
typedef struct __attribute__((packed)) {
    uint8_t             MagicByte0;         /* 0xAA */
    uint8_t             Version;
    uint8_t             MagicByte1;
    uint8_t             MagicByte2;
    
    struct __attribute__((packed)) {
        char            Label[64];
        uint8_t         MAC[6];             /* 48-bit MAC Address using byte array */
        uint8_t         MagicByte3;
        uint8_t         IPv4Address[4];
        uint16_t        IPv4Port;           /* 16-bit Port */
        uint8_t         MagicByte4;
        uint8_t         MagicByte5;
        uint8_t         MagicByte6;         /* Retains the 80-byte inner size */
    } Info;
    uint32_t            FrameType;
} ZECUFrame_Header_t;

/* Define Service Categories (High Byte) */
enum ZECUFrame_Service_ByteHigh_e {
    eZECU_Service_MotorControlLeft      = 0x8A,
    eZECU_Service_MotorControlRight     = 0x8B,
    eZECU_Service_SensorDistance0       = 0x8C,
    eZECU_Service_SensorDistance1       = 0x8D,
    eZECU_Service_SensorDistance2       = 0x8E,
    eZECU_Service_GPSLocation           = 0x4A,
};

/* Define Service Synchronization Types (Low Byte) */
enum ZECUFrame_Service_ByteLow_e {
    eZECU_Sync_Event                    = 0x3A,
    eZECU_Sync_Cyclic                   = 0x3B,
    eZECU_Sync_Mixed                    = 0x3C,
};

/* Define Service Access Rights */
enum ZECUFrame_ServiceAccess_e {
    eZECU_Access_Read                   = 0x72,
    eZECU_Access_Write                  = 0x74,
    eZECU_Access_ReadWrite              = 0x76,
};

/* Defined Frame Types */
enum ZECU_FrameType_e {
    eFrameType_Broadcast                = 0xFFFFFFFF,
    eFrameType_PairingRequest           = 0x55000001,
    eFrameType_AuthTX                   = 0xAA00000A,
    eFrameType_AuthRX                   = 0xAA00000B,
    eFrameType_AuthFail                 = 0xAA00000C,
    eFrameType_KeepAlive                = 0x5500AA00,
    eFrameType_HeartBeat                = 0xFF00FF0A,
    eFrameType_EngineControl1           = 0xFF00CC0A,
    eFrameType_EngineControl2           = 0xFF00CC0B,
    eFrameType_EngineControl3           = 0xFF00CC0C,
    eFrameType_EmergencyStop            = 0xF0F0F0FA
};

enum ZECU_PairingRequestID_e{
    ePairingRequest_New                 = 0xFFCC01E,
    ePairingRequest_Free                = 0xEEAA7BF,
};

/*
 * Structure describing a ZECU Service.
 * Total size is exactly 64 bytes for cache and memory alignment optimization.
 */
typedef struct __attribute__((packed)) {
    uint16_t    ID;         ///< 16-bit Service ID
    uint16_t    Type;       ///< 16-bit concatenated Type (High: Category, Low: Sync)
    uint8_t     Access;     ///< 8-bit Access Rights
    char        Desc[59];   ///< 59-byte description string
} ZECUFrame_Service_t;

/*
 * Trailer to verify packet integrity.
 * Total size: 8 bytes.
 */
typedef struct __attribute__((packed)) {
    uint8_t              MagicByte0;
    uint16_t             CheckSum;
    uint8_t              MagicByte1;
    uint32_t             CRC;
} ZECUFrame_Trailer_t;

/* --- BODY STRUCTURES (All padded to 4-byte boundaries) --- */

/* Total size: 520 bytes */
typedef struct __attribute__((packed)) {
    uint32_t            MagicDWord0;    
    uint8_t             ServiceCount;   
    uint8_t             _Padding[3];    
    ZECUFrame_Service_t Services[ZECU_FRAME_SERVICE_NUM_MAX]; 
} ZECUFrame_Body_Broadcast_t;

/* Total size: 8 bytes */
typedef struct __attribute__((packed)) {
    uint8_t            Magic0;
    uint32_t           Request;
    uint8_t            Magic1;
    uint8_t            Magic2;
    uint8_t            Magic3;
} ZECUFrame_Body_PairingRequest_t;

/* Total size: 12 bytes */
typedef struct __attribute__((packed)) {
    uint8_t            Magic0;
    uint64_t           Challenge;
    uint8_t            Magic1;
    uint8_t            Magic2;
    uint8_t            Magic3;
} ZECUFrame_Body_AuthTX_t;

/* Total size: 12 bytes */
typedef struct __attribute__((packed)) {
    uint8_t            Magic0;
    uint64_t           Signature;
    uint8_t            Magic1;
    uint8_t            Magic2;
    uint8_t            Magic3;
} ZECUFrame_Body_AuthRX_t;

/* Total size: 8 bytes */
typedef struct __attribute__((packed)) {
    uint8_t            Magic0;
    uint32_t           ReasonID;
    uint8_t            Magic1;
    uint8_t            Magic2;
    uint8_t            Magic3;
} ZECUFrame_Body_AuthFail_t;

/* Total size: 24 bytes (Padded +2 bytes) */
typedef struct __attribute__((packed)) {
    uint8_t            Magic0;
    uint8_t            SyncNum;
    uint8_t            Magic1;
    uint16_t           D0;
    uint16_t           D1;
    uint16_t           D2;
    uint8_t            Magic2;
    int8_t             M0;
    int8_t             M1;
    uint8_t            Magic3;
    uint32_t           Location_Long;
    uint32_t           Location_Lat;
    uint8_t            Magic4;
    uint8_t            Magic5;
    uint8_t            Magic6;
} ZECUFrame_Body_HeartBeat_t;

/* Total size: 8 bytes (Padded +2 bytes) */
typedef struct __attribute__((packed)) {
    uint8_t            Magic0;
    uint8_t            SyncCtrlNum;
    uint8_t            Magic1;
    int8_t             M0;
    uint8_t            Magic2;
    uint8_t            Magic3;
    uint16_t           CheckSum16;
} ZECUFrame_Body_EngineControl1_t;

/* Total size: 8 bytes (Padded +2 bytes) */
typedef struct __attribute__((packed)) {
    uint8_t            Magic0;
    uint8_t            SyncCtrlNum;
    uint8_t            Magic1;
    int8_t             M1;
    uint8_t            Magic2;
    uint8_t            Magic3;
    uint16_t           CheckSum16;
} ZECUFrame_Body_EngineControl2_t;

/* Total size: 8 bytes */
typedef struct __attribute__((packed)) {
    uint8_t            Magic0;
    uint8_t            SyncCtrlNum;
    uint8_t            Magic1;
    int8_t             M0;
    uint8_t            Magic2;
    int8_t             M1;
    uint16_t           CheckSum16;
} ZECUFrame_Body_EngineControl3_t;

/* Total size: 28 bytes (Padded +1 byte) */
typedef struct __attribute__((packed)) {
    uint8_t            Magic0;
    uint32_t           ReasonID;
    uint8_t            Magic1;
    uint16_t           D0;
    uint16_t           D1;
    uint16_t           D2;
    uint8_t            Magic2;
    int8_t             M0;
    int8_t             M1;
    uint8_t            Magic3;
    uint32_t           Location_Long;
    uint32_t           Location_Lat;
    uint8_t            Magic4;
    uint8_t            Magic5;
    uint16_t           CheckSum16;
} ZECUFrame_Body_EmergencyStop_t;

/* Total size: 28 bytes (Padded +1 byte) */
typedef struct __attribute__((packed)) {
    uint8_t            Magic0;
    uint8_t            SyncNum0;
    uint8_t            Magic1;
    uint8_t            Magic2;
} ZECUFrame_Body_KeepAlive_t;

/*
 * Union of all Body structures.
 * Its size is inherently determined by the largest member (Broadcast_t: 520 bytes).
 */
typedef union {
    ZECUFrame_Body_Broadcast_t       Broadcast;
    ZECUFrame_Body_PairingRequest_t  PairingRequest;
    ZECUFrame_Body_AuthTX_t          AuthTX;
    ZECUFrame_Body_AuthRX_t          AuthRX;
    ZECUFrame_Body_AuthFail_t        AuthFail;
    ZECUFrame_Body_HeartBeat_t       HeartBeat;
    ZECUFrame_Body_EngineControl1_t  EngineControl1;
    ZECUFrame_Body_EngineControl2_t  EngineControl2;
    ZECUFrame_Body_EngineControl3_t  EngineControl3;
    ZECUFrame_Body_EmergencyStop_t   EmergencyStop;
    ZECUFrame_Body_KeepAlive_t       KeepAlive;
} ZECUFrame_Body_t;

/* --- COMPOSITE GENERIC FRAME --- */

/*
 * Generic ZECU Frame encompassing all possible body payloads.
 * Total size: 84 (Header) + 520 (Body Union) + 8 (Trailer) = 612 bytes.
 * Perfectly aligned for generic socket Rx/Tx buffers.
 */
typedef struct __attribute__((packed)) {
    ZECUFrame_Header_t          Header;
    ZECUFrame_Body_t            Body;
    ZECUFrame_Trailer_t         Trailer;
} ZECUFrame_Generic_t;

/* Global vars */
extern ZECUFrame_Service_t ZECUFrame_ServiceList[ZECU_FRAME_SERVICE_NUM];

/* Core Networking Integrity Utilities */
uint16_t App_CalculateChecksum16(const uint8_t* data, uint32_t length, uint16_t initial_sum);
uint32_t App_CalculateCRC32(const uint8_t* data, uint32_t length, uint32_t initial_crc);

/* Standard Frame Processors */
ReturnCode_t App_UpdateTrailer(ZECUFrame_Generic_t* Frame);
ReturnCode_t App_VerifyPacket(const ZECUFrame_Generic_t* Frame);

/* --- TX Frame Generators --- */
ReturnCode_t App_WriteFrame_HeartBeat(ZECUFrame_Generic_t* Frame, uint8_t syncNum, uint16_t d0, uint16_t d1, uint16_t d2, int8_t m0, int8_t m1, uint32_t locLong, uint32_t locLat);
ReturnCode_t App_WriteFrame_EngineControl1(ZECUFrame_Generic_t* Frame, uint8_t syncCtrl, int8_t m0);
ReturnCode_t App_WriteFrame_EngineControl2(ZECUFrame_Generic_t* Frame, uint8_t syncCtrl, int8_t m1);
ReturnCode_t App_WriteFrame_EngineControl3(ZECUFrame_Generic_t* Frame, uint8_t syncCtrl, int8_t m0, int8_t m1);
ReturnCode_t App_WriteFrame_EmergencyStop(ZECUFrame_Generic_t* Frame, uint32_t reason, uint16_t d0, uint16_t d1, uint16_t d2, int8_t m0, int8_t m1, uint32_t locLong, uint32_t locLat);
ReturnCode_t App_WriteFrame_AuthFail(ZECUFrame_Generic_t* Frame, uint32_t reason);

/* AuthTX implicitly generates a Challenge via NodeAuth */
ReturnCode_t App_WriteFrame_AuthTX(ZECUFrame_Generic_t* Frame, uint32_t target_id, uint32_t timestamp);

/* AuthRX implicitly signs a Challenge via NodeAuth */
ReturnCode_t App_WriteFrame_AuthRX(ZECUFrame_Generic_t* Frame, uint64_t challenge);

ReturnCode_t App_WriteFrame_KeepAlive(ZECUFrame_Generic_t* Frame, uint8_t syncNum, uint8_t magic);

/* --- RX Frame Parsers --- */
ReturnCode_t App_ReadFrame_HeartBeat(const ZECUFrame_Generic_t* Frame, uint8_t* syncNum, uint16_t* d0, uint16_t* d1, uint16_t* d2, int8_t* m0, int8_t* m1, uint32_t* locLong, uint32_t* locLat);
ReturnCode_t App_ReadFrame_EngineControl1(const ZECUFrame_Generic_t* Frame, uint8_t* syncCtrl, int8_t* m0);
ReturnCode_t App_ReadFrame_EngineControl2(const ZECUFrame_Generic_t* Frame, uint8_t* syncCtrl, int8_t* m1);
ReturnCode_t App_ReadFrame_EngineControl3(const ZECUFrame_Generic_t* Frame, uint8_t* syncCtrl, int8_t* m0, int8_t* m1);
ReturnCode_t App_ReadFrame_EmergencyStop(const ZECUFrame_Generic_t* Frame, uint32_t* reason, uint16_t* d0, uint16_t* d1, uint16_t* d2, int8_t* m0, int8_t* m1, uint32_t* locLong, uint32_t* locLat);
ReturnCode_t App_ReadFrame_AuthFail(const ZECUFrame_Generic_t* Frame, uint32_t* reason);
ReturnCode_t App_ReadFrame_AuthTX(const ZECUFrame_Generic_t* Frame, uint64_t* challenge);
ReturnCode_t App_ReadFrame_KeepAlive(const ZECUFrame_Generic_t* Frame, uint8_t* syncNum);

/* Verify AuthRX implicitly checks signature via NodeAuth */
ReturnCode_t App_VerifyFrame_AuthRX(const ZECUFrame_Generic_t* Frame, uint64_t original_challenge);

#ifdef __cplusplus
}
#endif

#endif /*__APP_BASE_UDP_FRAME_DEF_H__*/