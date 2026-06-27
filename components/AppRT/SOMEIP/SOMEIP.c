#include "__CommonHeaders.h"
#include <stdlib.h>
#include <string.h>

/* PACKET UTILITIES *******************************************************************************/

/// @brief Initialized a SOMEIP packet (prevent garbage value).
/// @param PacketPtr Pointer to the SOMEIP_Packet_t ojbect.
/// @return Status of the procedure.
ReturnCode_t SOMEIP_ResetPacket(SOMEIP_Packet_t* PacketPtr) {
    if (PacketPtr == NULL) {
        return STAT_ERR_NULL;
    }

    /* Clear the entire packet structure to zero */
    memset(PacketPtr, 0, sizeof(SOMEIP_Packet_t));
    
    return STAT_OKE;
}

/// @brief Set payload for a SOMEIP packet (auto updates length field).
/// @param PacketPtr        Target packet.
/// @param PayloadPtr       Source payload.
/// @param PayloadLength    Length of payload (unit: byte).
/// @return Status of the procedure.
ReturnCode_t SOMEIP_SetPayload1(SOMEIP_Packet_t* PacketPtr, SOMEIP_PayloadPtr_t* PayloadPtr, uint32_t PayloadLength) {
    if (PacketPtr == NULL || PayloadPtr == NULL) {
        return STAT_ERR_NULL;
    }

    /* Assign payload pointer directly */
    PacketPtr->PayloadPtr = *PayloadPtr;
    
    /* Auto update SOME/IP Length field.
     * Note: In SOME/IP, the Length field covers everything after itself. 
     * Thus, it includes the size of the TP_Header (8 bytes) + actual PayloadLength. */
    PacketPtr->Header.Length = sizeof(SOMEIP_TP_Header_t) + PayloadLength;

    return STAT_OKE;
}

/// @brief Set payload for a SOMEIP packet (auto allocates new memory if PayloadPtr is NULL, updates length field).
/// @param PacketPtr        Target packet.
/// @param PayloadPtr       Source payload.
/// @param PayloadLength    Length of payload (unit: byte).
/// @return Status of the procedure.
ReturnCode_t SOMEIP_SetPayload2(SOMEIP_Packet_t* PacketPtr, SOMEIP_PayloadPtr_t* PayloadPtr, uint32_t PayloadLength) {
    if (PacketPtr == NULL) {
        return STAT_ERR_NULL;
    }

    /* If the provided pointer or its allocated data is NULL, allocate automatically */
    if (PayloadPtr == NULL || PayloadPtr->VoidPtr == NULL) {
        SOMEIP_PayloadPtr_t newPayload;
        ReturnCode_t ret = SOMEIP_AllocateNewPayload1(&newPayload, PayloadLength);
        
        if (ret != STAT_OKE) {
            return ret;
        }
        PacketPtr->PayloadPtr = newPayload;
    } else {
        /* Use existing allocated payload */
        PacketPtr->PayloadPtr = *PayloadPtr;
    }

    /* Auto update SOME/IP Length field */
    PacketPtr->Header.Length = sizeof(SOMEIP_TP_Header_t) + PayloadLength;

    return STAT_OKE;
}

/// @brief Allocate new payload for the SOMEIP_Packet_t (Initialize with ZERO).
/// @param PayloadPtr Pointer to a PayloadPtr_t object to store new allocated memonry.
/// @param PayloadLength Size of the payload
/// @retval STAT_OKE Successful
/// @retval STAT_ERR If has an error.
ReturnCode_t SOMEIP_AllocateNewPayload1(SOMEIP_PayloadPtr_t* PayloadPtr, uint32_t PayloadLength) {
    if (PayloadPtr == NULL) {
        return STAT_ERR_NULL;
    }
    
    if (PayloadLength == 0) {
        PayloadPtr->VoidPtr = NULL;
        return STAT_OKE;
    }

    /* Allocate and initialize with zero using calloc */
    PayloadPtr->VoidPtr = calloc(1, PayloadLength);
    
    if (PayloadPtr->VoidPtr == NULL) {
        return STAT_ERR;
    }

    return STAT_OKE;
}

/// @brief Allocate new payload for the SOMEIP_Packet_t (With init value array).
/// @param PayloadPtr Pointer to a PayloadPtr_t object to store new allocated memonry.
/// @param InitValuePtr Array containing values to copy to the new allocated memory.
/// @param PayloadLength Size of the payload
/// @retval STAT_OKE Successful
/// @retval STAT_ERR If has an error.
ReturnCode_t SOMEIP_AllocateNewPayload2(SOMEIP_PayloadPtr_t* PayloadPtr, void* InitValuePtr, uint32_t PayloadLength) {
    if (PayloadPtr == NULL || InitValuePtr == NULL) {
        return STAT_ERR_NULL;
    }

    if (PayloadLength == 0) {
        PayloadPtr->VoidPtr = NULL;
        return STAT_OKE;
    }

    /* Allocate uninitialized memory with malloc */
    PayloadPtr->VoidPtr = malloc(PayloadLength);
    
    if (PayloadPtr->VoidPtr == NULL) {
        return STAT_ERR;
    }

    /* Copy initialized value array into newly allocated buffer */
    memcpy(PayloadPtr->VoidPtr, InitValuePtr, PayloadLength);

    return STAT_OKE;
}

/// @brief Free allocated memory of payload (prevent dangling pointer).
/// @param PayloadPtr Target payload.
/// @return Status of the procedure.
ReturnCode_t SOMEIP_FreeNewPayload(SOMEIP_PayloadPtr_t* PayloadPtr) {
    if (PayloadPtr == NULL) {
        return STAT_ERR_NULL;
    }

    /* Free memory and nullify the pointer to avoid dangling pointers */
    if (PayloadPtr->VoidPtr != NULL) {
        free(PayloadPtr->VoidPtr);
        PayloadPtr->VoidPtr = NULL;
    }

    return STAT_OKE;
}

/// @brief Finalize the target packet (incl. check length, payload, compute E2E if nay).
/// @param PacketPtr Target packet.
/// @return Status of the procedure.
ReturnCode_t SOMEIP_FinalizePacket(SOMEIP_Packet_t* PacketPtr) {
    if (PacketPtr == NULL) {
        return STAT_ERR_NULL;
    }

    /* Base validation: Message length must at minimum cover the TP Header layout size (8 bytes) */
    if (PacketPtr->Header.Length < sizeof(SOMEIP_TP_Header_t)) {
        return STAT_ERR_INVALID_ARG;
    }

    /* TODO: Insert future logic to evaluate End-to-End (E2E) protection like calculating CRC and setting Seq_Counter here */

    return STAT_OKE;
}
