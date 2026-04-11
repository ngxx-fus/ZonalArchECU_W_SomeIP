from scapy.all import ARP, Ether, srp
import time

# @brief Discovers the MAC address of a target IP using ARP protocol
# @param target_ip The IP address to query (e.g., "10.0.0.1")
# @return The MAC address string if found, else None
def get_mac_address(target_ip):
    """ Create an Ethernet frame with broadcast destination """
    ether_layer = Ether(dst="ff:ff:ff:ff:ff:ff")
    
    """ Create the ARP request packet for the specific target """
    arp_layer = ARP(pdst=target_ip)

    """ Combine layers to form the final packet """
    packet = ether_layer / arp_layer

    try:
        """ Send packet and wait for a single response (timeout in seconds) """
        """ srp() is Send and Receive Packets at Layer 2 """
        result = srp(packet, timeout=2, verbose=False)[0]

        """ Extract the hardware address (MAC) from the response """
        if result:
            return result[0][1].hwsrc
        else:
            return None
            
    except Exception as e:
        print(f"Error during ARP request: {e}")
        return None

if __name__ == "__main__":
    TARGET_IP = "10.0.0.10"
    
    print(f"Starting ARP discovery for {TARGET_IP}...")
    
    """ Loop to query the MAC address periodically """
    while True:
        mac = get_mac_address(TARGET_IP)
        
        if mac:
            print(f"Success: The MAC address of {TARGET_IP} is {mac}")
        else:
            print(f"Failed: {TARGET_IP} did not respond to ARP.")
            
        time.sleep(1)