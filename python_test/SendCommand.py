import socket
import struct
import time
from datetime import datetime

# @brief Helper function to print logs with a precise timestamp [HH:MM:SS.SS]
# @param message The string content to print
def log(message):
    now = datetime.now()
    timestamp = now.strftime("[%H:%M:%S.") + f"{now.microsecond // 10000:02d}]"
    print(f"{timestamp} {message}")

# @brief Packs data into the SF_MotorCtl structure (504 bytes)
# @param m0 Motor 0 control bits
# @param m1 Motor 1 control bits
# @return Packed bytes payload matching C struct alignment
def pack_motor_ctl_payload(m0, m1):
    pattern0 = 0xFF00FFFF000055AA
    pattern1 = 0xBBCCDDDDEEEE2233
    pattern2 = 0x1144555567889999
    
    motor_bits = (m0 & 0x3FF) | ((m1 & 0x3FF) << 10)
    reserved_padding = b'\x00' * 476
    
    # Pack variables into memory buffer
    # Format: < (Little-endian), Q(8B), I(4B), Q(8B), Q(8B), 476s(476B) = 504 Bytes
    return struct.pack("<QIQQ476s", pattern0, motor_bits, pattern1, pattern2, reserved_padding)

# @brief Constructs and sends a single SyncFrame_t over UDP
# @param target_ip The destination IP address
# @param target_port The destination UDP port
# @param frame_id The ID of the synchronization frame
# @param m0 Motor 0 control bits
# @param m1 Motor 1 control bits
def send_sync_frame(target_ip, target_port, frame_id, m0, m1):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)

    try:
        header = struct.pack("<HHH", frame_id, 512, 0x0002)
        payload = pack_motor_ctl_payload(m0, m1)
        trailer = struct.pack("<H", 0xABCD)

        sync_frame = header + payload + trailer

        # Verify exact frame length before transmission
        if len(sync_frame) == 512:
            sock.sendto(sync_frame, (target_ip, target_port))
            log(f"Sent SyncFrame ID: {hex(frame_id)} | M0: {m0}, M1: {m1}")
        else:
            log(f"Frame alignment error: {len(sync_frame)} bytes generated.")

    except Exception as e:
        log(f"Transmission failed: {e}")
    finally:
        # Release system socket resources before exit
        sock.close()

if __name__ == "__main__":
    TARGET_IP = "10.0.0.59"
    TARGET_PORT = 5000
    
    log("Initializing Fail-safe Zonal Node emulator (Fixed & Cleaned)...")
    
    count = 0
    
    # Enter infinite loop for continuous frame transmission
    while True:
        send_sync_frame(TARGET_IP, TARGET_PORT, 0x123, count % 1024, 512)
        count += 1
        
        # Suspend thread execution for 2.5 seconds
        time.sleep(2.5)
