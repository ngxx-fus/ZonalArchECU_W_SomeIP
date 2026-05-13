from scapy.all import Ether, Raw, sendp, get_if_list
import time
from datetime import datetime

# /// @brief Helper function to print logs with a precise timestamp [HH:MM:SS.SS]
# /// @param message The string content to print
# /// @return None
def log(message):
    now = datetime.now()
    timestamp = now.strftime("[%H:%M:%S.") + f"{now.microsecond // 10000:02d}]"
    print(f"{timestamp} {message}")

# /// @brief Sends custom Ethernet frames with an incrementing payload byte
# /// @param dst_mac The destination hardware address
# /// @param iface The network interface to use
# /// @return None
def send_custom_ether_frames(dst_mac, iface):
    
    # /* Initialize counter and start the transmission loop */
    counter = 0
    
    while counter < 150:
        
        # /* Construct the 8-byte payload: 0x00 0xFF 0xAB 0xDE 0x00 A5 A5 <counter> */
        # payload_bytes = [0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, counter, counter, counter, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF]
        
        pattern = [
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            counter, counter, counter, counter, counter,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF
        ]

        payload_bytes = (pattern * (32 // len(pattern) + 1))[:1024]

        # Tạo Ethernet Frame (Layer 2). 
        # type=0x88B5 là EtherType dành cho mục đích thí nghiệm (Local Experimental)
        packet = Ether(dst=dst_mac, type=0x88B5) / Raw(load=bytes(payload_bytes))

        try:
            
            # /* Send the constructed packet at Layer 2 */
            sendp(packet, iface=iface, verbose=False)
            log(f"Sent frame to {dst_mac} | Payload: ...{counter:02X}")
            
            # Tăng counter và quay vòng từ 0x00 -> 0xFF
            counter = (counter + 1) % 256
            
        except Exception as e:
            log(f"Error sending packet: {e}")
            break
            
        time.sleep(0.1)

if __name__ == "__main__":
    # DEST_MAC = "03:02:01:dc:08:00"
    # DEST_MAC = "00:08:dc:01:02:03"
    DEST_MAC = "FF:FF:FF:FF:FF:FF"
    
    log(f"Available interfaces: {get_if_list()}")
    
    NETWORK_IFACE = "enxe466e5984fd7" 
    
    log(f"Starting custom frame transmission to {DEST_MAC} on {NETWORK_IFACE}...")
    send_custom_ether_frames(DEST_MAC, NETWORK_IFACE)
