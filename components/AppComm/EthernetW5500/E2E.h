#ifndef __E2E_H__
#define __E2E_H__

#include "Network.h"

/**
 * @brief E2E Profile Definitions:
 * * 1. Checksum:
 * - Computed by summing the payload (excluding the last 2 bytes), 
 * then applying 2's complement.
 * - Located at the last byte of the payload.
 * * 2. Alive Counter:
 * - Located at the byte immediately preceding the last byte (penultimate).
 * * 3. Special Frame Pattern:
 * - Defined by a fixed 32-byte pattern with 8 embedded variable bytes.
 * - Pattern Structure:
 * [0..3]   0xFF 0xFF 0xFF 0xFF
 * [4..7]   <VariableBytes 0-3>
 * [8..11]  0xFF 0xFF 0xFF 0xFF
 * [12..15] <VariableBytes 4-7>
 * [16..23] 0xEF 0xFE 0xAF 0xFB 0x13 0x42 0x59 0x98
 * [24..31] 0xAC 0xBD 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF
 */

extern Byte_t       PatternArray[];
extern EthSize_t    VariableByteIndex[];

/// @brief Calculate arithmetic sum of buffer
/// @param Arr Pointer to data
/// @param Size Data size
/// @return 8-bit sum
Byte_t CalcSum8(GenericPtr_t Arr, EthSize_t Size);

/// @brief Calculate 2's complement checksum
/// @param Arr Pointer to data
/// @param Size Data size
/// @return Checksum result
Byte_t CalcCS8(GenericPtr_t Arr, EthSize_t Size);

/// @brief Extract checksum from payload end
/// @param Arr Pointer to payload
/// @param Size Payload size
/// @return Checksum byte
Byte_t ExtractCS8(GenericPtr_t Arr, EthSize_t Size);

/// @brief Extract alive counter from payload
/// @param Arr Pointer to payload
/// @param Size Payload size
/// @return Alive counter byte
Byte_t ExtractAC8(GenericPtr_t Arr, EthSize_t Size);

/// @brief Update checksum in payload
/// @param Arr Pointer to payload
/// @param Size Payload size
/// @return STAT_OKE on success
ReturnCode_t UpdateCS8(GenericPtr_t Arr, EthSize_t Size);

/// @brief Increment and update alive counter
/// @param Arr Pointer to payload
/// @param Size Payload size
/// @return STAT_OKE on success
ReturnCode_t UpdateAC8(GenericPtr_t Arr, EthSize_t Size);

/// @brief Retrieve 8 variable bytes from pattern
/// @param Payload Pointer to source frame
/// @param Size Frame size
/// @param VarByte Buffer to store 8 bytes
/// @return STAT_OKE on success
ReturnCode_t GetVariableByte(void* Payload, EthSize_t Size, void* VarByte);

/// @brief Write 8 variable bytes into pattern
/// @param Payload Pointer to destination frame
/// @param Size Frame size
/// @param VarByte Source 8-byte buffer
/// @return STAT_OKE on success
ReturnCode_t SetVariableByte(void* Payload, EthSize_t Size, void* VarByte);

/// @brief Verify if the fixed bytes in the payload match the predefined pattern
/// @param Data Pointer to the received payload buffer
/// @param Size Total size of the payload (should be at least 32 bytes)
/// @return STAT_OKE if the pattern matches, STAT_ERR if mismatch, or error code
ReturnCode_t VerifyFixedByte(GenericPtr_t Data, EthSize_t Size);

#endif /// __E2E_H__