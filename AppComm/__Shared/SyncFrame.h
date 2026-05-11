#ifndef SYNC_FRAME_H
#define SYNC_FRAME_H

#include <stdint.h>
#include <stdbool.h>

#ifndef SYNC_FR_LEN
    /** @brief Define SYNC frame length */
    #define SYNC_FR_LEN     512U
#endif

/**
 * @brief Sensor distance data packed into 32-bit structure.
 * Total: 32 bits (4 bytes).
 */
typedef union {
    struct __attribute__((packed)) {
        uint32_t D0      : 10; /* Distance sensor 0 */
        uint32_t D1      : 10; /* Distance sensor 1 */
        uint32_t D2      : 10; /* Distance sensor 2 */
        uint32_t SyncNum : 2;  /* Synchronization number */
    } Fields;
    uint8_t RawByte[4];
} SensorDistance_t;

/**
 * @brief Actual motor state data packed into 32-bit structure.
 * Total: 32 bits (4 bytes).
 */
typedef union {
    struct __attribute__((packed)) {
        uint32_t M0       : 10; /* Motor sensor 0 */
        uint32_t M1       : 10; /* Motor sensor 1 */
        uint32_t SyncNum  : 2;  /* Synchronization number */
        uint32_t Reserved : 10; /* Reserved bits for alignment */
    } Fields;
    uint8_t RawByte[4];
} ActualMotor_t;

/**
 * @brief Combined ECU State for Fail-safe Zonal Architecture.
 * This structure aggregates distance and motor data.
 */
typedef struct __attribute__((packed)) {
    SensorDistance_t Distance;
    ActualMotor_t    Motor;
} SF_ECUState_t;

/**
 * @brief Dynamic Header option field (2 Bytes).
 */
typedef union {
    /**
     * @brief Control field for multi-frame transmission.
     * Total: 16 bits (2 bytes).
     */
    struct __attribute__((packed)) {
        uint16_t NoResponse : 1;  /* 1: No response required, 0: Response required */
        uint16_t FirstFrame : 1;  /* 1: Start of multi-frame sequence */
        uint16_t SeqNumber  : 5;  /* Sequence number of consecutive frames (0-31) */
        uint16_t CycleTime  : 9;  /* Interval between consecutive frames (ms or us) */
    } Ctrl;

    struct __attribute__((packed)) {
        uint8_t AliveCounter;
        uint8_t CheckSum;
    } E2E;

    uint16_t Raw;
} FrameOption_t;

/**
 * @brief Optimized Sync Frame with 16-bit Payload Length.
 */
typedef union {
    struct __attribute__((packed)) {
        struct __attribute__((packed)) {
            uint16_t      ID;       /* 2 Bytes: Frame Identifier */
            uint16_t      Length;   /* 2 Bytes: Supports up to 65535 bytes */
            FrameOption_t Type;     /* 2 Bytes: Control or E2E */
        } Header;                   /* Total Header: 6 Bytes */

        /* Payload calculation: 512 - 6 (Header) - 2 (Trailer) = 504 bytes */
        uint8_t Payload[SYNC_FR_LEN - 6 - 2];

        struct __attribute__((packed)) {
            uint16_t CRC;           /* 2 Bytes: Frame Check Sequence */
        } Trailer;
    } Member;

    uint8_t Raw[SYNC_FR_LEN];
} SyncFrame_t;

#endif /* SYNC_FRAME_H */