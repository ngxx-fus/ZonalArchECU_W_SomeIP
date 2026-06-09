#ifndef __TEMPORARY_CONTROL_FRAME_H__
#define __TEMPORARY_CONTROL_FRAME_H__

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "__CommonHeaders.h"

/*
+-------------------------------------------------------------------------------+
| SUB-STRUCTURE: CCU HEADER (ccu_header_t) - 44 Bytes Total                     |
+-------------------+-------------------+-------------------+-------------------+
| Name (Identifier) | MAC Address       | Port Number       | IP Address        |
| (char[32])        | (uint8_t[6])      | (uint16_t)        | (uint8_t[4])      |
| 32 Bytes          | 6 Bytes           | 2 Bytes           | 4 Bytes           |
+-------------------+-------------------+-------------------+-------------------+
| Notes: Precedes every command frame. Ensures network routing & identification.|
+-------------------------------------------------------------------------------+

+-------------------------------------------------------------------------------------------------------+
| FRAME: SYSTEM CONTROL (Total: 64 Bytes)                                                               |
+-----------------------+-----------------------+-------------------------------------------------------+
| Header                | Command               | Payload (sys_ctrl_payload_t)                          |
| 44 Bytes              | 4 Bytes               | 16 Bytes (4 x 4-byte arguments)                       |
| (ccu_header_t)        | Fixed: 0xFE000000     | Arg0       | Arg1       | Arg2       | Arg3       |
|                       | (CMD_SYS_CTRL)        | fl_state   | fr_state   | rl_state   | rr_state   |
|                       |                       | (4 Bytes)  | (4 Bytes)  | (4 Bytes)  | (4 Bytes)  |
+-----------------------+-----------------------+------------+------------+------------+------------+
| Valid Values / Implementation Notes:                                                                  |
| - Command:  Fixed value `0xFE000000` indicates a System Control command.                              |
| - fl_state: Expected values are `0xFE00A0AB` (ARG_SYS_START) or `0xFE00A0CD` (ARG_SYS_STOP).          |
| - fr/rl/rr: Currently reserved for Front-Right, Rear-Left, Rear-Right states (logic not implemented). |
+-------------------------------------------------------------------------------------------------------+

+-------------------------------------------------------------------------------------------------------+
| FRAME: ENGINE CONTROL (Total: 64 Bytes)                                                               |
+-----------------------+-----------------------+-------------------------------------------------------+
| Header                | Command               | Payload (eng_ctrl_payload_t)                          |
| 44 Bytes              | 4 Bytes               | 16 Bytes (4 x 4-byte arguments)                       |
| (ccu_header_t)        | Fixed: 0xFE00A000     | Arg0       | Arg1       | Arg2       | Arg3       |
|                       | (CMD_ENG_CTRL)        | left_dir   | left_speed | right_dir  | right_speed|
|                       |                       | (4 Bytes)  | (4 Bytes)  | (4 Bytes)  | (4 Bytes)  |
+-----------------------+-----------------------+------------+------------+------------+------------+
| Valid Values / Implementation Notes:                                                                  |
| - Command:     Fixed value `0xFE00A000`.                                                              |
| - left_dir:    `0xFE00A000` (NOT_STARTED), `0xFE00A001` (FWD), `0xFE00A002` (BWD).                    |
| - left_speed:  Base offset `0xFE00A100` + integer speed value.                                        |
| - right_dir:   `0xFE00A000` (NOT_STARTED), `0xFE00A001` (FWD), `0xFE00A002` (BWD).                    |
| - right_speed: Base offset `0xFE00A100` + integer speed value.                                        |
| * Note: Speed is parsed as `(int32_t)(speed - 0xFE00A100)` and mapped via ConvertPercentToRaw().      |
+-------------------------------------------------------------------------------------------------------+

+-------------------------------------------------------------------------------------------------------+
| FRAME: CCU REGISTRATION (Total: 64 Bytes)                                                             |
+-----------------------+-----------------------+-------------------------------------------------------+
| Header                | Command               | Payload (ccu_reg_payload_t / raw_args)                |
| 44 Bytes              | 4 Bytes               | 16 Bytes (4 x 4-byte arguments)                       |
| (ccu_header_t)        | Fixed: 0xF100A000     | Arg0       | Arg1       | Arg2       | Arg3       |
|                       | (CMD_CCU_REGISTER)    | IP Address | Auth Num   | HB | Rsvd  | Checksum   |
|                       |                       | (4 Bytes)  | (4 Bytes)  | 2B | 2B    | (4 Bytes)  |
+-----------------------+-----------------------+------------+------------+----+-------+------------+
| Valid Values / Implementation Notes:                                                                  |
| - Command: Fixed value `0xF100A000` indicates a CCU Registration command sent from the CCU.           |
| - Arg0 (IP Address): 32-bit representation of the transmitting CCU's IPv4 address.                    |
| - Arg1 (Auth Number): Static authentication token, currently fixed at `0xAAAAAAAA`.                   |
| - Arg2 (Heartbeat & Reserved): Split into two 16-bit fields. The first 16 bits define the heartbeat   |
|   cycle time (cyctime), and the remaining 16 bits are reserved for future use.                        |
| - Arg3 (Checksum): 32-bit frame integrity checksum. Currently not implemented (can be ignored).       |
+-------------------------------------------------------------------------------------------------------+

*/


/*
 * @brief   Enumeration defining CCU main commands.
 */
typedef enum {
    CMD_SYS_CTRL        = 0xFE000000,
    CMD_ENG_CTRL        = 0xFE00A000,
    CMD_CCU_REGISTER    = 0xF100A000,
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