import socket
import time
from datetime import datetime

# @brief Helper function to print logs with a precise timestamp [HH:MM:SS.SS]
# @param message The string content to print
def log(message):
    now = datetime.now()
    # strftime("%f") trả về microsecond (6 số), ta cắt lấy 2 số đầu để ra .SS
    timestamp = now.strftime("[%H:%M:%S.") + f"{now.microsecond // 10000:02d}]"
    print(f"{timestamp} {message}")

# @brief Sends a UDP string message to a target IP and Port
# @param target_ip The destination IP address (e.g., "10.0.0.19")
# @param target_port The destination UDP port (e.g., 5000)
# @param message The string payload to send
# @return True if sent successfully, False otherwise
def send_udp_packet(target_ip, target_port, message):
    """ Create a standard UDP socket (IPv4, Datagram) """
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    
    try:
        """ Encode the string to bytes and send it to the destination """
        sock.sendto(message.encode('utf-8'), (target_ip, target_port))
        log(f"Sent: '{message}' to {target_ip}:{target_port}")
        
        """ Optional: Wait for a response (timeout 2 seconds) """
        sock.settimeout(2.0)
        data, addr = sock.recvfrom(1024)
        log(f"Received reply from {addr[0]}:{addr[1]} -> {data.decode('utf-8')}")
        return True
        
    except socket.timeout:
        log(f"Timeout: No response from {target_ip}:{target_port}.")
        return False
    except Exception as e:
        log(f"Error during UDP communication: {e}")
        return False
    finally:
        """ Always close the socket to free OS resources """
        sock.close()

if __name__ == "__main__":
    TARGET_IP = "255.255.255.255"
    TARGET_PORT = 5000
    
    log(f"Starting UDP transmission to {TARGET_IP}:{TARGET_PORT}...")
    
    packet_count = 1
    
    """ Loop to send test packets periodically """
    while True:
        payload = f">>>>>>>>>>>>>>>>{packet_count}>>>>>>>>>>>>>>>>"
        send_udp_packet(TARGET_IP, TARGET_PORT, payload)
        
        packet_count += 1
        time.sleep(0.1)