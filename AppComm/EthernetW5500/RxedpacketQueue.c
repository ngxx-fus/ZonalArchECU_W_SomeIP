#include "RxedPacketQueue.h"

SemaphoreHandle_t    RxedPacket_Lock;
const EthSize_t      MaxNumRxedPacket = 8;
volatile EthSize_t   NumRxedPacket = 0;
GenericPayload_t     RxedPacket[8];

/* INTERNAL SETUP FUNCTION ******************************************************************************************************/

/// @brief Initializes the priority queue with a Mutex for thread safety and priority inheritance
void RxedPacket_Init() {
    static uint8_t Initialized = 0;
    if (Initialized < 1) {
        /* Initialize the mutex for queue manipulation protection */
        Initialized = 1;
        RxedPacket_Lock = xSemaphoreCreateMutex();
    }
}

/// @brief Compares two payloads to determine their priority (Min-Heap logic)
/// @param A Pointer to the first payload
/// @param B Pointer to the second payload
/// @return -1 if A < B, 0 if equal, 1 if A > B
int8_t CompareTwoPayload(GenericPayload_t* A, GenericPayload_t* B) {
    /* Validate pointers to prevent null dereference crashes */
    if (A == NULL || B == NULL || A->Ptr == NULL || B->Ptr == NULL) {
        return 0;
    }

    if (A->Size < B->Size) {
        return -1;
    }
    else if (A->Size > B->Size) {
        return 1;
    }
    else {
        /* Compare actual data byte by byte if sizes are equal */
        for (int i = 0; i < (int)A->Size; ++i) {
            if (((Byte_t*)A->Ptr)[i] < ((Byte_t*)B->Ptr)[i]) {
                return -1;
            }
            else if (((Byte_t*)A->Ptr)[i] > ((Byte_t*)B->Ptr)[i]) {
                return 1;
            }
        }
        return 0;
    }
}

/* INTERNAL UTILS ***********************************************************************************************************/

/// @brief Swaps two payload structures in the heap array
/// @param i First index
/// @param j Second index
static void SwapPayload(EthSize_t i, EthSize_t j) {
    /* Perform shallow copy swap, which safely swaps the ownership of the internal pointers */
    GenericPayload_t Temp = RxedPacket[i];
    RxedPacket[i] = RxedPacket[j];
    RxedPacket[j] = Temp;
}

/// @brief Maintains min-heap property by moving an element up
/// @param Index Starting index
static void HeapifyUp(EthSize_t Index) {
    /* Compare with parent and swap if current is smaller to maintain Min-Heap */
    while (Index > 0) {
        EthSize_t Parent = (Index - 1) / 2;
        if (CompareTwoPayload(&RxedPacket[Index], &RxedPacket[Parent]) < 0) {
            SwapPayload(Index, Parent);
            Index = Parent;
        }
        else {
            break;
        }
    }
}

/// @brief Maintains min-heap property by moving an element down
/// @param Index Starting index
static void HeapifyDown(EthSize_t Index) {
    /* Sink the element down until the smallest is at the top */
    while (1) {
        EthSize_t Smallest = Index;
        EthSize_t Left = 2 * Index + 1;
        EthSize_t Right = 2 * Index + 2;

        if (Left < NumRxedPacket && CompareTwoPayload(&RxedPacket[Left], &RxedPacket[Smallest]) < 0) {
            Smallest = Left;
        }
        if (Right < NumRxedPacket && CompareTwoPayload(&RxedPacket[Right], &RxedPacket[Smallest]) < 0) {
            Smallest = Right;
        }

        if (Smallest != Index) {
            SwapPayload(Index, Smallest);
            Index = Smallest;
        }
        else {
            break;
        }
    }
}

/* INTERFACE FUNCTIONS ******************************************************************************************************/

