import socket
import time
import struct
import argparse
import sys
import matplotlib.pyplot as plt
from datetime import datetime

# /* Global Statistics Containers */
stats = {
    "received": 0,
    "sync_errors": 0,
    "dist_sync_history": [],
    "motor_sync_history": [],
    "d0_history": [],
    "d1_history": [],
    "d2_history": [],
    "m0_history": [],
    "m1_history": [],
    "last_seq": -1,
    "last_time": 0.0,
    "cycle_buffer": [],
    "avg_cycle_history": [],
    "avg_error_history": []
}

# /* Initialize Matplotlib for interactive plotting with 3 subplots */
plt.ion()
fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(10, 10), sharex=True)

# /* Subplot 1: Synchronization Counters */
line_dist_seq, = ax1.plot([], [], 'g-o', markersize=4, label='Dist SyncNum')
line_motor_seq, = ax1.plot([], [], 'b-x', markersize=4, label='Motor SyncNum')
ax1.set_ylabel('Sync Counter (0-3)')
ax1.set_title('W5500 UDP SF_ECUState_t Monitor')
ax1.grid(True)
ax1.legend(loc='upper left')

# /* Twin axis for Subplot 1 to display Cycle Time & Error */
ax1_twin = ax1.twinx()
line_avg_cycle, = ax1_twin.plot([], [], 'r--', markersize=4, label='Avg Cycle (ms)')
line_avg_error, = ax1_twin.plot([], [], 'm--', markersize=4, label='Avg Error (ms)')
ax1_twin.set_ylabel('Cycle Time (ms)')
ax1_twin.legend(loc='upper right')

# /* Subplot 2: Distance Sensors */
line_d0, = ax2.plot([], [], 'r-o', markersize=4, label='D0 (cm)')
line_d1, = ax2.plot([], [], 'm-x', markersize=4, label='D1 (cm)')
line_d2, = ax2.plot([], [], 'c-^', markersize=4, label='D2 (cm)')
ax2.set_ylabel('Distance')
ax2.grid(True)
ax2.legend(loc='upper left')

# /* Subplot 3: Motor Sensors */
line_m0, = ax3.plot([], [], 'y-o', markersize=4, label='M0 (RPM)')
line_m1, = ax3.plot([], [], 'k-x', markersize=4, label='M1 (RPM)')
ax3.set_xlabel('Packet Index')
ax3.set_ylabel('Motor Speed')
ax3.grid(True)
ax3.legend(loc='upper left')

fig.tight_layout()

# /**
#  * @brief Helper function to print logs with a precise timestamp [HH:MM:SS.SS]
#  * @param message The string content to print
#  */
def log(message):
    now = datetime.now()
    timestamp = now.strftime("[%H:%M:%S.") + f"{now.microsecond // 10000:02d}]"
    print(f"{timestamp} {message}")
    
    # /* Return from execution */
    return

