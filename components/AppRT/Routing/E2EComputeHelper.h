#ifndef __E2E_COMPUTE_HELPER_H__
#define __E2E_COMPUTE_HELPER_H__

#include <stdint.h>

/* CRC16 Polynomial Definitions */
#define CRC16_DNP       0x3D65      /* DNP, IEC 870, M-BUS, wM-BUS, ... */
#define CRC16_CCITT     0x1021      /* X.25, V.41, HDLC FCS, Bluetooth, ... */
#define CRC16_IBM       0x8005      /* ModBus, USB, Bisync, CRC-16, CRC-16-ANSI, ... */
#define CRC16_T10_DIF   0x8BB7      /* SCSI DIF */
#define CRC16_DECT      0x0589      /* Cordeless Telephones */
#define CRC16_ARINC     0xA02B      /* ACARS Aplications */

/**
 * @brief Computes the 8-bit sum of a data buffer.
 * @param Data Pointer to the data buffer.
 * @param ByteCount Number of bytes to process.
 * @return Computed sum.
 */
uint16_t ComputeSum8(void * const Data, int32_t const ByteCount);

/**
 * @brief Computes the 8-bit two's complement checksum of a data buffer.
 * @param Data Pointer to the data buffer.
 * @param ByteCount Number of bytes to process.
 * @return Computed checksum.
 */
uint16_t ComputeCheckSum8(void * const Data, int32_t const ByteCount);

/**
 * @brief Computes the CRC16 for a given data buffer using a specified polynomial.
 * @param Data Pointer to the data buffer.
 * @param ByteCount Number of bytes to process.
 * @param Polynomial The CRC16 polynomial to apply.
 * @return Computed CRC16 value.
 */
uint16_t ComputeCRC16(void * const Data, int32_t const ByteCount, uint16_t const Polynomial);

/**
 * @brief Computes the CRC16 IBM for a data buffer.
 * @param Data Pointer to the data buffer.
 * @param ByteCount Number of bytes to process.
 * @return Computed CRC16 IBM value.
 */
uint16_t ComputeCRC16_IBM(void * const Data, int32_t const ByteCount); 

/**
 * @brief Computes the CRC16 T10 DIF for a data buffer.
 * @param Data Pointer to the data buffer.
 * @param ByteCount Number of bytes to process.
 * @return Computed CRC16 T10 DIF value.
 */
uint16_t ComputeCRC16_T10_DIF(void * const Data, int32_t const ByteCount); 

/**
 * @brief Computes the CRC16 DECT for a data buffer.
 * @param Data Pointer to the data buffer.
 * @param ByteCount Number of bytes to process.
 * @return Computed CRC16 DECT value.
 */
uint16_t ComputeCRC16_DECT(void * const Data, int32_t const ByteCount); 

/**
 * @brief Computes the CRC16 ARINC for a data buffer.
 * @param Data Pointer to the data buffer.
 * @param ByteCount Number of bytes to process.
 * @return Computed CRC16 ARINC value.
 */
uint16_t ComputeCRC16_ARINC(void * const Data, int32_t const ByteCount); 

/**
 * @brief Computes the CRC16 CCITT for a data buffer.
 * @param Data Pointer to the data buffer.
 * @param ByteCount Number of bytes to process.
 * @return Computed CRC16 CCITT value.
 */
uint16_t ComputeCRC16_CCITT(void * const Data, int32_t const ByteCount); 

/**
 * @brief Computes the CRC16 DNP for a data buffer.
 * @param Data Pointer to the data buffer.
 * @param ByteCount Number of bytes to process.
 * @return Computed CRC16 DNP value.
 */
uint16_t ComputeCRC16_DNP(void * const Data, int32_t const ByteCount); 

#endif /* __E2E_COMPUTE_HELPER_H__ */