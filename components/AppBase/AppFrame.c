/* AppFrame.c */
#include "AppFrame.h"
#include <string.h>

/* Global registry of provided ZECU services */
ZECUFrame_Service_t ZECUFrame_ServiceList[ZECU_FRAME_SERVICE_NUM] = {
    [0] = {
        .ID     = (uint16_t)eZECU_Service_MotorControlLeft,
        .Type   = CONCAT16(eZECU_Service_MotorControlLeft, eZECU_Sync_Mixed),
        .Access = (uint8_t)eZECU_Access_ReadWrite,
        .Desc   = "Motor Control Left - Mixed Sync"
    },
    [1] = {
        .ID     = (uint16_t)eZECU_Service_MotorControlRight,
        .Type   = CONCAT16(eZECU_Service_MotorControlRight, eZECU_Sync_Mixed),
        .Access = (uint8_t)eZECU_Access_ReadWrite,
        .Desc   = "Motor Control Right - Mixed Sync"
    },
    [2] = {
        .ID     = (uint16_t)eZECU_Service_SensorDistance0,
        .Type   = CONCAT16(eZECU_Service_SensorDistance0, eZECU_Sync_Cyclic),
        .Access = (uint8_t)eZECU_Access_Read,
        .Desc   = "Distance Sensor 0 - Cyclic Sync"
    },
    [3] = {
        .ID     = (uint16_t)eZECU_Service_SensorDistance1,
        .Type   = CONCAT16(eZECU_Service_SensorDistance1, eZECU_Sync_Cyclic),
        .Access = (uint8_t)eZECU_Access_Read,
        .Desc   = "Distance Sensor 1 - Cyclic Sync"
    },
    [4] = {
        .ID     = (uint16_t)eZECU_Service_SensorDistance2,
        .Type   = CONCAT16(eZECU_Service_SensorDistance2, eZECU_Sync_Cyclic),
        .Access = (uint8_t)eZECU_Access_Read,
        .Desc   = "Distance Sensor 2 - Cyclic Sync"
    },
    [5] = {
        .ID     = (uint16_t)eZECU_Service_GPSLocation,
        .Type   = CONCAT16(eZECU_Service_GPSLocation, eZECU_Sync_Cyclic),
        .Access = (uint8_t)eZECU_Access_Read,
        .Desc   = "GPS Location - Cyclic Sync"
    }
};

/* --- CORE INTEGRITY UTILS --- */

uint16_t App_CalculateChecksum16(const uint8_t* data, uint32_t length, uint16_t initial_sum) {
    uint32_t sum = initial_sum;
    uint32_t i;
    
    /* Control flow: validate data pointer safely */
    if (data == NULL) {
        return initial_sum;
    }

    for (i = 0; i < length; i++) {
        sum += data[i];
    }
    
    /* Control flow: fold 32-bit overflow back into 16-bit */
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return (uint16_t)~sum;
}

uint32_t App_CalculateCRC32(const uint8_t* data, uint32_t length, uint32_t initial_crc) {
    uint32_t crc = initial_crc;
    uint32_t i, j;
    
    /* Control flow: validate data pointer safely */
    if (data == NULL) {
        return initial_crc;
    }

    for (i = 0; i < length; i++) {
        crc ^= (uint32_t)data[i];
        
        /* Control flow: process each bit of the byte */
        for (j = 0; j < 8; j++) {
            /* Control flow: shift and XOR with polynomial if LSB is set */
            if (crc & 1) {
                crc = (crc >> 1) ^ ZECU_CRC_POLYNOMIAL;
            } else {
                crc >>= 1;
            }
        }
    }
    
    return crc;
}

/* --- GENERIC FRAME PROCESSORS --- */

/*
 * Internal helper to format the generic header and clear the body payload
 */
static void App_FormatHeader(ZECUFrame_Generic_t* Frame, uint32_t frame_type) {
    /* Initialize body to zero to avoid transmitting garbage data */
    memset(&Frame->Body, 0, sizeof(ZECUFrame_Body_t));
    
    Frame->Header.MagicByte0 = ZECU_FRAME_HEADER_MAGIC_BYTE;
    Frame->Header.Version    = ZECU_FRAME_VERSION;
    Frame->Header.MagicByte1 = ZECU_FRAME_HEADER_MAGIC_BYTE;
    Frame->Header.MagicByte2 = ZECU_FRAME_HEADER_MAGIC_BYTE;
    Frame->Header.FrameType  = frame_type;
}

