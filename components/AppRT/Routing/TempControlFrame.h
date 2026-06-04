#ifndef __TEMPORARY_CONTROL_FRAME_H__
#define __TEMPORARY_CONTROL_FRAME_H__

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "__CommonHeaders.h"

/*
 * @brief   Enumeration defining CCU main commands.
 */
typedef enum {
    CMD_SYS_CTRL = 0xFE000000,
    CMD_ENG_CTRL = 0xFE00A000
} ccu_command_t;

/*
 * @brief   Enumeration defining system state arguments.
 */
typedef enum {
    ARG_SYS_START = 0xFE00A0AB,
    ARG_SYS_STOP  = 0xFE00A0CD
} sys_state_t;

/*
 * @brief   Enumeration defining engine direction states.
 */
typedef enum {
    ARG_ENG_NOT_STARTED = 0xFE00A000,
    ARG_ENG_FWD         = 0xFE00A001,
    ARG_ENG_BWD         = 0xFE00A002
} eng_dir_t;

/*
 * @brief   Struct representing the 44-byte network header.
 */
typedef struct __attribute__((packed)) {
    char name[32];
    uint8_t mac[6];
    uint16_t port;
    uint8_t ip[4];
} ccu_header_t;

/*
 * @brief   Struct representing arguments for System Control.
 */
typedef struct __attribute__((packed)) {
    uint32_t fl_state;
    uint32_t fr_state;
    uint32_t rl_state;
    uint32_t rr_state;
} sys_ctrl_payload_t;

/*
 * @brief   Struct representing arguments for Engine Control.
 */
typedef struct __attribute__((packed)) {
    uint32_t left_dir;
    uint32_t left_speed;
    uint32_t right_dir;
    uint32_t right_speed;
} eng_ctrl_payload_t;

/*
 * @brief   Union wrapping various payload formats.
 */
typedef union {
    sys_ctrl_payload_t sys_ctrl;
    eng_ctrl_payload_t eng_ctrl;
    uint32_t raw_args[4];
} payload_info_t;

/*
 * @brief   Struct representing the complete 64-byte CCU packet.
 */
typedef struct __attribute__((packed)) {
    ccu_header_t header;
    uint32_t command;
    payload_info_t info;
} ccu_packet_t;

/*
 * @brief   Parses the incoming UDP packet and extracts telemetry commands.
 * @param   raw_data Pointer to the received byte array.
 * @param   len Total length of the received byte array.
 * @return  None.
 */
void ParseCCUPacket(const uint8_t *raw_data, size_t len);

#endif /*__TEM*/