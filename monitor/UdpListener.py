## @file UdpListener.py
## @brief Background threaded UDP listener for real-time telemetry extraction.

import socket
import struct
import time
from PySide6.QtCore import QThread, Signal

# /* --- CONFIGURATION --- */
ECU_HEADER_SIZE = 44  
DATA_PAYLOAD_SIZE = 8
EXPECTED_PACKET_SIZE = ECU_HEADER_SIZE + DATA_PAYLOAD_SIZE

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
        self.BIND_IP = "0.0.0.0" 
        self.BIND_PORT = 30490
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.settimeout(1.0)
        self.sock.bind((self.BIND_IP, self.BIND_PORT))

    def run(self):
        # self.BIND_IP = "0.0.0.0" 
        # self.BIND_PORT = 30490
        
        # sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        # sock.settimeout(1.0)
        # sock.bind((self.BIND_IP, self.BIND_PORT))
        
        print(f"[*] Thread: UDP Listener Started on {self.BIND_IP}:{self.BIND_PORT}")

        while self.is_running:
            try:
                data, addr = self.sock.recvfrom(2048)
                # data, addr = sock.recvfrom(2048)
                current_time = time.perf_counter()
                
                if len(data) != EXPECTED_PACKET_SIZE:
                    continue
                    
                header_bytes = data[:ECU_HEADER_SIZE]
                ecu_name_raw = struct.unpack("<32s", header_bytes[:32])[0]
                ecu_name = ecu_name_raw.decode('utf-8', errors='ignore').split('\x00')[0]
                
                ecu_mac = ':'.join(f'{b:02X}' for b in header_bytes[32:38])
                
                ecu_ip = addr[0]  
                ecu_port = struct.unpack("<H", header_bytes[38:40])[0]
                
                data_bytes = data[ECU_HEADER_SIZE : ECU_HEADER_SIZE + DATA_PAYLOAD_SIZE]
                dist_raw, motor_raw = struct.unpack("<II", data_bytes)
                
                d0 = dist_raw & 0x3FF
                d1 = (dist_raw >> 10) & 0x3FF
                d2 = (dist_raw >> 20) & 0x3FF
                dist_sync = (dist_raw >> 30) & 0x03
                
                m0 = (motor_raw & 0x3FF) - 100
                m1 = ((motor_raw >> 10) & 0x3FF) - 100
                motor_sync = (motor_raw >> 20) & 0x03
                
                payload = {
                    "name": ecu_name,
                    "mac": ecu_mac,
                    "ip": ecu_ip,
                    "port": ecu_port,
                    "d0": d0, "d1": d1, "d2": d2, "dist_sync": dist_sync,
                    "m0": m0, "m1": m1, "motor_sync": motor_sync,
                    "timestamp": current_time
                }
                
                self.data_received.emit(payload)
                
            except socket.timeout:
                continue
            except Exception as e:
                print(f"[!] Thread Error: {e}")

        self.sock.close()
        print("[*] Thread: UDP Listener Stopped.")
        return

    def stop(self):
        self.is_running = False
        return