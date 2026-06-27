#ifndef __APP_SOMEIP_DEFS_H__
#define __APP_SOMEIP_DEFS_H__

#include "__CommonHeaders.h"

typedef union {
    struct __attribute__((packed)) {
        uint8_t Ignored0                    : 2;        
        uint8_t TP_Flag                     : 1;        
        uint8_t Ignored1                    : 5;        
    };
    uint8_t Uint8;
} SOMEIP_TP_MessageType_t;

typedef union {
    struct __attribute__((packed)) {
        uint32_t RequestID;
        uint32_t ProtocolVersion            :   8;
        uint32_t InterfaceVersion           :   8;
        uint32_t MessageType                :   8;
        uint32_t ReturnCode                 :   8;
        uint32_t Offset28                   :   28;
        uint32_t RES                        :   3;
        uint32_t M                          :   1;
    };
    uint8_t Uint8[8];
} SOMEIP_TP_Header_t;

typedef union {
    struct __attribute__((packed)) {
        uint32_t            MessageID;
        uint32_t            Length;
    };
    uint8_t Uint8[16];
} SOMEIP_Header_t;


/* PACKET *****************************************************************************************/

typedef union SOMEIP_PayloadPtr_t {
    void*                   VoidPtr;
    uint8_t                 BytePtr;
} SOMEIP_PayloadPtr_t;

typedef struct SOMEIP_Packet_t {
    SOMEIP_Header_t         Header;
    SOMEIP_PayloadPtr_t     PayloadPtr;
} SOMEIP_Packet_t;

/// @brief Initialized a SOMEIP packet (prevent garbage value).
/// @param PacketPtr Pointer to the SOMEIP_Packet_t ojbect.
/// @return Status of the procedure.
ReturnCode_t SOMEIP_ResetPacket(SOMEIP_Packet_t* PacketPtr);

/// @brief Set payload for a SOMEIP packet (auto updates length field).
/// @param PacketPtr        Target packet.
/// @param PayloadPtr       Source payload.
/// @param PayloadLength    Length of payload (unit: byte).
/// @return Status of the procedure.
ReturnCode_t SOMEIP_SetPayload1(SOMEIP_Packet_t* PacketPtr, SOMEIP_PayloadPtr_t* PayloadPtr, uint32_t PayloadLength);

/// @brief Set payload for a SOMEIP packet (auto allocates new memory if PayloadPtr is NULL, updates length field).
/// @param PacketPtr        Target packet.
/// @param PayloadPtr       Source payload.
/// @param PayloadLength    Length of payload (unit: byte).
/// @return Status of the procedure.
ReturnCode_t SOMEIP_SetPayload2(SOMEIP_Packet_t* PacketPtr, SOMEIP_PayloadPtr_t* PayloadPtr, uint32_t PayloadLength);


/// @brief Allocate new payload for the SOMEIP_Packet_t (Initialize with ZERO).
/// @param PayloadPtr Pointer to a PayloadPtr_t object to store new allocated memonry.
/// @param PayloadLength Size of the payload
/// @retval STAT_OKE Successful
/// @retval STAT_ERR If has an error.
ReturnCode_t SOMEIP_AllocateNewPayload1(SOMEIP_PayloadPtr_t* PayloadPtr, uint32_t PayloadLength);

/// @brief Allocate new payload for the SOMEIP_Packet_t (With init value array).
/// @param PayloadPtr Pointer to a PayloadPtr_t object to store new allocated memonry.
/// @param PayloadLength Size of the payload
/// @retval STAT_OKE Successful
/// @retval STAT_ERR If has an error.
ReturnCode_t SOMEIP_AllocateNewPayload2(SOMEIP_PayloadPtr_t* PayloadPtr, void* InitValuePtr, uint32_t PayloadLength);

/// @brief Free allocated memory of payload (prevent dangling pointer).
/// @param PayloadPtr Target payload.
/// @return Status of the procedure.
ReturnCode_t SOMEIP_FreeNewPayload(SOMEIP_PayloadPtr_t* PayloadPtr);

/// @brief Finalize the target packet (incl. check length, payload, compute E2E if nay).
/// @param PacketPtr Target packet.
/// @return Status of the procedure.
ReturnCode_t SOMEIP_FinalizePacket(SOMEIP_Packet_t* PacketPtr);

#endif /*__APP_SOMEIP_DEFS_H__*/
