#include "E2E.h"

Byte_t PatternArray[] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
    0xEF, 0xFE, 0xAF, 0xFB, 0x13, 0x42, 0x59, 0x98,
    0xAC, 0xBD, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

EthSize_t VariableByteIndex[] = {4, 5, 6, 7, 12, 13, 14, 15};

/// @brief Calculate the arithmetic sum of all bytes in a buffer
/// @param Arr Pointer to the data buffer
/// @param Size Number of bytes to process
/// @return The 8-bit sum (modulo 256)
Byte_t CalcSum8(GenericPtr_t Arr, EthSize_t Size) {
    /* Calculate sum using a 32-bit accumulator to prevent overflow during addition */
    Byte_t* data = Arr.Byte;
    uint32_t sum = 0;
    for (EthSize_t i = 0; i < Size; i++) {
        sum += data[i];
    }
    return (Byte_t)(sum & 0xFF);
}

/// @brief Calculate the 2's complement checksum
/// @param Arr Pointer to the data buffer
/// @param Size Number of bytes to process
/// @return The calculated checksum byte
Byte_t CalcCS8(GenericPtr_t Arr, EthSize_t Size) {
    /* Apply 2's complement: invert bits and add one */
    return (Byte_t)(~CalcSum8(Arr, Size) + 1);
}

/// @brief Extract the checksum byte from the end of the payload
/// @param Arr Pointer to the payload
/// @param Size Total size of the payload
/// @return The checksum byte found at the last index
Byte_t ExtractCS8(GenericPtr_t Arr, EthSize_t Size) {
    /* Checksum is stored in the final byte of the array */
    return Arr.Byte[Size - 1];
}

/// @brief Extract the alive counter byte
/// @param Arr Pointer to the payload
/// @param Size Total size of the payload
/// @return The alive counter byte found before the checksum
Byte_t ExtractAC8(GenericPtr_t Arr, EthSize_t Size) {
    /* Alive counter is located at the second to last byte */
    if (Size < 2) {
        return 0;
    }
    else {
        return Arr.Byte[Size - 2];
    }
}

/// @brief Compute and update the checksum byte in the payload
/// @param Arr Pointer to the payload buffer
/// @param Size Total size of the payload
/// @return STAT_OKE if successful, error code otherwise
ReturnCode_t UpdateCS8(GenericPtr_t Arr, EthSize_t Size) {
    /* Validate input and update the last byte with a new checksum */
    if (Arr.Byte == NULL) {
        return STAT_ERR_NULL;
    }
    else if (Size < 1) {
        return STAT_ERR_INVALID_SIZE;
    }
    else {
        Arr.Byte[Size - 1] = CalcCS8(Arr, Size - 1);
        return STAT_OKE;
    }
}

/// @brief Increment and update the alive counter in the payload
/// @param Arr Pointer to the payload buffer
/// @param Size Total size of the payload
/// @return STAT_OKE if successful, error code otherwise
ReturnCode_t UpdateAC8(GenericPtr_t Arr, EthSize_t Size) {
    /* Increment the byte at the penultimate position */
    if (Arr.Byte == NULL) {
        return STAT_ERR_NULL;
    }
    else if (Size < 2) {
        return STAT_ERR_INVALID_SIZE;
    }
    else {
        Arr.Byte[Size - 2]++;
        return STAT_OKE;
    }
}



/// @brief Retrieve 8 variable bytes from the special frame pattern
/// @param Payload Pointer to the source frame
/// @param Size Size of the payload
/// @param VarByte Pointer to the destination 8-byte buffer
/// @return STAT_OKE if successful, error code otherwise
ReturnCode_t GetVariableByte(void* Payload, EthSize_t Size, void* VarByte) {
    /* Map payload bytes to output buffer using predefined index mapping */
    if (Payload == NULL || VarByte == NULL) {
        return STAT_ERR_NULL;
    }
    else {
        Byte_t* p = (Byte_t*)Payload;
        Byte_t* v = (Byte_t*)VarByte;
        for (int i = 0; i < 8; i++) {
            v[i] = p[VariableByteIndex[i]];
        }
        return STAT_OKE;
    }
}

/// @brief Write 8 variable bytes into the special frame pattern
/// @param Payload Pointer to the destination frame
/// @param Size Size of the payload
/// @param VarByte Pointer to the source 8-byte buffer
/// @return STAT_OKE if successful, error code otherwise
ReturnCode_t SetVariableByte(void* Payload, EthSize_t Size, void* VarByte) {
    /* Insert external variable data into the specific pattern offsets */
    if (Payload == NULL || VarByte == NULL) {
        return STAT_ERR_NULL;
    }
    else {
        Byte_t* p = (Byte_t*)Payload;
        Byte_t* v = (Byte_t*)VarByte;
        for (int i = 0; i < 8; i++) {
            p[VariableByteIndex[i]] = v[i];
        }
        return STAT_OKE;
    }
}

/// @brief Verify if the fixed bytes in the payload match the predefined pattern
/// @param Data Pointer to the received payload buffer
/// @param Size Total size of the payload (should be at least 32 bytes)
/// @return STAT_OKE if the pattern matches, STAT_ERR if mismatch, or error code
ReturnCode_t VerifyFixedByte(GenericPtr_t Data, EthSize_t Size) {
    /* Ensure the data buffer is valid and meets the minimum pattern length */
    if (Data.Byte == NULL) {
        return STAT_ERR_NULL;
    }
    else if (Size < 32) {
        return STAT_ERR_INVALID_SIZE;
    }
    else {
        /* Iterate through the 32-byte pattern array */
        for (EthSize_t i = 0; i < 32; i++) {
            uint8_t isVariable = 0;

            /* Check if the current index is one of the 8 variable bytes */
            for (int j = 0; j < 8; j++) {
                if (i == VariableByteIndex[j]) {
                    isVariable = 1;
                    break;
                }
            }

            /* If it is a fixed byte, compare it with the pattern */
            if (!isVariable) {
                if (Data.Byte[i] != PatternArray[i]) {
                    return STAT_ERR; /* Pattern mismatch detected */
                }
            }
        }
        return STAT_OKE;
    }
}