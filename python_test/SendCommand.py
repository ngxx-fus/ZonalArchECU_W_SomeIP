import os
import sys
import socket
import struct
import time
import random
from datetime import datetime

# Add the 'monitor' directory to the path so we can import configurations
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', 'monitor')))

try:
    from MonitorMain import CURRENT_CCU_IP, CURRENT_CCU_PORT
except ImportError:
    # Fallback if import fails
    CURRENT_CCU_IP = "10.0.0.100"
    CURRENT_CCU_PORT = 30490

# --- Frame Type Constants from AppFrame.h ---
eFrameType_EngineControl3 = 0xFF00CC0C

def log(message):
    now = datetime.now()
    timestamp = now.strftime("[%H:%M:%S.") + f"{now.microsecond // 1000:03d}]"
    print(f"{timestamp} {message}")

def calc_checksum16(data):
    """Calculates a simple 16-bit checksum."""
    csum = sum(data)
    while csum >> 16:
        csum = (csum & 0xFFFF) + (csum >> 16)
    return (~csum) & 0xFFFF

def calc_crc32(data):
    """Calculates CRC32 matching the ECU polynomial."""
    crc = 0xFFFFFFFF
    for b in data:
        crc ^= b
        for _ in range(8):
            if crc & 1:
                crc = (crc >> 1) ^ 0xEDB88320
            else:
                crc >>= 1
    return crc

def build_zecu_frame(frame_type, body_data):
    """Builds a generic ZECU frame adhering to the new AppFrame.h spec."""
    # Pad body strictly to 520 bytes
    body = body_data.ljust(520, b'\x00')
    
    # Pack header strictly aligned to 88 bytes
    header = struct.pack("<BBBB64s6sB4sHBBBI",
        0xAA, 0x04, 0xAA, 0xAA,
        b"CentralControlUnit",
        bytes.fromhex("e466e5984fd7"),
        0xAA, socket.inet_aton(CURRENT_CCU_IP), CURRENT_CCU_PORT,
        0xAA, 0xAA, 0xAA,
        frame_type
    )
    
    # Calculate checksums
    csum16 = calc_checksum16(header + body)
    crc32 = calc_crc32(header + body) ^ 0xFFFFFFFF
    
    # Trailer (8 bytes)
    magic0 = 0xAA
    magic1 = 0xAA
    trailer = struct.pack("<BHBI", magic0, csum16, magic1, crc32)
    
    return header + body + trailer

def start_random_test(target_ip="10.0.0.58", target_port=51786):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)

    sync_num = 0
    
    log(f"Starting Random Engine Control Test...")
    log(f"Broadcasting to {target_ip}:{target_port}")
    log(f"Emulating CCU IP: {CURRENT_CCU_IP}:{CURRENT_CCU_PORT}")

    try:
        while True:
            # Generate random motor speeds between -100% and +100%
            m0 = random.randint(-100, 100)
            m1 = random.randint(-100, 100)
            sync_num = (sync_num + 1) % 256
            
            # Build ZECUFrame_Body_EngineControl3_t
            # B(Magic0), B(Sync), B(Magic1), b(M0), B(Magic2), b(M1), H(CheckSum16)
            body_head = struct.pack("<BBBbBb", 0xAA, sync_num, 0xAA, m0, 0xAA, m1)
            body_csum = calc_checksum16(body_head)
            body_data = body_head + struct.pack("<H", body_csum)
            
            # Construct the complete UDP payload (616 bytes)
            frame = build_zecu_frame(eFrameType_EngineControl3, body_data)
            
            # Transmit
            sock.sendto(frame, (target_ip, target_port))
            log(f"Sent EngineControl3 | Sync: {sync_num:03d} | M0: {m0:4d}%, M1: {m1:4d}%")
            
            # Send at 10Hz (100ms interval)
            time.sleep(1)
            
    except KeyboardInterrupt:
        log("Test stopped by user (Ctrl+C).")
    finally:
        sock.close()

if __name__ == "__main__":
    start_random_test()
