#include "PacketQueue.h"

#define MAX_NUM_RXED_PACKETS 32
#define MAX_NUM_TXED_PACKETS 32

/* Static pool allocations without dynamic memory usage */
PacketSlot_t RxPool[MAX_NUM_RXED_PACKETS];
PacketSlot_t TxPool[MAX_NUM_TXED_PACKETS];

/* INTERNAL SETUP FUNCTION ******************************************************************************************************/

/**
 * @brief Initializes the lock-free packet pool.
 */
void RxedPacket_Init() {
    /* Initialize every slot in the pools to an available state */
    for (int32_t i = 0; i < MAX_NUM_RXED_PACKETS; i++) {
        atomic_init(&RxPool[i].State, SLOT_FREE);
        RxPool[i].Size = 0;
        atomic_init(&TxPool[i].State, SLOT_FREE);
        TxPool[i].Size = 0;
    }
}

/* INTERFACE FUNCTIONS ******************************************************************************************************/

/**
 * @brief Pushes a new packet into the pool using atomic compare-and-swap.
 * * @param Size Size of the incoming payload.
 * @param Payload Pointer to the data to be copied.
 * @param src_ip Pointer to the source IP address.
 * @param src_port Source port number.
 * @param src_mac Pointer to the source MAC address.
 * @return STAT_OKE on success, error code otherwise.
 */
ReturnCode_t RxedPacket_Push(EthSize_t Size, GenericPtr_t Payload, uint8_t* src_ip, uint16_t src_port, uint8_t* src_mac) {
    /* Validate input arguments before processing */
    if (Size > 1500 || Payload.Void == NULL) {
        /* Return invalid argument error */
        return STAT_ERR_INVALID_ARG;
    }

    /* Iterate through the Rx pool to find an available FREE slot */
    for (int32_t i = 0; i < MAX_NUM_RXED_PACKETS; i++) {
        uint_fast8_t expected = SLOT_FREE;
        
        /* Attempt to atomically transition state from FREE to WRITING */
        if (atomic_compare_exchange_strong(&RxPool[i].State, &expected, SLOT_WRITING)) {
            
            /* Logic: Copy data into the locked static buffer */
            memcpy(RxPool[i].Data, Payload.Void, Size);
            RxPool[i].Size = Size;
            
            /* Copy source IP if valid pointer is provided */
            if(src_ip) memcpy(RxPool[i].SrcIP.Byte, src_ip, 4);
            
            RxPool[i].SrcPort.Word = src_port;
            
            /* Copy source MAC if valid pointer is provided */
            if(src_mac) memcpy(RxPool[i].SrcMAC.Byte, src_mac, 6);

            /* Finalize write operation and signal readiness for consumers */
            atomic_store(&RxPool[i].State, SLOT_READY);
            
            /* Return success */
            return STAT_OKE;
        }
    }
    
    /* Return error if no slots were available during the scan */
    return STAT_ERR_OVERFLOW;
}

/**
 * @brief Identifies the highest priority packet and locks it for reading.
 * * @return Pointer to the locked PacketSlot_t, or NULL if none found.
 */
PacketSlot_t* RxedPacket_GetHighestPriority() {
    int32_t best_idx = -1;
    uint32_t highest_priority = 0xFFFFFFFF;

    /* Scan all slots to identify the one with the lowest priority value (highest priority) */
    for (int32_t i = 0; i < MAX_NUM_RXED_PACKETS; i++) {
        
        /* Check if the current slot is in READY state */
        if (atomic_load(&RxPool[i].State) == SLOT_READY) {
            
            /* Logic: Extract priority identifier from the first byte of data */
            uint8_t current_priority = RxPool[i].Data[0];
            
            /* Update best candidate if current packet has higher priority */
            if (best_idx == -1 || current_priority < highest_priority) {
                highest_priority = current_priority;
                best_idx = i;
            }
        }
    }

    /* Attempt to lock the best candidate if one was identified */
    if (best_idx != -1) {
        uint_fast8_t expected = SLOT_READY;
        
        /* Atomic check to prevent race conditions with other consumers */
        if (atomic_compare_exchange_strong(&RxPool[best_idx].State, &expected, SLOT_READING)) {
            
            /* Return pointer to the successfully locked packet */
            return &RxPool[best_idx];
        }
    }
    
    /* Return NULL if no eligible packets were found or lock failed */
    return NULL;
}

/**
 * @brief Releases a previously locked slot back to the FREE state.
 * * @param Slot Pointer to the slot to be released.
 */
void RxedPacket_Release(PacketSlot_t* Slot) {
    /* Ensure the pointer is valid before modifying state */
    if (Slot != NULL) {
        /* Restore slot to FREE state for producer reuse */
        atomic_store(&Slot->State, SLOT_FREE);
    }
}

/**
 * @brief Checks if the Rx queue is empty.
 * * @return STAT_OKE if empty, STAT_ERR if not.
 */
ReturnCode_t RxedPacket_Empty() {
    /* Return status based on current size */
    return (RxedPacket_Size() == 0) ? STAT_OKE : STAT_ERR;
}

/**
 * @brief Calculates the number of packets currently in READY state.
 * * @return Total count of ready packets.
 */