ReturnCode_t App_UpdateTrailer(ZECUFrame_Generic_t* Frame) {
    uint32_t payload_size;
    uint16_t checksum;
    uint32_t crc;

    /* Control flow: validate input pointer */
    if (Frame == NULL) {
        return STAT_ERR_NULL;
    }

    /* Target CRC spans exactly 604 bytes (Header 84 + Body 520) */
    payload_size = sizeof(ZECUFrame_Header_t) + sizeof(ZECUFrame_Body_t);

    checksum = App_CalculateChecksum16((const uint8_t*)Frame, payload_size, 0);
    crc = App_CalculateCRC32((const uint8_t*)Frame, payload_size, 0xFFFFFFFF);

    Frame->Trailer.MagicByte0 = ZECU_FRAME_HEADER_MAGIC_BYTE;
    Frame->Trailer.CheckSum   = checksum;
    Frame->Trailer.MagicByte1 = ZECU_FRAME_HEADER_MAGIC_BYTE;
    Frame->Trailer.CRC        = crc ^ 0xFFFFFFFF;

    return STAT_OKE;
}

ReturnCode_t App_VerifyPacket(const ZECUFrame_Generic_t* Frame) {
    uint32_t payload_size;
    uint16_t expected_checksum;
    uint32_t expected_crc;

    /* Control flow: validate input pointer */
    if (Frame == NULL) {
        return STAT_ERR_NULL;
    }

    /* Control flow: verify magic bytes in trailer */
    if ((Frame->Trailer.MagicByte0 != ZECU_FRAME_HEADER_MAGIC_BYTE) ||
        (Frame->Trailer.MagicByte1 != ZECU_FRAME_HEADER_MAGIC_BYTE)) {
        return STAT_ERR_CRC;
    }

    payload_size = sizeof(ZECUFrame_Header_t) + sizeof(ZECUFrame_Body_t);

    expected_checksum = App_CalculateChecksum16((const uint8_t*)Frame, payload_size, 0);
    /* Control flow: compare checksum */
    if (Frame->Trailer.CheckSum != expected_checksum) {
        return STAT_ERR_CRC;
    }

    expected_crc = App_CalculateCRC32((const uint8_t*)Frame, payload_size, 0xFFFFFFFF);
    expected_crc ^= 0xFFFFFFFF;
    
    /* Control flow: compare CRC32 */
    if (Frame->Trailer.CRC != expected_crc) {
        return STAT_ERR_CRC;
    }

    return STAT_OKE;
}

/* --- TX FRAME GENERATORS --- */

ReturnCode_t App_WriteFrame_HeartBeat(ZECUFrame_Generic_t* Frame, uint8_t syncNum, uint16_t d0, uint16_t d1, uint16_t d2, int8_t m0, int8_t m1, uint32_t locLong, uint32_t locLat) {
    /* Control flow: check pointers */
    if (Frame == NULL) return STAT_ERR_NULL;

    App_FormatHeader(Frame, eFrameType_HeartBeat);
    Frame->Body.HeartBeat.Magic0 = ZECU_FRAME_HEADER_MAGIC_BYTE;
    Frame->Body.HeartBeat.SyncNum = syncNum;
    Frame->Body.HeartBeat.D0 = d0;
    Frame->Body.HeartBeat.D1 = d1;
    Frame->Body.HeartBeat.D2 = d2;
    Frame->Body.HeartBeat.M0 = m0;
    Frame->Body.HeartBeat.M1 = m1;
    Frame->Body.HeartBeat.Location_Long = locLong;
    Frame->Body.HeartBeat.Location_Lat = locLat;
    
    return App_UpdateTrailer(Frame);
}

ReturnCode_t App_WriteFrame_EngineControl1(ZECUFrame_Generic_t* Frame, uint8_t syncCtrl, int8_t m0) {
    /* Control flow: check pointers */
    if (Frame == NULL) return STAT_ERR_NULL;

    App_FormatHeader(Frame, eFrameType_EngineControl1);
    Frame->Body.EngineControl1.Magic0 = ZECU_FRAME_HEADER_MAGIC_BYTE;
    Frame->Body.EngineControl1.SyncCtrlNum = syncCtrl;
    Frame->Body.EngineControl1.M0 = m0;
    
    return App_UpdateTrailer(Frame);
}

