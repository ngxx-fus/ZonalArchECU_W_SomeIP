import socket
import time
import sys
import os

# Route execution to include the monitor directory in the system path for cross-imports
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', 'monitor')))

from CoreNetwork import CCUCommandBuilder
from MonitorConfig import SysInfo, SysErr

# Network Configuration
CCU_IP = "10.0.0.102"
CCU_PORT = 30490
ZECU_IP = "10.0.0.58"
ZECU_PORT = 51786
CYCLE_TIME_SEC = 0.100

def start_spamming():
    """
    @brief Initiates a continuous transmission of UDP control frames to the ZECU.
    @return None.
    """
    SysInfo("SpamControl initialized. CCU: %s:%d -> ZECU: %s:%d", CCU_IP, CCU_PORT, ZECU_IP, ZECU_PORT)
    
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    
    # Route execution to bind the socket to the specific CCU interface
    try:
        sock.bind((CCU_IP, CCU_PORT))
    except OSError as e:
        SysErr("Failed to bind to %s:%d. Error: %s", CCU_IP, CCU_PORT, str(e))
        # Terminate execution on socket bind failure
        return
        
    packet_counter = 0
    
    # Steer execution into an infinite loop for periodic transmission
    while True:
        # Route execution to safely handle user interruptions and network errors
        try:
            # Generate a neutral engine control payload (0% left, 0% right)
            payload = CCUCommandBuilder.build_eng_ctrl(0, 0)
            
            sock.sendto(payload, (ZECU_IP, ZECU_PORT))
            packet_counter += 1
            
            # Route execution to log every 10 packets to avoid console flooding
            if packet_counter % 10 == 0:
                SysInfo("Sent %d packets to %s:%d", packet_counter, ZECU_IP, ZECU_PORT)
                
            time.sleep(CYCLE_TIME_SEC)
            
        except KeyboardInterrupt:
            SysInfo("SpamControl terminated by user.")
            # Terminate loop on keyboard interrupt
            break
        except Exception as e:
            SysErr("Transmission error: %s", str(e))
            # Skip to next iteration on transient error
            continue
            
    sock.close()
    
    # Exit function
    return

if __name__ == "__main__":
    """
    @brief Main execution entry point.
    """
    start_spamming()