ReturnCode_t RxedPacket_Size() {
    uint8_t count = 0;
    
    /* Accumulate count of all slots currently marked as READY */
    for (int32_t i = 0; i < MAX_NUM_RXED_PACKETS; i++) {
        
        /* Atomic load to ensure state consistency during count */
        if (atomic_load(&RxPool[i].State) == SLOT_READY) {
            count++;
        }
    }
    
    /* Return the total count */
    return count;
}

/* TX INTERFACE FUNCTIONS ***************************************************************************************************/

/**
 * @brief Pushes a new packet into the Tx pool using atomic compare-and-swap.
 * * @param Size Size of the outgoing payload.
 * @param Payload Pointer to the data to be copied.
 * @param dst_ip Pointer to the destination IP address.
 * @param dst_port Destination port number.
 * @param dst_mac Pointer to the destination MAC address.
 * @return STAT_OKE on success, error code otherwise.
 */
ReturnCode_t TxPacket_Push(EthSize_t Size, GenericPtr_t Payload, uint8_t* dst_ip, uint16_t dst_port, uint8_t* dst_mac) {
    /* Validate input arguments before processing */
    if (Size > 1500 || Payload.Void == NULL) {
        /* Return invalid argument error */
        return STAT_ERR_INVALID_ARG;
    }

    /* Iterate through the Tx pool to find an available FREE slot */
    for (int32_t i = 0; i < MAX_NUM_TXED_PACKETS; i++) {
        uint_fast8_t expected = SLOT_FREE;
        
        /* Attempt to atomically transition state from FREE to WRITING */
        if (atomic_compare_exchange_strong(&TxPool[i].State, &expected, SLOT_WRITING)) {
            memcpy(TxPool[i].Data, Payload.Void, Size);
            TxPool[i].Size = Size;
            
            /* Copy destination IP if valid pointer is provided */
            if(dst_ip) memcpy(TxPool[i].SrcIP.Byte, dst_ip, 4);
            
            TxPool[i].SrcPort.Word = dst_port;
            
            /* Copy destination MAC if valid pointer is provided */
            if(dst_mac) memcpy(TxPool[i].SrcMAC.Byte, dst_mac, 6);
            
            /* Finalize write operation and signal readiness for consumption */
            atomic_store(&TxPool[i].State, SLOT_READY);
            
            /* Return success */
            return STAT_OKE;
        }
    }
    
    /* Return error if no slots were available during the scan */
    return STAT_ERR_OVERFLOW;
}

/**
 * @brief Identifies the highest priority packet in Tx pool and locks it for reading.
 * * @return Pointer to the locked PacketSlot_t, or NULL if none found.
 */
PacketSlot_t* TxPacket_GetHighestPriority() {
    int32_t best_idx = -1;
    uint32_t highest_priority = 0xFFFFFFFF;

    /* Scan all slots to identify the one with the lowest priority value */
    for (int32_t i = 0; i < MAX_NUM_TXED_PACKETS; i++) {
        
        /* Check if the current slot is in READY state */
        if (atomic_load(&TxPool[i].State) == SLOT_READY) {
            uint8_t current_priority = TxPool[i].Data[0];
            
            /* Update best candidate if current packet has higher priority */
            if (best_idx == -1 || current_priority < highest_priority) {
                highest_priority = current_priority;
                best_idx = i;
            }
        }
    }

    /* Attempt to lock the best candidate if one was identified */
    if (best_idx != -1) {
        uint_fast8_t expected = SLOT_READY;
        
        /* Atomic check to prevent race conditions */
        if (atomic_compare_exchange_strong(&TxPool[best_idx].State, &expected, SLOT_READING)) {
            /* Return pointer to the successfully locked packet */
            return &TxPool[best_idx];
        }
    }
    
    /* Return NULL if no eligible packets were found or lock failed */
    return NULL;
}

/**
 * @brief Releases a previously locked Tx slot back to the FREE state.
 * * @param Slot Pointer to the slot to be released.
 */
void TxPacket_Release(PacketSlot_t* Slot) {
    /* Ensure the pointer is valid before modifying state */
    if (Slot != NULL) {
        /* Restore slot to FREE state for producer reuse */
        atomic_store(&Slot->State, SLOT_FREE);
    }
}

/**
 * @brief Checks if the Tx queue is empty.
 * * @return STAT_OKE if empty, STAT_ERR if not.
 */
ReturnCode_t TxPacket_Empty() {
    /* Return status based on current Tx queue size */
    return (TxPacket_Size() == 0) ? STAT_OKE : STAT_ERR;
}

/**
 * @brief Calculates the number of Tx packets currently in READY state.
 * * @return Total count of ready Tx packets.
 */
ReturnCode_t TxPacket_Size() {
    uint8_t count = 0;
    
    /* Accumulate count of all slots currently marked as READY */
    for (int32_t i = 0; i < MAX_NUM_TXED_PACKETS; i++) {
        
        /* Atomic load to ensure state consistency during count */
        if (atomic_load(&TxPool[i].State) == SLOT_READY) {
            count++;
        }
    }
    
    /* Return the total count */
    return count;
}