# /**
#  * @brief Renders a graphical chart using Matplotlib to track all ECU state parameters
#  * @param max_points Number of maximum points to display on the X-axis
#  */
def display_plt(max_points=50):
    # /* Guard clause to prevent plotting on empty data */
    if not stats["dist_sync_history"]:
        return

    # /* Slice the latest data points for visualization */
    y_dist_seq = stats["dist_sync_history"][-max_points:]
    y_motor_seq = stats["motor_sync_history"][-max_points:]
    y_d0 = stats["d0_history"][-max_points:]
    y_d1 = stats["d1_history"][-max_points:]
    y_d2 = stats["d2_history"][-max_points:]
    y_m0 = stats["m0_history"][-max_points:]
    y_m1 = stats["m1_history"][-max_points:]

    x_data = list(range(max(0, stats["received"] - len(y_dist_seq)), stats["received"]))

    # /* Update graphical lines with new data */
    line_dist_seq.set_data(x_data, y_dist_seq)
    line_motor_seq.set_data(x_data, y_motor_seq)
    line_d0.set_data(x_data, y_d0)
    line_d1.set_data(x_data, y_d1)
    line_d2.set_data(x_data, y_d2)
    line_m0.set_data(x_data, y_m0)
    line_m1.set_data(x_data, y_m1)

    # /* Process data for twin axis (Avg Cycle & Error) */
    min_x = min(x_data) if x_data else 0
    filtered_avg_cycle = [(x, y) for x, y in stats["avg_cycle_history"] if x >= min_x]
    filtered_avg_error = [(x, y) for x, y in stats["avg_error_history"] if x >= min_x]

    if filtered_avg_cycle:
        line_avg_cycle.set_data([x for x, y in filtered_avg_cycle], [y for x, y in filtered_avg_cycle])
        line_avg_error.set_data([x for x, y in filtered_avg_error], [y for x, y in filtered_avg_error])
        
        y_cycles = [y for x, y in filtered_avg_cycle]
        y_errors = [y for x, y in filtered_avg_error]
        max_y = max(max(y_cycles), max(y_errors))
        ax1_twin.set_ylim(0, max_y * 1.2 if max_y > 0 else 10)
    else:
        line_avg_cycle.set_data([], [])
        line_avg_error.set_data([], [])

    # /* Adjust axis limits dynamically for Subplot 1 (Sync) */
    ax1.set_xlim(min(x_data), max(x_data) + 1)
    ax1.set_ylim(-0.5, 3.5) # /* 2-bit counter limits (0-3) */
    
    # /* Adjust axis limits dynamically for Subplot 2 (Distance) */
    ax2.set_xlim(min(x_data), max(x_data) + 1)
    dist_vals = y_d0 + y_d1 + y_d2
    max_dist = max(dist_vals) if dist_vals else 1023
    ax2.set_ylim(-10, (max_dist * 1.2) if max_dist > 0 else 1023)

    # /* Adjust axis limits dynamically for Subplot 3 (Motor) */
    ax3.set_xlim(min(x_data), max(x_data) + 1)
    motor_vals = y_m0 + y_m1
    max_motor = max(motor_vals) if motor_vals else 1023
    ax3.set_ylim(-10, (max_motor * 1.2) if max_motor > 0 else 1023)
    
    # /* Flush UI events and pause to render without blocking */
    fig.canvas.draw()
    fig.canvas.flush_events()
    plt.pause(0.01)
    
    # /* Return from execution */
    return

# /**
#  * @brief Log the payload details in a standard Hex + ASCII dump format
#  * @param data The raw byte array
#  * @param direction String denoting RX or TX
#  */
def log_frame(data, direction="RX"):
    size = len(data)
    log(f"log_frame(...) : {direction} | Size: {size} bytes")
    
    # /* Iterate through the data buffer 8 bytes at a time */
    for i in range(0, size, 8):
        chunk = data[i:i+8]
        hex_part = " ".join([f"{b:02X}" for b in chunk])
        ascii_part = "".join([chr(b) if 0x20 <= b <= 0x7E else "." for b in chunk])
        log(f"{i:04X}: {hex_part:<23} | {ascii_part}")
    
    # /* Return from execution */
    return

# /**
#  * @brief Parses the application-specific parameters from the SF_ECUState_t payload
#  * @param packet The raw bytes of the incoming packet
#  */
def parse_application_data(packet):
    # /* Guard clause: Ensure payload is long enough to contain SF_ECUState_t (8 bytes) */
    if len(packet) < 8:
        return
        
    # /* Unpack 8 bytes into two 32-bit Little-Endian unsigned integers */
    dist_raw, motor_raw = struct.unpack("<II", packet[:8])
    
    # /* Extract SensorDistance_t bitfields (10 bits, 10 bits, 10 bits, 2 bits) */
    d0 = dist_raw & 0x3FF
    d1 = (dist_raw >> 10) & 0x3FF
    d2 = (dist_raw >> 20) & 0x3FF
    dist_sync = (dist_raw >> 30) & 0x03
    
    # /* Extract ActualMotor_t bitfields (10 bits, 10 bits, 2 bits, 10 bits reserved) */
    m0 = motor_raw & 0x3FF
    m1 = (motor_raw >> 10) & 0x3FF
    motor_sync = (motor_raw >> 20) & 0x03
    reserved = (motor_raw >> 22) & 0x3FF
    
    # /* Record historical data for Matplotlib rendering */
    stats["dist_sync_history"].append(dist_sync)
    stats["motor_sync_history"].append(motor_sync)
    stats["d0_history"].append(d0)
    stats["d1_history"].append(d1)
    stats["d2_history"].append(d2)
    stats["m0_history"].append(m0)
    stats["m1_history"].append(m1)
    
    log(f"--- DECODED ECU STATE ---")
    log(f" > Distance [D0: {d0:4d}, D1: {d1:4d}, D2: {d2:4d}] | Sync: {dist_sync}")
    log(f" > Motor    [M0: {m0:4d}, M1: {m1:4d}]             | Sync: {motor_sync} | Res: {reserved}")
    log(f"-------------------------\n")
    
    # /* Return from execution */
    return

