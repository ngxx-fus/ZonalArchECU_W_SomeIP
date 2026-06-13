#include "Deque.h"
#include <stdlib.h>
#include <string.h>

/// @brief Helper to manage array reallocation or eviction when pushing elements
/// @param cb Pointer to the circular buffer
static void CBuffer_EnsureCapacity(GenericDeque_t* cb) {
    /* Handle buffer bounds by evicting the oldest element or attempting to reallocate */
    if (cb->Size >= CBUFFER_MAX_SIZE) {
        /* Trigger IsFull event before forcing an eviction */
        if (cb->OnChangeCallBack != NULL) {
            cb->OnChangeCallBack(eCBuff_IsFull);
        }
        CBuffer_PopBack(cb);
    }
    else if (cb->Size == cb->ReservedSize) {
        CBuffSize_t newCap = cb->ReservedSize + (cb->ReservedSize * 15 / 100);
        
        if (newCap <= cb->ReservedSize) {
            newCap = cb->ReservedSize + 1;
        }
        if (newCap > CBUFFER_MAX_SIZE) {
            newCap = CBUFFER_MAX_SIZE;
        }
        
        GenericElement_t* newArr = calloc(newCap, sizeof(GenericElement_t));
        if (newArr != NULL) {
            for (CBuffSize_t i = 0; i < cb->Size; i++) {
                newArr[i] = cb->Arr[(cb->Tail + i) % cb->ReservedSize];
            }
            free(cb->Arr);
            cb->Arr = newArr;
            cb->Tail = 0;
            cb->Head = cb->Size;
            cb->ReservedSize = newCap;

            /* Trigger ReAllocate event upon successful capacity expansion */
            if (cb->OnChangeCallBack != NULL) {
                cb->OnChangeCallBack(eCBuff_ReAllocate);
            }
        }
        else {
            CBuffer_PopBack(cb);
        }
    }
}

GenericDeque_t* CBuffer_Create(CBuffSize_t ReservedSize) {
    /* Allocate the parent struct and initialize the internal array with zeroed memory */
    if (ReservedSize == 0) {
        ReservedSize = 1;
    }
    if (ReservedSize > CBUFFER_MAX_SIZE) {
        ReservedSize = CBUFFER_MAX_SIZE;
    }
    
    GenericDeque_t* cb = malloc(sizeof(GenericDeque_t));
    if (cb != NULL) {
        cb->Arr = calloc(ReservedSize, sizeof(GenericElement_t));
        if (cb->Arr != NULL) {
            cb->ReservedSize = ReservedSize;
            cb->Size = 0;
            cb->Head = 0;
            cb->Tail = 0;
            cb->OnChangeCallBack = NULL; /* Explicitly set callback to NULL upon creation */
        }
        else {
            free(cb);
            cb = NULL;
        }
    }
    
    return cb;
}

void CBuffer_Destroy(GenericDeque_t* cb) {
    /* Safely release all payloads, the array, and the object itself */
    if (cb != NULL) {
        CBuffer_Clear(cb);
        if (cb->Arr != NULL) {
            free(cb->Arr);
        }
        free(cb);
    }
}

bool CBuffer_IsEmpty(const GenericDeque_t* cb) {
    /* Check logical state to determine if the deque has zero elements */
    return (cb == NULL || cb->Size == 0);
}

bool CBuffer_IsFull(const GenericDeque_t* cb) {
    /* Validate if the deque size has hit the absolute defined maximum */
    return (cb != NULL && cb->Size >= CBUFFER_MAX_SIZE);
}

CBuffSize_t CBuffer_Size(const GenericDeque_t* cb) {
    /* Retrieve the active quantity of inserted elements */
    if (cb != NULL) {
        return cb->Size;
    }
    else {
        return 0;
    }
}

void CBuffer_SetCallback(GenericDeque_t* cb, void (*CallBack)(enum OnChangeType Type)) {
    /* Assign the user-provided callback function */
    if (cb != NULL) {
        cb->OnChangeCallBack = CallBack;
    }
}

void CBuffer_ClearCallback(GenericDeque_t* cb) {
    /* Remove the currently active callback */
    if (cb != NULL) {
        cb->OnChangeCallBack = NULL;
    }
}

CBuffSize_t CBuffer_PushFront(GenericDeque_t* cb, const CBuffBase_t Data, CBuffSize_t DataSize) {
    /* Inject data into the next head slot and advance the head pointer safely */
    if (cb == NULL || Data == NULL || DataSize == 0) {
        return cb ? cb->Size : 0;
    }
    
    CBuffer_EnsureCapacity(cb);
    
    uint8_t* newPtr = malloc(DataSize);
    if (newPtr == NULL) {
        return cb->Size;
    }
    
    if (cb->Arr[cb->Head].DataPtr != NULL) {
        free(cb->Arr[cb->Head].DataPtr);
    }
    
    cb->Arr[cb->Head].DataPtr = newPtr;
    memcpy(cb->Arr[cb->Head].DataPtr, Data, DataSize);
    cb->Arr[cb->Head].DataSize = DataSize;
    cb->Head = (cb->Head + 1) % cb->ReservedSize;
    cb->Size++;
    
    /* Notify application about the PushFront event */
    if (cb->OnChangeCallBack != NULL) {
        cb->OnChangeCallBack(eCBuff_PushFront);
    }
    
    return cb->Size;
}