ReturnCode_t App_WriteFrame_EngineControl2(ZECUFrame_Generic_t* Frame, uint8_t syncCtrl, int8_t m1) {
    /* Control flow: check pointers */
    if (Frame == NULL) return STAT_ERR_NULL;

    App_FormatHeader(Frame, eFrameType_EngineControl2);
    Frame->Body.EngineControl2.Magic0 = ZECU_FRAME_HEADER_MAGIC_BYTE;
    Frame->Body.EngineControl2.SyncCtrlNum = syncCtrl;
    Frame->Body.EngineControl2.M1 = m1;
    
    return App_UpdateTrailer(Frame);
}

ReturnCode_t App_WriteFrame_EngineControl3(ZECUFrame_Generic_t* Frame, uint8_t syncCtrl, int8_t m0, int8_t m1) {
    /* Control flow: check pointers */
    if (Frame == NULL) return STAT_ERR_NULL;

    App_FormatHeader(Frame, eFrameType_EngineControl3);
    Frame->Body.EngineControl3.Magic0 = ZECU_FRAME_HEADER_MAGIC_BYTE;
    Frame->Body.EngineControl3.SyncCtrlNum = syncCtrl;
    Frame->Body.EngineControl3.M0 = m0;
    Frame->Body.EngineControl3.M1 = m1;
    
    return App_UpdateTrailer(Frame);
}

ReturnCode_t App_WriteFrame_EmergencyStop(ZECUFrame_Generic_t* Frame, uint32_t reason, uint16_t d0, uint16_t d1, uint16_t d2, int8_t m0, int8_t m1, uint32_t locLong, uint32_t locLat) {
    /* Control flow: check pointers */
    if (Frame == NULL) return STAT_ERR_NULL;

    App_FormatHeader(Frame, eFrameType_EmergencyStop);
    Frame->Body.EmergencyStop.Magic0 = ZECU_FRAME_HEADER_MAGIC_BYTE;
    Frame->Body.EmergencyStop.ReasonID = reason;
    Frame->Body.EmergencyStop.D0 = d0;
    Frame->Body.EmergencyStop.D1 = d1;
    Frame->Body.EmergencyStop.D2 = d2;
    Frame->Body.EmergencyStop.M0 = m0;
    Frame->Body.EmergencyStop.M1 = m1;
    Frame->Body.EmergencyStop.Location_Long = locLong;
    Frame->Body.EmergencyStop.Location_Lat = locLat;
    
    return App_UpdateTrailer(Frame);
}

ReturnCode_t App_WriteFrame_AuthFail(ZECUFrame_Generic_t* Frame, uint32_t reason) {
    /* Control flow: check pointers */
    if (Frame == NULL) return STAT_ERR_NULL;

    App_FormatHeader(Frame, eFrameType_AuthFail);
    Frame->Body.AuthFail.Magic0 = ZECU_FRAME_HEADER_MAGIC_BYTE;
    Frame->Body.AuthFail.ReasonID = reason;
    
    return App_UpdateTrailer(Frame);
}

ReturnCode_t App_WriteFrame_AuthTX(ZECUFrame_Generic_t* Frame, uint32_t target_id, uint32_t timestamp) {
    uint64_t challenge = 0;
    DefaultRet_t auth_stat;
    
    /* Control flow: check pointers */
    if (Frame == NULL) return STAT_ERR_NULL;
    
    auth_stat = Auth_GenerateChallenge(target_id, timestamp, &challenge);
    /* Control flow: ensure node auth generation succeeds */
    if (auth_stat != STAT_OKE) {
        return auth_stat;
    }

    App_FormatHeader(Frame, eFrameType_AuthTX);
    Frame->Body.AuthTX.Magic0 = ZECU_FRAME_HEADER_MAGIC_BYTE;
    Frame->Body.AuthTX.Challenge = challenge;
    
    return App_UpdateTrailer(Frame);
}

