import socket
import time

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
        print(f"Sent: '{message}' to {target_ip}:{target_port}")
        
        """ Optional: Wait for a response (timeout 2 seconds) """
        sock.settimeout(2.0)
        data, addr = sock.recvfrom(1024)
        print(f"Received reply from {addr[0]}:{addr[1]} -> {data.decode('utf-8')}")
        return True
        
    except socket.timeout:
        print(f"Timeout: No response from {target_ip}:{target_port}.")
        return False
    except Exception as e:
        print(f"Error during UDP communication: {e}")
        return False
    finally:
        """ Always close the socket to free OS resources """
        sock.close()

if __name__ == "__main__":
    TARGET_IP = "10.0.0.19"
    TARGET_PORT = 5000
    
    print(f"Starting UDP transmission to {TARGET_IP}:{TARGET_PORT}...")
    
    packet_count = 1
    
    """ Loop to send test packets periodically """
    while True:
        payload = f"Hello W5500, this is packet {packet_count}"
        send_udp_packet(TARGET_IP, TARGET_PORT, payload)
        
        packet_count += 1
        time.sleep(5)
