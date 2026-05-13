import time
import argparse
from datetime import datetime
from scapy.all import ARP, Ether, srp

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
 * @param direction String indicating "Sent" or "Received" for context
 */
"""
def log_frame(data, direction="Packet"):
    size = len(data)
    log(f"log_frame(...) : {direction} Dump | Size: {size} bytes")

    # /* Iterate through the data buffer 8 bytes at a time */
    for i in range(0, size, 8):
        chunk = data[i:i+8]
        
        # /* Build the hex string (e.g., "FF FF FF...") */
        hex_part = " ".join([f"{b:02X}" for b in chunk])
        
        # /* Build the ASCII string, replacing non-printable chars with '.' and adding spaces */
        ascii_part = " ".join([chr(b) if 0x20 <= b <= 0x7E else "." for b in chunk])
        
        # /* Align output: Offset | Hex Data | ASCII Data */
        # /* <23 aligns the hex part to occupy exactly 23 chars (8*2 + 7 spaces) */
        log(f"{i:04X}: {hex_part:<23} | {ascii_part}")
        
    # /* Return from function */
    return

"""
/*
 * @brief Discovers the MAC address of a target IP using ARP protocol
 * @param target_ip The IP address to query
 * @return The MAC address string if found, else None
 */
"""
def get_mac_address(target_ip):
    ether_layer = Ether(dst="ff:ff:ff:ff:ff:ff")
    arp_layer = ARP(pdst=target_ip)
    packet = ether_layer / arp_layer

    # /* Log the outgoing ARP request frame */
    log_frame(bytes(packet), "Sent ARP Request")

    # /* Try block to handle network transmission and reception */
    try:
        result = srp(packet, timeout=2, verbose=False)[0]

        # /* Check if a valid response was received */
        if result:
            response_pkt = result[0][1]
            
            # /* Log the received ARP reply frame */
            log_frame(bytes(response_pkt), "Received ARP Reply")
            
            # /* Return the resolved hardware source MAC */
            return response_pkt.hwsrc
        # /* Handle the case where no response is received */
        else:
            # /* Return NULL equivalent */
            return None
            
    # /* Catch execution errors */
    except Exception as e:
        log(f"Error during ARP request: {e}")
        
        # /* Return NULL equivalent upon exception */
        return None

# /* Main execution entry point */
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="ARP MAC Discovery Tool")
    parser.add_argument("--ip", type=str, default="10.0.0.59", help="Target IP address to resolve")
    args = parser.parse_args()
    
    target_ip = args.ip
    
    log(f"Starting ARP discovery for {target_ip}...")
    
    # /* Infinite loop to query the MAC address periodically */
    while True:
        mac = get_mac_address(target_ip)
        
        # /* Evaluate if MAC resolution succeeded */
        if mac:
            log(f"Success: The MAC address of {target_ip} is {mac}\n")
        # /* Handle failed MAC resolution scenario */
        else:
            log(f"Failed: {target_ip} did not respond to ARP.\n")
            
        # /* Delay before the next transmission */
        time.sleep(2)