ReturnCode_t App_WriteFrame_AuthRX(ZECUFrame_Generic_t* Frame, uint64_t challenge) {
    uint64_t signature = 0;
    DefaultRet_t auth_stat;
    
    /* Control flow: check pointers */
    if (Frame == NULL) return STAT_ERR_NULL;
    
    auth_stat = Auth_SignChallenge(challenge, &signature);
    /* Control flow: ensure node auth signature creation succeeds */
    if (auth_stat != STAT_OKE) {
        return auth_stat;
    }

    App_FormatHeader(Frame, eFrameType_AuthRX);
    Frame->Body.AuthRX.Magic0 = ZECU_FRAME_HEADER_MAGIC_BYTE;
    Frame->Body.AuthRX.Signature = signature;
    
    return App_UpdateTrailer(Frame);
}

ReturnCode_t App_WriteFrame_KeepAlive(ZECUFrame_Generic_t* Frame, uint8_t syncNum, uint8_t magic) {
    /* Control flow: check pointers */
    if (Frame == NULL) return STAT_ERR_NULL;

    App_FormatHeader(Frame, eFrameType_KeepAlive);
    Frame->Body.KeepAlive.Magic0 = magic;
    Frame->Body.KeepAlive.SyncNum0 = syncNum;
    Frame->Body.KeepAlive.Magic1 = magic;
    Frame->Body.KeepAlive.Magic2 = magic;
    
    return App_UpdateTrailer(Frame);
}

/* --- RX FRAME PARSERS --- */

ReturnCode_t App_ReadFrame_HeartBeat(const ZECUFrame_Generic_t* Frame, uint8_t* syncNum, uint16_t* d0, uint16_t* d1, uint16_t* d2, int8_t* m0, int8_t* m1, uint32_t* locLong, uint32_t* locLat) {
    /* Control flow: validate input frame structure */
    if (App_VerifyPacket(Frame) != STAT_OKE) return STAT_ERR_CRC;
    
    /* Control flow: verify frame type */
    if (Frame->Header.FrameType != eFrameType_HeartBeat) return STAT_ERR_INVALID_ARG;

    if (syncNum) *syncNum = Frame->Body.HeartBeat.SyncNum;
    if (d0) *d0 = Frame->Body.HeartBeat.D0;
    if (d1) *d1 = Frame->Body.HeartBeat.D1;
    if (d2) *d2 = Frame->Body.HeartBeat.D2;
    if (m0) *m0 = Frame->Body.HeartBeat.M0;
    if (m1) *m1 = Frame->Body.HeartBeat.M1;
    if (locLong) *locLong = Frame->Body.HeartBeat.Location_Long;
    if (locLat) *locLat = Frame->Body.HeartBeat.Location_Lat;

    return STAT_OKE;
}

ReturnCode_t App_ReadFrame_EngineControl1(const ZECUFrame_Generic_t* Frame, uint8_t* syncCtrl, int8_t* m0) {
    /* Control flow: validate input frame structure */
    if (App_VerifyPacket(Frame) != STAT_OKE) return STAT_ERR_CRC;
    
    /* Control flow: verify frame type */
    if (Frame->Header.FrameType != eFrameType_EngineControl1) return STAT_ERR_INVALID_ARG;

    if (syncCtrl) *syncCtrl = Frame->Body.EngineControl1.SyncCtrlNum;
    if (m0) *m0 = Frame->Body.EngineControl1.M0;

    return STAT_OKE;
}

ReturnCode_t App_ReadFrame_EngineControl2(const ZECUFrame_Generic_t* Frame, uint8_t* syncCtrl, int8_t* m1) {
    /* Control flow: validate input frame structure */
    if (App_VerifyPacket(Frame) != STAT_OKE) return STAT_ERR_CRC;
    
    /* Control flow: verify frame type */
    if (Frame->Header.FrameType != eFrameType_EngineControl2) return STAT_ERR_INVALID_ARG;

    if (syncCtrl) *syncCtrl = Frame->Body.EngineControl2.SyncCtrlNum;
    if (m1) *m1 = Frame->Body.EngineControl2.M1;

    return STAT_OKE;
}

