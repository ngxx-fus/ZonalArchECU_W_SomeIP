#ifndef __RXED_PACKET_CIRCULAR_QUEUE_H__
#define __RXED_PACKET_CIRCULAR_QUEUE_H__

#include "EthernetW5500.h"
#include <stdatomic.h>

/**
 * @brief Slot lifecycle states for atomic state machine management.
 * Ensures thread-safe, lock-free access between producer and consumer tasks.
 */
typedef enum {
    SLOT_FREE    = 0,  /**< Empty, available for producer to write. */
    SLOT_WRITING = 1,  /**< Locked by a producer, data is being copied. */
    SLOT_READY   = 2,  /**< Data successfully written, available for consumer. */
    SLOT_READING = 3   /**< Locked by a consumer, data is being processed. */
} PacketSlotState_t;

/**
 * @brief Static structure representing a network packet slot in the pool.
 */
typedef struct {
    atomic_uint_fast8_t State;      /**< Atomic state for thread-safe memory access. */
    EthSize_t           Size;       /**< Actual payload size in bytes. */
    IPv4Addr_t          SrcIP;      /**< Extracted Source/Dest IP Address. */
    IPv4Port_t          SrcPort;    /**< Extracted Source/Dest Port. */
    EthMACAddr_t        SrcMAC;     /**< Extracted Source/Dest MAC Address. */
    Byte_t              Data[1500]; /**< Static buffer for maximum Ethernet MTU payload. */
} PacketSlot_t;

/* INTERNAL SETUP FUNCTION *******************************************************************************************************/

/**
 * @brief Initializes the internal packet pool and atomic states.
 * Must be called once before any Push/Pop operations to ensure all slots are FREE.
 */
void RxedPacket_Init();

/* RX QUEUE MANIPULATE FN *******************************************************************************************************/

/**
 * @brief Finds a FREE slot and copies the payload using an atomic lock-free approach.
 * * @param Size The size of the incoming payload to be pushed (max 1500).
 * @param Payload Pointer to the source data buffer.
 * @param src_ip Pointer to the extracted 4-byte source IP.
 * @param src_port The extracted 2-byte source port.
 * @param src_mac Pointer to the extracted 6-byte source MAC.
 * @return ReturnCode_t STAT_OKE if successfully pushed, error code otherwise.
 */
ReturnCode_t        RxedPacket_Push(EthSize_t Size, GenericPtr_t Payload, uint8_t* src_ip, uint16_t src_port, uint8_t* src_mac);

/**
 * @brief Scans READY slots to find and lock the packet with the highest priority (lowest ID).
 * Transitions state to READING to prevent concurrent access by other consumers.
 * * @return PacketSlot_t* Pointer to the locked slot, or NULL if no packets are available.
 */
PacketSlot_t* RxedPacket_GetHighestPriority();

/**
 * @brief Releases the slot back to the FREE state after the consumer finishes processing.
 * * @param Slot Pointer to the slot to be released.
 */
void                RxedPacket_Release(PacketSlot_t* Slot);

/**
 * @brief Checks if the Rx priority queue is currently devoid of packets.
 * * @return ReturnCode_t STAT_OKE if empty (0 packets), STAT_ERR if packets are present.
 */
ReturnCode_t        RxedPacket_Empty();

/**
 * @brief Retrieves the current count of READY slots in the Rx queue.
 * * @return ReturnCode_t The total number of unread packets.
 */
ReturnCode_t        RxedPacket_Size();

/* TX QUEUE MANIPULATE FN ****************************************************************************************************/

/**
 * @brief Pushes a new packet into the Tx pool for network transmission.
 * * @param Size The size of the payload to be transmitted.
 * @param Payload Pointer to the source data to send.
 * @param dst_ip Pointer to the destination IP address.
 * @param dst_port The destination port number.
 * @param dst_mac Pointer to the destination MAC address.
 * @return ReturnCode_t STAT_OKE if successfully pushed, error code otherwise.
 */
ReturnCode_t        TxPacket_Push(EthSize_t Size, GenericPtr_t Payload, uint8_t* dst_ip, uint16_t dst_port, uint8_t* dst_mac);

/**
 * @brief Scans READY slots to find the highest priority Tx packet and locks it for transmission.
 * * @return PacketSlot_t* Pointer to the locked Tx slot, or NULL if empty.
 */
PacketSlot_t* TxPacket_GetHighestPriority();

/**
 * @brief Releases the Tx slot back to FREE state after transmission is complete.
 * * @param Slot Pointer to the slot to be released.
 */
void                TxPacket_Release(PacketSlot_t* Slot);

/**
 * @brief Checks if the Tx queue is currently empty.
 * * @return ReturnCode_t STAT_OKE if empty, STAT_ERR if packets are waiting to send.
 */
ReturnCode_t        TxPacket_Empty();

/**
 * @brief Returns the total number of packets currently waiting to be transmitted.
 * * @return ReturnCode_t The count of ready Tx packets.
 */
ReturnCode_t        TxPacket_Size();

#endif /* __RXED_PACKET_CIRCULAR_QUEUE_H__ */

/* EOF *******************************************************************************************************************/