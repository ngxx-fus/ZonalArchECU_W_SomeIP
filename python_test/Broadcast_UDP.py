import socket
import time
import struct
import argparse
import sys
import matplotlib.pyplot as plt
from datetime import datetime

# /* Global Statistics Containers */
stats = {
    "sent": 0,
    "received": 0,
    "timeouts": 0,
    "rtt_history": [],
    "timestamps": []
}

# /* Initialize Matplotlib for interactive plotting */
plt.ion()
fig, ax = plt.subplots(figsize=(10, 5))
line, = ax.plot([], [], 'b-o', markersize=4, label='RTT (ms)')
ax.set_xlabel('Packet Sequence')
ax.set_ylabel('Latency (ms)')
ax.set_title('W5500 Real-time UDP Echo Latency')
ax.grid(True)
ax.legend()

"""
/*
 * @brief Helper function to print logs with a precise timestamp [HH:MM:SS.SS]
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
 * @brief Renders a graphical chart using Matplotlib
 * @param max_points Number of recent points to display on the X-axis
 */
"""
def display_rtt_plt(max_points=50):
    # /* Guard clause: check if enough data is available */
    if not stats["rtt_history"]:
        return

    # /* Slice the latest data points */
    y_data = stats["rtt_history"][-max_points:]
    x_data = list(range(max(0, stats["sent"] - len(y_data)), stats["sent"]))

    # /* Update plot data without re-drawing the whole figure */
    line.set_data(x_data, y_data)
    
    # /* Adjust axes limits for dynamic visibility */
    ax.set_xlim(min(x_data), max(x_data) + 1)
    ax.set_ylim(0, max(y_data) * 1.2 if y_data else 10)
    
    # /* Force a draw and a short pause to handle GUI events */
    fig.canvas.draw()
    fig.canvas.flush_events()
    plt.pause(0.01)
    
    log(f"Plot Updated | Sent: {stats['sent']} | Recv: {stats['received']} | Loss: {stats['timeouts']}")
    
    # /* Return from function */
    return

"""
/*
 * @brief Log the payload details in a standard Hex + ASCII dump format
 */
"""
def log_frame(data, direction="TX"):
    size = len(data)
    log(f"log_frame(...) : {direction} | Size: {size} bytes")
    for i in range(0, size, 8):
        chunk = data[i:i+8]
        hex_part = " ".join([f"{b:02X}" for b in chunk])
        ascii_part = "".join([chr(b) if 0x20 <= b <= 0x7E else "." for b in chunk])
        log(f"{i:04X}: {hex_part:<23} | {ascii_part}")
    
    # /* Return from function */
    return

"""
/*
 * @brief Sends a UDP message with Magic Bytes and measures RTT
 */
"""
def send_and_receive_echo(target_ip, target_port, seq_num):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    sock.settimeout(0.5)
    
    magic_header = struct.pack(">BBH", 0xAA, 0xAA, seq_num)
    payload_text = f"ECHO_PLT_{seq_num:04d}".encode('utf-8')
    full_payload = magic_header + payload_text
    
    stats["sent"] += 1
    log_frame(full_payload, "TX")
    start_time = time.perf_counter()
    
    try:
        sock.sendto(full_payload, (target_ip, target_port))
        data, addr = sock.recvfrom(2048)
        end_time = time.perf_counter()
        
        rtt = (end_time - start_time) * 1000
        stats["received"] += 1
        stats["rtt_history"].append(rtt)
        
        log(f"RX <- {addr[0]} | Seq: {seq_num} | RTT: {rtt:.2f}ms")
        log_frame(data, "RX")
        
    except socket.timeout:
        stats["timeouts"] += 1
        # /* Log a sentinel value to keep history consistent if needed */
        log(f"RX Timeout | Seq: {seq_num}")
        
    except Exception as e:
        log(f"Socket Error: {e}")
        
    finally:
        sock.close()
        
    # /* Return from function */
    return

# /* Main execution entry point */
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="W5500 UDP Echo Monitor with Matplotlib")
    parser.add_argument("--packets-num", type=int, default=0, 
                        help="Update plot every N packets. Set 0 to disable.")
    args = parser.parse_args()

    TARGET_IP = "255.255.255.255"
    TARGET_PORT = 7979
    
    enable_stats = 1 <= args.packets_num <= 1000
    
    log(f"Monitor started. Target: {TARGET_IP}:{TARGET_PORT}")

    current_seq = 0
    try:
        # /* Infinite execution loop */
        while True:
            send_and_receive_echo(TARGET_IP, TARGET_PORT, current_seq)
            
            # /* Increment sequence and check for plot trigger */
            current_seq = (current_seq + 1) & 0xFFFF
            
            if enable_stats and (current_seq % args.packets_num == 0):
                display_rtt_plt()
                time.sleep(600)
                break
            
            # /* Delay 100ms between probes */
            time.sleep(0.1)
            
    except KeyboardInterrupt:
        log("Monitoring stopped by user. Closing plot...")
        plt.ioff()
        plt.show() # /* Keep chart open after exit */