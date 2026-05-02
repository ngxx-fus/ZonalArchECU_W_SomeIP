#ifndef __RXED_PACKET_CIRCULAR_QUEUE_H__
#define __RXED_PACKET_CIRCULAR_QUEUE_H__

#include "Module.h"

extern SemaphoreHandle_t    RxedPacket_Lock;
extern const EthSize_t      MaxNumRxedPacket;
extern volatile EthSize_t   NumRxedPacket;
extern GenericPayload_t     RxedPacket[];

/*INTERNAL SETUP FUNCTION *******************************************************************************************************/

/// @brief Initializes the priority queue and its synchronization mutex
void RxedPacket_Init();

/*INTERNAL COMPARE FUNCTION *****************************************************************************************************/

/// @brief Compares two payloads to determine their priority (Min-Heap logic)
/// @param A Pointer to the first payload structure to compare
/// @param B Pointer to the second payload structure to compare
/// @return -1 if A < B (higher priority), 0 if equal, 1 if A > B
int8_t CompareTwoPayload(GenericPayload_t* A, GenericPayload_t* B);

/*QUEUE MANIPULATE FN *******************************************************************************************************/

/// @brief Clears all packets from the queue and releases all allocated payload memory
/// @return STAT_OKE on success, or STAT_ERR_BUSY if the lock cannot be acquired
ReturnCode_t        RxedPacket_Clear();

/// @brief Inserts a new packet into the priority queue and reorganizes the heap
/// @param Size The size of the payload in bytes
/// @param Payload Pointer to the raw data buffer
/// @return STAT_OKE on success, or error code on failure
ReturnCode_t        RxedPacket_Push(EthSize_t Size, GenericPtr_t Payload);

/// @brief Retrieves the packet with the highest priority (the root element)
/// @warning Caller MUST lock RxedPacket_Lock before invoking this and accessing the returned pointer
/// @return Pointer to the top GenericPayload_t structure, or NULL if empty
GenericPayload_t* RxedPacket_Top();

/// @brief Removes the highest priority packet and restores heap properties
/// @return STAT_OKE on success, or error code if the queue is empty
ReturnCode_t        RxedPacket_Pop();

/// @brief Checks if the priority queue contains any packets
/// @return STAT_OKE if empty, STAT_ERR if packets are present
ReturnCode_t        RxedPacket_Empty();

/// @brief Returns the total number of packets currently in the priority queue
/// @return The count of packets (NumRxedPacket)
ReturnCode_t        RxedPacket_Size();

#endif /// __RXED_PACKET_CIRCULAR_QUEUE_H__

/* EOF *******************************************************************************************************************/