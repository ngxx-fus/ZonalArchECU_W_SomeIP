#ifndef __DEQUE_H__
#define __DEQUE_H__

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "FlagControl.h"

typedef uint16_t    CBuffSize_t;
typedef void* CBuffBase_t;

#define CBUFFER_MAX_SIZE   128U

/// @brief Generic element stored inside buffer
///
/// Ownership:
///  - DataPtr is ALWAYS owned by buffer (malloc/copy inside push)
///  - Will be freed on Pop / Clear / Destroy
typedef struct GenericElement_t {
    CBuffSize_t         DataSize;
    uint8_t* DataPtr;
} GenericElement_t;

enum OnChangeType{ 
    eCBuff_PushFront    = 0, 
    eCBuff_PopFront     = 1, 
    eCBuff_PushBack     = 2, 
    eCBuff_PopBack      = 3, 
    eCBuff_IsFull       = 4, 
    eCBuff_ReAllocate   = 5,
    eCBuff_Clear        = 6
};

/// @brief Generic Circular Buffer (Deque-capable)
///
/// Indexing model:
///  - Head: next write position (front side)
///  - Tail: oldest element (back side)
///
/// Layout (logical):
///  Tail -> [ ... elements ... ] -> Head
typedef struct GenericDeque_t {
    CBuffSize_t         ReservedSize;  /**< Allocated capacity */
    CBuffSize_t         Size;          /**< Current number of elements */

    CBuffSize_t         Head;          /**< Next write index (front) */
    CBuffSize_t         Tail;          /**< Oldest element (back) */

    GenericElement_t*   Arr;           /**< Element array */
    void (*OnChangeCallBack)(enum OnChangeType Type);
} GenericDeque_t;

/* Creation / Destruction *******************************************************************************************************/

/// @brief Create a new circular buffer
/// @param ReservedSize Initial capacity of the buffer
/// @return Pointer to the allocated buffer object or NULL if initialization failed
GenericDeque_t* CBuffer_Create(CBuffSize_t ReservedSize);

/// @brief Destroy buffer and free all associated memory
/// @param cb Pointer to the circular buffer to destroy
void CBuffer_Destroy(GenericDeque_t* cb);

/// @brief Assign a callback function to handle buffer state changes
/// @param cb Pointer to the circular buffer
/// @param CallBack Function pointer matching the OnChangeCallBack signature
void CBuffer_SetCallback(GenericDeque_t* cb, void (*CallBack)(enum OnChangeType Type));

/// @brief Clear the currently assigned callback function (Set to NULL)
/// @param cb Pointer to the circular buffer
void CBuffer_ClearCallback(GenericDeque_t* cb);

/* State utilities (Deque) ******************************************************************************************************/

/// @brief Check if the circular buffer is completely empty
/// @param cb Pointer to the circular buffer
/// @return true if empty, false otherwise
bool CBuffer_IsEmpty(const GenericDeque_t* cb);

/// @brief Check if the circular buffer has reached its maximum size limit
/// @param cb Pointer to the circular buffer
/// @return true if full, false otherwise
bool CBuffer_IsFull(const GenericDeque_t* cb);

/// @brief Retrieve the current active element count in the buffer
/// @param cb Pointer to the circular buffer
/// @return The current number of elements
CBuffSize_t CBuffer_Size(const GenericDeque_t* cb);

/* Push operations (Deque) ******************************************************************************************************/

/// @brief Push an element at the FRONT (Head side)
/// @param cb Pointer to the circular buffer
/// @param Data Pointer to the raw data payload to copy
/// @param DataSize Size of the payload to copy
/// @return The new size of the buffer after insertion
CBuffSize_t CBuffer_PushFront(GenericDeque_t* cb, const CBuffBase_t Data, CBuffSize_t DataSize);

/// @brief Push an element at the BACK (Tail side)
/// @param cb Pointer to the circular buffer
/// @param Data Pointer to the raw data payload to copy
/// @param DataSize Size of the payload to copy
/// @return The new size of the buffer after insertion
CBuffSize_t CBuffer_PushBack(GenericDeque_t* cb, const CBuffBase_t Data, CBuffSize_t DataSize);

/* Pop operations (Deque) *******************************************************************************************************/


/// @brief Remove an element from the FRONT (Head side) and free its memory
/// @param cb Pointer to the circular buffer
/// @return The new size of the buffer after removal
CBuffSize_t CBuffer_PopFront(GenericDeque_t* cb);

/// @brief Remove an element from the BACK (Tail side) and free its memory
/// @param cb Pointer to the circular buffer
/// @return The new size of the buffer after removal
CBuffSize_t CBuffer_PopBack(GenericDeque_t* cb);

/* Access operations ************************************************************************************************************/

/// @brief Access an element by its logical deque index
/// @param cb Pointer to the circular buffer
/// @param Index Logical index (0 = FRONT, Size-1 = BACK, -1 = BACK)
/// @return Pointer to the requested element, or NULL if invalid
GenericElement_t* CBuffer_At(GenericDeque_t* cb, int32_t Index);

/// @brief Retrieve the newest element inserted at the FRONT
/// @param cb Pointer to the circular buffer
/// @return Pointer to the FRONT element
GenericElement_t* CBuffer_Front(GenericDeque_t* cb);

/// @brief Retrieve the oldest element residing at the BACK
/// @param cb Pointer to the circular buffer
/// @return Pointer to the BACK element
GenericElement_t* CBuffer_Back(GenericDeque_t* cb);

/* Maintenance *******************************************************************************************************************/

/// @brief Clear all elements and reset indices without freeing the buffer object
/// @param cb Pointer to the circular buffer
void CBuffer_Clear(GenericDeque_t* cb);

#endif /* __DEQUE_H__ */