/// @brief Inserts a new packet into the priority queue and reorganizes the heap
/// @param Size The size of the payload in bytes
/// @param Payload Pointer to the raw data buffer
/// @return STAT_OKE on success, or error code on failure
ReturnCode_t RxedPacket_Push(EthSize_t Size, GenericPtr_t Payload) {
    /* Acquire mutex to safely manipulate queue */
    if (xSemaphoreTake(RxedPacket_Lock, pdMS_TO_TICKS(100)) != pdTRUE) {
        return STAT_ERR_BUSY;
    }

    if (NumRxedPacket >= MaxNumRxedPacket) {
        xSemaphoreGive(RxedPacket_Lock);
        return STAT_ERR_OVERFLOW;
    }
    else {
        /* Allocate new memory and copy the payload into the heap */
        EthSize_t CurrentIdx = NumRxedPacket;
        RxedPacket[CurrentIdx].Ptr = NULL;
        RxedPacket[CurrentIdx].Size = 0;
        
        NewGenericPayload(&RxedPacket[CurrentIdx], Size);
        if (RxedPacket[CurrentIdx].Ptr == NULL) {
            xSemaphoreGive(RxedPacket_Lock);
            return STAT_ERR_MALLOC_FAILED;
        }
        else {
            memcpy(RxedPacket[CurrentIdx].Ptr, Payload.Void, Size);
            NumRxedPacket++;

            HeapifyUp(CurrentIdx);
            xSemaphoreGive(RxedPacket_Lock);
            return STAT_OKE;
        }
    }
}

/// @brief Retrieves the packet with the highest priority (the root element)
/// @warning Caller MUST lock RxedPacket_Lock before invoking this and accessing the returned pointer
/// @return Pointer to the top GenericPayload_t structure, or NULL if empty
GenericPayload_t* RxedPacket_Top() {
    /* Return the root element directly without locking */
    return (NumRxedPacket == 0) ? NULL : &RxedPacket[0];
}

/// @brief Removes the highest priority packet and restores heap properties
/// @return STAT_OKE on success, or error code if the queue is empty
ReturnCode_t RxedPacket_Pop() {
    /* Acquire mutex to protect queue state */
    if (xSemaphoreTake(RxedPacket_Lock, pdMS_TO_TICKS(100)) != pdTRUE) {
        return STAT_ERR_BUSY;
    }

    if (NumRxedPacket == 0) {
        xSemaphoreGive(RxedPacket_Lock);
        return STAT_ERR_UNDERFLOW;
    }
    else {
        /* Delete the old root safely, then replace it with the last element */
        DeleteGenericPayload(&RxedPacket[0]);
        NumRxedPacket--;

        if (NumRxedPacket > 0) {
            RxedPacket[0] = RxedPacket[NumRxedPacket];
            
            /* Clear the dangling old leaf element */
            RxedPacket[NumRxedPacket].Ptr = NULL;
            RxedPacket[NumRxedPacket].Size = 0;
            
            HeapifyDown(0);
        }

        xSemaphoreGive(RxedPacket_Lock);
        return STAT_OKE;
    }
}

/// @brief Clears all packets from the queue and releases all allocated payload memory
/// @return STAT_OKE on success, or STAT_ERR_BUSY if the lock cannot be acquired
ReturnCode_t RxedPacket_Clear() {
    /* Acquire mutex to safely free all queued memory */
    if (xSemaphoreTake(RxedPacket_Lock, pdMS_TO_TICKS(100)) != pdTRUE) {
        return STAT_ERR_BUSY;
    }
    else {
        while (NumRxedPacket > 0) {
            NumRxedPacket--;
            DeleteGenericPayload(&RxedPacket[NumRxedPacket]);
        }

        xSemaphoreGive(RxedPacket_Lock);
        return STAT_OKE;
    }
}

/// @brief Checks if the priority queue contains any packets
/// @return STAT_OKE if empty, STAT_ERR if packets are present
ReturnCode_t RxedPacket_Empty() {
    return (NumRxedPacket == 0) ? STAT_OKE : STAT_ERR;
}

/// @brief Returns the total number of packets currently in the priority queue
/// @return The count of packets (NumRxedPacket)
ReturnCode_t RxedPacket_Size() {
    return (ReturnCode_t)NumRxedPacket;
}
