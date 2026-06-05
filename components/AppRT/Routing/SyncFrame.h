#ifndef SYNC_FRAME_H
#define SYNC_FRAME_H

#include <stdint.h>
#include <stdbool.h>

#ifndef SYNC_FR_LEN
    /** @brief Define SYNC frame length */
    #define SYNC_FR_LEN     512U
#endif

/*
 * @brief Union representing Electronic Control Unit (ECU) network information.
 * Facilitates easy switching between structured field access and raw byte transmission.
 */
typedef union {
    /*
     * @brief Packed structure containing specific ECU configuration parameters.
     * Memory alignment is disabled to ensure exact byte mapping for network protocols.
     */
    struct __attribute__((packed)) Fields_t {
        char     ECUName[32];         /*< Null-terminated string identifying the ECU. */
        uint64_t EthMACAddress : 48;  /*< 48-bit Hardware Ethernet MAC Address. */
        uint16_t IPv4DefPort;         /*< 16-bit Default Communication Port. */
        uint32_t IPv4Address;         /*< 32-bit Network IPv4 Address. */
    } Fields;
    
    uint8_t RawByte[sizeof(struct Fields_t)]; /*< Raw byte array for direct serialization. */
} ECUInfo_t;

/*
 * @brief Global constant instance of the ECU network configuration.
 * Defined in an external source file, providing read-only access to device identity.
 */
extern const ECUInfo_t ECUInfo;

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
    ECUInfo_t           ECUInfo;
    SensorDistance_t    Distance;
    ActualMotor_t       Motor;
} SF_ECUState_t;

/**
 * @brief Union for SF Motor Control data handling.
 * Total size is fixed at 504 bytes.
 */
typedef union SF_MotorCtl_t {
    struct __attribute__((packed)) {
        uint64_t        Pattern0        : 64;  ///< Pattern0: 0xFF00FFFF000055AA
        uint32_t        M0              : 10;
        uint32_t        M1              : 10;
        uint32_t        ReservedBits    : 12;  ///< Padding to align to next byte
        uint64_t        Pattern1        : 64;  ///< Pattern1: 0xBBCCDDDDEEEE2233
        uint64_t        Pattern2        : 64;  ///< Pattern2: 0x1144555567889999
        
        /* Calculate remaining bytes: 504 - (8 + 4 + 8 + 8) = 476 bytes */
        uint8_t         Reserved[476];         ///< Padding to reach 504 bytes
    };
    uint8_t RawByte[504];
} SF_MotorCtl_t;

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