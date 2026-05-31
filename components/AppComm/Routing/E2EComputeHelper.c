#include "E2EComputeHelper.h"

uint16_t ComputeSum8(void * const Data, int32_t const ByteCount) {
    uint16_t sum = 0;
    uint8_t *ptr = (uint8_t *)Data;
    int32_t i = 0;

    /* Iterate through all bytes in the buffer */
    while (i < ByteCount) {
        sum += ptr[i];
        i++;
    }

    /* Return the accumulated sum */
    return sum;
}

uint16_t ComputeCheckSum8(void * const Data, int32_t const ByteCount) {
    uint16_t sum;
    uint16_t checksum;

    sum = ComputeSum8(Data, ByteCount);
    
    /* Calculate the 8-bit two's complement of the sum */
    checksum = (uint16_t)((~sum + 1) & 0xFF);

    /* Return the calculated checksum */
    return checksum;
}

uint16_t ComputeCRC16(void * const Data, int32_t const ByteCount, uint16_t const Polynomial) {
    uint16_t crcValue = 0x0000;
    uint8_t *ptr = (uint8_t *)Data;
    int32_t byteIndex = 0;
    uint8_t bitIndex = 0;
    uint8_t newByte = 0;

    /* Iterate through the entire data buffer */
    while (byteIndex < ByteCount) {
        newByte = ptr[byteIndex];

        /* Process all 8 bits of the current byte */
        for (bitIndex = 0; bitIndex < 8; bitIndex++) {
            
            /* Check if the MSB of CRC XORed with MSB of data byte is 1 */
            if (((crcValue & 0x8000) >> 8) ^ (newByte & 0x80)) {
                crcValue = (crcValue << 1) ^ Polynomial;
            } 
            /* Otherwise, only shift the CRC value left */
            else {
                crcValue = (crcValue << 1);
            }

            newByte <<= 1;
        }
        
        byteIndex++;
    }

    /* Return the final CRC16 value */
    return crcValue;
}

uint16_t ComputeCRC16_IBM(void * const Data, int32_t const ByteCount) {
    uint16_t crcResult;
    
    crcResult = ComputeCRC16(Data, ByteCount, CRC16_IBM);
    
    /* Return the computed CRC16 IBM value */
    return crcResult;
}

uint16_t ComputeCRC16_T10_DIF(void * const Data, int32_t const ByteCount) {
    uint16_t crcResult;
    
    crcResult = ComputeCRC16(Data, ByteCount, CRC16_T10_DIF);
    
    /* Return the computed CRC16 T10 DIF value */
    return crcResult;
}

uint16_t ComputeCRC16_DECT(void * const Data, int32_t const ByteCount) {
    uint16_t crcResult;
    
    crcResult = ComputeCRC16(Data, ByteCount, CRC16_DECT);
    
    /* Return the computed CRC16 DECT value */
    return crcResult;
}

uint16_t ComputeCRC16_ARINC(void * const Data, int32_t const ByteCount) {
    uint16_t crcResult;
    
    crcResult = ComputeCRC16(Data, ByteCount, CRC16_ARINC);
    
    /* Return the computed CRC16 ARINC value */
    return crcResult;
}

uint16_t ComputeCRC16_CCITT(void * const Data, int32_t const ByteCount) {
    uint16_t crcResult;
    
    crcResult = ComputeCRC16(Data, ByteCount, CRC16_CCITT);
    
    /* Return the computed CRC16 CCITT value */
    return crcResult;
}

uint16_t ComputeCRC16_DNP(void * const Data, int32_t const ByteCount) {
    uint16_t crcResult;
    
    crcResult = ComputeCRC16(Data, ByteCount, CRC16_DNP);
    
    /* Return the computed CRC16 DNP value */
    return crcResult;
}