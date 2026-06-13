## @file UdpListener.py
## @brief Background threaded UDP listener for real-time telemetry extraction.

import socket
import struct
import time
from PySide6.QtCore import QThread, Signal

# /* --- CONFIGURATION --- */
# Generic Frame sizes based on ZECUFrame_Generic_t
ECU_HEADER_SIZE = 88       # 4 (Magic/Ver) + 80 (Info) + 4 (FrameType)
ECU_BODY_UNION_SIZE = 520  # Largest body payload (Broadcast)
ECU_TRAILER_SIZE = 8       # 1 (Magic) + 2 (CheckSum) + 1 (Magic) + 4 (CRC)
EXPECTED_PACKET_SIZE = ECU_HEADER_SIZE + ECU_BODY_UNION_SIZE + ECU_TRAILER_SIZE # 616 bytes

# Frame Type Magic Numbers
HEARTBEAT_FRAME_TYPE = 0xFF00FF0A
AUTHTX_FRAME_TYPE    = 0xAA00000A
BROADCAST_FRAME_TYPE = 0xFFFFFFFF

# /*
#  * @brief Background thread for continuous UDP listening.
#  */
class UdpListenerThread(QThread):
    # /* Define a signal to send payload dictionary to the Main GUI Thread */
    data_received = Signal(dict)

    def __init__(self):
        super().__init__()
        self.is_running = True

        self.BIND_IP = "0.0.0.0" 
        self.BIND_PORT = 30490
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        
        # /* Allow sharing this Port with the Discovery Dialog */
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        if hasattr(socket, 'SO_REUSEPORT'):
            self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        
        self.sock.settimeout(1.0)
        self.sock.bind((self.BIND_IP, self.BIND_PORT))

    def run(self):
        print(f"[*] Thread: UDP Listener Started on {self.BIND_IP}:{self.BIND_PORT}")

        while self.is_running:
            try:
                data, addr = self.sock.recvfrom(2048)
                current_time = time.perf_counter()
                
                # /* Validate absolute minimum packet size to prevent out-of-bounds unpacking */
                if len(data) < ECU_HEADER_SIZE + 24:
                    continue
                    
                # /* 1. Parse the Generic Header */
                header_bytes = data[:ECU_HEADER_SIZE]
                
                # Format: 4x B (Magic/Ver), 64s (Label), 6s (MAC), B (Magic), 4s (IP), H (Port), 3x B (Magic), I (FrameType)
                magic0, ver, magic1, magic2, label_bytes, mac_bytes, m3, ip_bytes, port, m4, m5, m6, frame_type = struct.unpack("<BBBB64s6sB4sHBBBI", header_bytes)
                
                # /* Security check: Verify ZECU Frame integrity */
                if magic0 != 0xAA or ver != 0x04:
                    continue
                    
                # /* Extract textual identification and format MAC */
                ecu_name = label_bytes.decode('utf-8', errors='ignore').split('\x00')[0]
                ecu_mac = ':'.join(f'{b:02X}' for b in mac_bytes)
                ecu_ip = addr[0]  
                ecu_port = port
                
                # /* 2. Intercept and parse only HeartBeat telemetry frames for the monitor */
                if frame_type == HEARTBEAT_FRAME_TYPE:
                    body_bytes = data[ECU_HEADER_SIZE : ECU_HEADER_SIZE + 24]
                    
                    # ZECUFrame_Body_HeartBeat_t Format (24 bytes):
                    # B(Magic0), B(Sync), B(Magic1), H(D0), H(D1), H(D2), B(Magic2), 
                    # b(M0), b(M1), B(Magic3), I(Long), I(Lat), B(M4), B(M5), B(M6)
                    bm0, sync_num, bm1, d0, d1, d2, bm2, m0, m1, bm3, loc_long, loc_lat, bm4, bm5, bm6 = struct.unpack("<BBBHHHBbbBIIBBB", body_bytes)
                    
                    # /* Package the parsed data. Note: Reusing sync_num for both dist and motor to preserve UI logic */
                    payload = {
                        "type": "HeartBeat",
                        "name": ecu_name,
                        "mac": ecu_mac,
                        "ip": ecu_ip,
                        "port": ecu_port,
                        "d0": d0, "d1": d1, "d2": d2, 
                        "dist_sync": sync_num,
                        "m0": m0, "m1": m1, 
                        "motor_sync": sync_num,
                        "loc_long": loc_long,
                        "loc_lat": loc_lat,
                        "timestamp": current_time
                    }
                    
                    # /* Dispatch the assembled payload dictionary back to the UI Thread */
                    self.data_received.emit(payload)
                    
                # /* 3. Intercept Authentication Challenge (AuthTX) frames */
                elif frame_type == AUTHTX_FRAME_TYPE:
                    body_bytes = data[ECU_HEADER_SIZE : ECU_HEADER_SIZE + 12]
                    # Format: B(Magic0), Q(Challenge), B(Magic1), B(Magic2), B(Magic3)
                    m0, challenge, m1, m2, m3 = struct.unpack("<BQBBB", body_bytes)
                    
                    print(f"[*] UdpListener: Intercepted AuthTX Challenge: 0x{challenge:016X}")
                    
                    payload = {
                        "type": "AuthTX",
                        "name": ecu_name,
                        "mac": ecu_mac,
                        "ip": ecu_ip,
                        "port": ecu_port,
                        "challenge": challenge
                    }
                    self.data_received.emit(payload)
                    
                # /* 4. Silently ignore Broadcast frames (DiscoveryDialog handles these) */
                elif frame_type == BROADCAST_FRAME_TYPE:
                    pass

                else:
                    print(f"[*] UdpListener: Unhandled FrameType 0x{frame_type:08X} from {ecu_ip}")
                
            except socket.timeout:
                # /* Normal occurrence; keep thread spinning */
                continue
            except Exception as e:
                print(f"[!] Thread Error: {e}")

        # /* Shutdown protocol */
        self.sock.close()
        print("[*] Thread: UDP Listener Stopped.")
        return

    def stop(self):
        # /* Signal loop termination */
        self.is_running = False
        return