ReturnCode_t App_ReadFrame_EngineControl3(const ZECUFrame_Generic_t* Frame, uint8_t* syncCtrl, int8_t* m0, int8_t* m1) {
    /* Control flow: validate input frame structure */
    if (App_VerifyPacket(Frame) != STAT_OKE) return STAT_ERR_CRC;
    
    /* Control flow: verify frame type */
    if (Frame->Header.FrameType != eFrameType_EngineControl3) return STAT_ERR_INVALID_ARG;

    if (syncCtrl) *syncCtrl = Frame->Body.EngineControl3.SyncCtrlNum;
    if (m0) *m0 = Frame->Body.EngineControl3.M0;
    if (m1) *m1 = Frame->Body.EngineControl3.M1;

    return STAT_OKE;
}

ReturnCode_t App_ReadFrame_EmergencyStop(const ZECUFrame_Generic_t* Frame, uint32_t* reason, uint16_t* d0, uint16_t* d1, uint16_t* d2, int8_t* m0, int8_t* m1, uint32_t* locLong, uint32_t* locLat) {
    /* Control flow: validate input frame structure */
    if (App_VerifyPacket(Frame) != STAT_OKE) return STAT_ERR_CRC;
    
    /* Control flow: verify frame type */
    if (Frame->Header.FrameType != eFrameType_EmergencyStop) return STAT_ERR_INVALID_ARG;

    if (reason) *reason = Frame->Body.EmergencyStop.ReasonID;
    if (d0) *d0 = Frame->Body.EmergencyStop.D0;
    if (d1) *d1 = Frame->Body.EmergencyStop.D1;
    if (d2) *d2 = Frame->Body.EmergencyStop.D2;
    if (m0) *m0 = Frame->Body.EmergencyStop.M0;
    if (m1) *m1 = Frame->Body.EmergencyStop.M1;
    if (locLong) *locLong = Frame->Body.EmergencyStop.Location_Long;
    if (locLat) *locLat = Frame->Body.EmergencyStop.Location_Lat;

    return STAT_OKE;
}

ReturnCode_t App_ReadFrame_AuthFail(const ZECUFrame_Generic_t* Frame, uint32_t* reason) {
    /* Control flow: validate input frame structure */
    if (App_VerifyPacket(Frame) != STAT_OKE) return STAT_ERR_CRC;
    
    /* Control flow: verify frame type */
    if (Frame->Header.FrameType != eFrameType_AuthFail) return STAT_ERR_INVALID_ARG;

    if (reason) *reason = Frame->Body.AuthFail.ReasonID;

    return STAT_OKE;
}

ReturnCode_t App_ReadFrame_AuthTX(const ZECUFrame_Generic_t* Frame, uint64_t* challenge) {
    /* Control flow: validate input frame structure */
    if (App_VerifyPacket(Frame) != STAT_OKE) return STAT_ERR_CRC;
    
    /* Control flow: verify frame type */
    if (Frame->Header.FrameType != eFrameType_AuthTX) return STAT_ERR_INVALID_ARG;

    if (challenge) *challenge = Frame->Body.AuthTX.Challenge;

    return STAT_OKE;
}

ReturnCode_t App_ReadFrame_KeepAlive(const ZECUFrame_Generic_t* Frame, uint8_t* syncNum) {
    /* Control flow: validate input frame structure */
    if (App_VerifyPacket(Frame) != STAT_OKE) return STAT_ERR_CRC;
    
    /* Control flow: verify frame type */
    if (Frame->Header.FrameType != eFrameType_KeepAlive) return STAT_ERR_INVALID_ARG;

    if (syncNum) *syncNum = Frame->Body.KeepAlive.SyncNum0;

    return STAT_OKE;
}

ReturnCode_t App_VerifyFrame_AuthRX(const ZECUFrame_Generic_t* Frame, uint64_t original_challenge) {
    /* Control flow: validate input frame structure internally via checksum/CRC */
    if (App_VerifyPacket(Frame) != STAT_OKE) {
        return STAT_ERR_CRC;
    }
    
    /* Control flow: ensure this is the correct authorization response frame */
    if (Frame->Header.FrameType != eFrameType_AuthRX) {
        return STAT_ERR_INVALID_ARG;
    }

    /* Process signature validation using the NodeAuth library engine */
    return Auth_VerifySignature(original_challenge, Frame->Body.AuthRX.Signature);
}