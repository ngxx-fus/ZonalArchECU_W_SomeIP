## @file CoreNetwork.py
## @brief Core networking structures and command builders.

import struct
import socket

# /*
#  * @brief Class representing a Zonal Electronic Control Unit (ECU).
#  * Holds fundamental network identity parameters.
#  */
class ZoneECU:
    def __init__(self, name="", ip="", port=0, mac=""):
        self.name = name
        self.ip = ip
        self.port = port
        self.mac = mac

# /*
#  * @brief Utility class to construct UDP command payloads adhering to CCU specifications.
#  * Ensures exact C-struct memory alignment and bit-packing constraints.
#  */
class CCUCommandBuilder:
    # /* Fixed Identity Parameters for the CCU */
    CCU_NAME = b"CentralControlUnit".ljust(32, b'\x00')
    CCU_MAC = bytes.fromhex("e466e5984fd7")
    CCU_PORT = 30490
    CCU_IP = socket.inet_aton("10.0.0.102")

    # /* System Control Commands */
    CMD_SYS_CTRL = 0xFE000000
    ARG_SYS_START = 0xFE00A0AB
    ARG_SYS_STOP = 0xFE00A0CD

    # /* Engine Control Commands */
    CMD_ENG_CTRL = 0xFE00A000
    ARG_ENG_NOT_STARTED = 0xFE00A000
    ARG_ENG_FWD = 0xFE00A001
    ARG_ENG_BWD = 0xFE00A002

    # /*
    #  * @brief Packs the unified ECU Info header and payload arguments.
    #  * @param command The 32-bit command identifier.
    #  * @param args A list containing four 32-bit arguments.
    #  * @return Byte array ready for UDP transmission.
    #  */
    @classmethod
    def build_frame(cls, command, args):
        header = struct.pack("<32s 6s H 4s", cls.CCU_NAME, cls.CCU_MAC, cls.CCU_PORT, cls.CCU_IP)
        payload = struct.pack("<I 4I", command, *args)
        return header + payload

    # /*
    #  * @brief Constructs a system-level start/stop command frame.
    #  * @param is_start Boolean indicating desired engine state.
    #  */
    @classmethod
    def build_sys_ctrl(cls, is_start):
        arg_val = cls.ARG_SYS_START if is_start else cls.ARG_SYS_STOP
        return cls.build_frame(cls.CMD_SYS_CTRL, [arg_val, arg_val, arg_val, arg_val])

    # /*
    #  * @brief Constructs a precise engine control frame mapping Left and Right motors.
    #  * @param val_l Left motor percentage (-100 to 100).
    #  * @param val_r Right motor percentage (-100 to 100).
    #  */
    @classmethod
    def build_eng_ctrl(cls, val_l, val_r):
        def parse_motor(v):
            if v == 0:
                return cls.ARG_ENG_NOT_STARTED, 0xFE00A100
            elif v > 0:
                return cls.ARG_ENG_FWD, 0xFE00A100 + v
            else:
                return cls.ARG_ENG_BWD, 0xFE00A100 + abs(v)
        
        dir_l, spd_l = parse_motor(val_l)
        dir_r, spd_r = parse_motor(val_r)
        
        return cls.build_frame(cls.CMD_ENG_CTRL, [dir_l, spd_l, dir_r, spd_r])