# /**
#  * @brief Verifies the Sync Sequence Counter of a received packet
#  * @param packet The raw bytes of the incoming packet
#  * @return True if sync is valid, False otherwise
#  */
def verify_sync(packet):
    # /* Guard clause for packet minimum length */
    if len(packet) < 8:
        return False

    # /* Extract SyncNum from the Distance struct (bits 30-31 of first 4 bytes) */
    dist_raw = struct.unpack("<I", packet[:4])[0]
    received_seq = (dist_raw >> 30) & 0x03
        
    # /* Conditional steering to validate Sequence Counter logic */
    if stats["last_seq"] != -1:
        expected_seq = (stats["last_seq"] + 1) % 4
        
        # /* Branch if sequence mismatch is detected */
        if received_seq != expected_seq:
            log(f"SYNC SEQ ERROR: Expected {expected_seq}, got {received_seq}")
            stats["last_seq"] = received_seq
            return False
            
    stats["last_seq"] = received_seq
    log(f"SYNC OK | Seq: {received_seq}")
    
    # /* Return success validation */
    return True

# /* Main execution entry point */
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="W5500 UDP Heartbeat Listener & Parser")
    parser.add_argument("--packets-num", type=int, default=10, 
                        help="Update plot every N packets. Set 0 to disable.")
    args = parser.parse_args()

    BIND_IP = "0.0.0.0" 
    BIND_PORT = 30490
    
    # /* Evaluate if statistics plotting is enabled */
    enable_stats = 1 <= args.packets_num <= 1000
    
    log(f"Starting UDP Listener on {BIND_IP}:{BIND_PORT}...")

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((BIND_IP, BIND_PORT))
    
    # /* Try-catch block for network operations and graceful exit */
    try:
        # /* Infinite loop to continuously monitor incoming traffic */
        while True:
            # /* Await incoming network data */
            data, addr = sock.recvfrom(2048)
            current_time = time.perf_counter()
            
            # /* Calculate Cycle Time & Tumbling Window Stats */
            if stats["last_time"] != 0.0:
                cycle_time = (current_time - stats["last_time"]) * 1000.0 # /* Convert to ms */
                stats["cycle_buffer"].append((stats["received"], cycle_time))
                
                # /* Check if buffer reached 30 packets for tumbling window */
                if len(stats["cycle_buffer"]) == 30:
                    avg_cycle = sum(c[1] for c in stats["cycle_buffer"]) / 30.0
                    avg_error = sum(abs(c[1] - avg_cycle) for c in stats["cycle_buffer"]) / 30.0
                    
                    for pkt_idx, _ in stats["cycle_buffer"]:
                        stats["avg_cycle_history"].append((pkt_idx, avg_cycle))
                        stats["avg_error_history"].append((pkt_idx, avg_error))
                        
                    stats["cycle_buffer"] = []
                    
            stats["last_time"] = current_time
            stats["received"] += 1
            
            log(f"RX <- {addr[0]}:{addr[1]}")
            log_frame(data, "RX")
            
            # /* Conditional branching to verify Sync Sequence */
            if not verify_sync(data):
                stats["sync_errors"] += 1
                
            # /* Parse application data into global dictionaries */
            parse_application_data(data)
            
            # /* Steering logic to trigger plot updates based on user threshold */
            if enable_stats and (stats["received"] % args.packets_num == 0):
                display_plt()
            
    # /* Handle graceful shutdown upon user interrupt */
    except KeyboardInterrupt:
        log("Monitoring stopped by user. Closing plot...")
        plt.ioff()
        plt.show() 
        
    # /* Cleanup block to ensure OS resources are freed */
    finally:
        log("Closing socket gracefully.")
        sock.close()