#ifndef __RXED_PACKET_CIRCULAR_QUEUE_H__
#define __RXED_PACKET_CIRCULAR_QUEUE_H__

#include "EthernetW5500.h"
#include <stdatomic.h>

/**
 * @brief Slot lifecycle states for atomic state machine management.
 */
typedef enum {
    SLOT_FREE    = 0,  /**< Empty, available for writing. */
    SLOT_WRITING = 1,  /**< Locked by a producer for writing. */
    SLOT_READY   = 2,  /**< Data written, available for reading. */
    SLOT_READING = 3   /**< Locked by a consumer for processing. */
} PacketSlotState_t;

/**
 * @brief Static structure representing a network packet slot.
 */
typedef struct {
    atomic_uint_fast8_t State;      /**< Atomic state for thread-safe access. */
    EthSize_t           Size;       /**< Actual payload size. */
    IPv4Addr_t          SrcIP;      /**< Extracted Source IP Address */
    IPv4Port_t          SrcPort;    /**< Extracted Source Port */
    EthMACAddr_t        SrcMAC;     /**< Extracted Source MAC Address */
    Byte_t              Data[1500]; /**< Static buffer for packet data. */
} PacketSlot_t;

/* INTERNAL SETUP FUNCTION *******************************************************************************************************/

/**
 * @brief Initializes the lock-free pool.
 */
/* Initialize the internal packet pool and atomic states */
void RxedPacket_Init();

/* QUEUE MANIPULATE FN *******************************************************************************************************/

/**
 * @brief Finds an empty slot and copies the payload using a lock-free approach.
 * 
 * @param Size The size of the payload to be pushed.
 * @param Payload Pointer to the source data.
 * @return ReturnCode_t STAT_SUCCESS if pushed, error code otherwise.
 */
/* Locate a free slot and perform thread-safe data ingestion */
ReturnCode_t        RxedPacket_Push(EthSize_t Size, GenericPtr_t Payload, uint8_t* src_ip, uint16_t src_port, uint8_t* src_mac);

/**
 * @brief Scans READY slots to find and lock the packet with the highest priority.
 * 
 * @return PacketSlot_t* Pointer to the locked slot, or NULL if no packets are available.
 */
/* Identify the highest priority slot and transition state to READING */
PacketSlot_t*       RxedPacket_GetHighestPriority();

/**
 * @brief Releases the slot after the consumer has finished processing.
 * 
 * @param Slot Pointer to the slot to be released.
 */
/* Reset the slot state to FREE to allow subsequent writes */
void                RxedPacket_Release(PacketSlot_t* Slot);

/**
 * @brief Checks if the priority queue contains any packets.
 * 
 * @return ReturnCode_t STAT_OKE if empty, STAT_ERR if packets are present.
 */
/* Evaluate if the queue is currently devoid of packets */
ReturnCode_t        RxedPacket_Empty();

/**
 * @brief Returns the total number of packets currently in the priority queue.
 * 
 * @return ReturnCode_t The count of packets (NumRxedPacket).
 */
/* Retrieve the current count of occupied slots in the queue */
ReturnCode_t        RxedPacket_Size();

/* TX QUEUE MANIPULATE FN ****************************************************************************************************/

/* Lock-free functions for Transmitting Packets */
ReturnCode_t        TxPacket_Push(EthSize_t Size, GenericPtr_t Payload, uint8_t* dst_ip, uint16_t dst_port, uint8_t* dst_mac);
PacketSlot_t*       TxPacket_GetHighestPriority();
void                TxPacket_Release(PacketSlot_t* Slot);
ReturnCode_t        TxPacket_Empty();
ReturnCode_t        TxPacket_Size();

#endif /* __RXED_PACKET_CIRCULAR_QUEUE_H__ */

/* EOF *******************************************************************************************************************/