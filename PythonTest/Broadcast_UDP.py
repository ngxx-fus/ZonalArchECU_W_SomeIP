import socket
import time
from datetime import datetime

"""
/*
 * @brief Helper function to print logs with a precise timestamp [HH:MM:SS.SS]
 * @param message The string content to print
 */
"""
def log(message):
    now = datetime.now()
    timestamp = now.strftime("[%H:%M:%S.") + f"{now.microsecond // 10000:02d}]"
    print(f"{timestamp} {message}")
    
    # /* Return from function */
    return

"""
/*
 * @brief Log the payload details in a standard Hex + ASCII dump format
 * @param data The payload buffer as a byte array
 */
"""
def log_frame(data):
    size = len(data)
    log(f"log_frame(...) : Payload Dump | Size: {size} bytes")

    # /* Iterate through the data buffer 8 bytes at a time */
    for i in range(0, size, 8):
        chunk = data[i:i+8]
        
        # /* Build the hex string (e.g., "3E 3E 3E...") */
        hex_part = " ".join([f"{b:02X}" for b in chunk])
        
        # /* Build the ASCII string, replacing non-printable chars with '.' */
        ascii_part = "".join([chr(b) if 0x20 <= b <= 0x7E else "." for b in chunk])
        
        # /* Align output: Offset | Hex Data | ASCII Data */
        # /* <23 aligns the hex part to occupy exactly 23 chars (8*2 + 7 spaces) */
        log(f"{i:04X}: {hex_part:<23} | {ascii_part}")
        
    # /* Return from function */
    return

"""
/*
 * @brief Sends a UDP string message to a target IP and Port (Fire-and-forget)
 * @param target_ip The destination IP address
 * @param target_port The destination UDP port
 * @param message The string payload to send
 * @return True if sent successfully, False otherwise
 */
"""
def send_udp_packet(target_ip, target_port, message):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    
    payload_bytes = message.encode('utf-8')
    
    # /* Try block to handle network transmission */
    try:
        sock.sendto(payload_bytes, (target_ip, target_port))
        log(f"Sent to {target_ip}:{target_port}")
        
        # /* Dump the transmitted payload to console */
        log_frame(payload_bytes)
        
        # /* Return success status immediately without blocking for receive */
        return True
        
    # /* Catch execution errors */
    except Exception as e:
        log(f"Error during UDP communication: {e}")
        
        # /* Return failure status */
        return False
        
    # /* Always close the socket to prevent resource leaks */
    finally:
        sock.close()

# /* Main execution entry point */
if __name__ == "__main__":
    TARGET_IP = "255.255.255.255"
    TARGET_PORT = 5000
    
    log(f"Starting UDP transmission to {TARGET_IP}:{TARGET_PORT}...")
    
    packet_count = 1
    
    # /* Infinite loop to send test packets periodically */
    while True:
        payload = f">>>>>>>>>>>>>>>>{packet_count}>>>>>>>>>>>>>>>>"
        send_udp_packet(TARGET_IP, TARGET_PORT, payload)
        
        packet_count += 1
        
        # /* Delay exactly 100ms before the next transmission */
        time.sleep(1)