CBuffSize_t CBuffer_PushBack(GenericDeque_t* cb, const CBuffBase_t Data, CBuffSize_t DataSize) {
    /* Steer the tail pointer backward and insert data safely into the new tail slot */
    if (cb == NULL || Data == NULL || DataSize == 0) {
        return cb ? cb->Size : 0;
    }
    
    CBuffer_EnsureCapacity(cb);
    
    CBuffSize_t newTail = (cb->Tail + cb->ReservedSize - 1) % cb->ReservedSize;
    uint8_t* newPtr = malloc(DataSize);
    if (newPtr == NULL) {
        return cb->Size;
    }
    
    if (cb->Arr[newTail].DataPtr != NULL) {
        free(cb->Arr[newTail].DataPtr);
    }
    
    cb->Arr[newTail].DataPtr = newPtr;
    memcpy(cb->Arr[newTail].DataPtr, Data, DataSize);
    cb->Arr[newTail].DataSize = DataSize;
    cb->Tail = newTail;
    cb->Size++;
    
    /* Notify application about the PushBack event */
    if (cb->OnChangeCallBack != NULL) {
        cb->OnChangeCallBack(eCBuff_PushBack);
    }
    
    return cb->Size;
}

CBuffSize_t CBuffer_PopFront(GenericDeque_t* cb) {
    /* Isolate and eliminate the most recent element from the front */
    if (cb == NULL || cb->Size == 0) {
        return cb ? cb->Size : 0;
    }
    
    CBuffSize_t frontIdx = (cb->Head + cb->ReservedSize - 1) % cb->ReservedSize;
    if (cb->Arr[frontIdx].DataPtr != NULL) {
        free(cb->Arr[frontIdx].DataPtr);
        cb->Arr[frontIdx].DataPtr = NULL;
    }
    cb->Arr[frontIdx].DataSize = 0;
    
    cb->Head = frontIdx;
    cb->Size--;
    
    /* Notify application about the PopFront event */
    if (cb->OnChangeCallBack != NULL) {
        cb->OnChangeCallBack(eCBuff_PopFront);
    }
    
    return cb->Size;
}

CBuffSize_t CBuffer_PopBack(GenericDeque_t* cb) {
    /* Strip away the oldest element existing at the back boundary */
    if (cb == NULL || cb->Size == 0) {
        return cb ? cb->Size : 0;
    }
    
    if (cb->Arr[cb->Tail].DataPtr != NULL) {
        free(cb->Arr[cb->Tail].DataPtr);
        cb->Arr[cb->Tail].DataPtr = NULL;
    }
    cb->Arr[cb->Tail].DataSize = 0;
    
    cb->Tail = (cb->Tail + 1) % cb->ReservedSize;
    cb->Size--;
    
    /* Notify application about the PopBack event */
    if (cb->OnChangeCallBack != NULL) {
        cb->OnChangeCallBack(eCBuff_PopBack);
    }
    
    return cb->Size;
}

GenericElement_t* CBuffer_At(GenericDeque_t* cb, int32_t Index) {
    /* Compute logical mapping matching deque order and extract the specific array item */
    if (cb == NULL || cb->Size == 0) {
        return NULL;
    }
    
    int32_t logicalIdx = Index;
    if (logicalIdx < 0) {
        logicalIdx += cb->Size;
    }
    
    if (logicalIdx < 0 || logicalIdx >= cb->Size) {
        return NULL;
    }
    
    CBuffSize_t physicalIdx = (cb->Head + cb->ReservedSize - 1 - logicalIdx) % cb->ReservedSize;
    return &cb->Arr[physicalIdx];
}

GenericElement_t* CBuffer_Front(GenericDeque_t* cb) {
    /* Relay the extraction request for index zero directly */
    return CBuffer_At(cb, 0);
}

GenericElement_t* CBuffer_Back(GenericDeque_t* cb) {
    /* Relay the extraction request targeting the rear end index */
    return CBuffer_At(cb, -1);
}

void CBuffer_Clear(GenericDeque_t* cb) {
    /* Cycle through and wipe off every internal node to restabilize buffer */
    if (cb != NULL) {
        bool changed = (cb->Size > 0);
        
        while (cb->Size > 0) {
            if (cb->Arr[cb->Tail].DataPtr != NULL) {
                free(cb->Arr[cb->Tail].DataPtr);
                cb->Arr[cb->Tail].DataPtr = NULL;
            }
            cb->Arr[cb->Tail].DataSize = 0;
            cb->Tail = (cb->Tail + 1) % cb->ReservedSize;
            cb->Size--;
        }
        cb->Head = 0;
        cb->Tail = 0;
        
        /* Notify application about the bulk clear event once at the end */
        if (changed && cb->OnChangeCallBack != NULL) {
            cb->OnChangeCallBack(eCBuff_Clear);
        }
    }
}