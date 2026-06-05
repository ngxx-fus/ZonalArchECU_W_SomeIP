import socket
import time
import struct
import argparse
import sys
import matplotlib.pyplot as plt
from datetime import datetime

# /* --- CONFIGURATION --- */
# /* Modify this constant if the ECUInfo_t size changes (e.g., padded to 48) */
ECU_HEADER_SIZE = 44  
DATA_PAYLOAD_SIZE = 8
EXPECTED_PACKET_SIZE = ECU_HEADER_SIZE + DATA_PAYLOAD_SIZE

# /* --- GLOBAL REGISTRIES --- */
# /* ecu_registry format: { ecu_id: { "name": str, "stats": dict, "lines": dict } } */
ecu_registry = {}

# /* Initialize Matplotlib for interactive plotting with 3 subplots */
plt.ion()
fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(10, 10), sharex=True)

ax1.set_ylabel('Sync Counter (0-3)')
ax1.set_title('W5500 UDP Zonal ECU Monitor')
ax1.grid(True)

ax1_twin = ax1.twinx()
ax1_twin.set_ylabel('Cycle Time (ms)')

ax2.set_ylabel('Distance (cm)')
ax2.grid(True)

ax3.set_xlabel('Packet Index')
ax3.set_ylabel('Motor Speed (RPM)')
ax3.grid(True)
fig.tight_layout()

# /**
#  * @brief Helper function to print logs with a precise timestamp [HH:MM:SS.SS]
#  * @param message The string content to print
#  */
def log(message):
    now = datetime.now()
    timestamp = now.strftime("[%H:%M:%S.") + f"{now.microsecond // 10000:02d}]"
    print(f"{timestamp} {message}")
    
    # /* Exit logging function */
    return

# /**
#  * @brief Initializes data containers and Matplotlib lines for a newly discovered ECU
#  * @param ecu_id Unique 8-bit identifier of the ECU
#  * @param ecu_name String name of the ECU
#  */
def register_new_ecu(ecu_id, ecu_name):
    log(f"[*] NEW ECU DETECTED: {ecu_name} (ID: {ecu_id:#04X})")
    
    # /* Initialize statistics container for this specific ECU */
    stats = {
        "received": 0, "sync_errors": 0, "last_seq": -1, "last_time": 0.0,
        "dist_sync_history": [], "motor_sync_history": [],
        "d0_history": [], "d1_history": [], "d2_history": [],
        "m0_history": [], "m1_history": [],
        "cycle_buffer": [], "avg_cycle_history": [], "avg_error_history": []
    }
    
    # /* Initialize unique plot lines for this ECU */
    lines = {
        "dist_seq": ax1.plot([], [], marker='o', markersize=4, label=f'[{ecu_name}] Dist Sync')[0],
        "motor_seq": ax1.plot([], [], marker='x', markersize=4, label=f'[{ecu_name}] Motor Sync')[0],
        "avg_cycle": ax1_twin.plot([], [], linestyle='--', markersize=4, label=f'[{ecu_name}] Cycle (ms)')[0],
        "d0": ax2.plot([], [], marker='o', markersize=4, label=f'[{ecu_name}] D0')[0],
        "d1": ax2.plot([], [], marker='x', markersize=4, label=f'[{ecu_name}] D1')[0],
        "d2": ax2.plot([], [], marker='^', markersize=4, label=f'[{ecu_name}] D2')[0],
        "m0": ax3.plot([], [], marker='o', markersize=4, label=f'[{ecu_name}] M0')[0],
        "m1": ax3.plot([], [], marker='x', markersize=4, label=f'[{ecu_name}] M1')[0],
    }
    
    # /* Refresh legends to include the new lines */
    ax1.legend(loc='upper left', fontsize='small')
    ax1_twin.legend(loc='upper right', fontsize='small')
    ax2.legend(loc='upper left', fontsize='small')
    ax3.legend(loc='upper left', fontsize='small')
    
    # /* Save to global registry */
    ecu_registry[ecu_id] = {
        "name": ecu_name,
        "stats": stats,
        "lines": lines
    }
    
    # /* Conclude initialization and return to caller */
    return

# /**
#  * @brief Renders the graphical chart using Matplotlib for ALL registered ECUs
#  * @param max_points Number of maximum points to display on the X-axis
#  */
def display_plt_legacy(max_points=50):
    global_max_x = 0
    global_min_x = 0
    
    # /* Iterate through every registered ECU to update graphical representation */
    for ecu_id, ecu_data in ecu_registry.items():
        stats = ecu_data["stats"]
        lines = ecu_data["lines"]
        
        # /* Check if data history exists to avoid plotting empty arrays */
        if not stats["dist_sync_history"]:
            # /* Skip this ECU if no data is available */
            continue

        # /* Slice the latest data points */
        y_dist_seq = stats["dist_sync_history"][-max_points:]
        y_motor_seq = stats["motor_sync_history"][-max_points:]
        y_d0 = stats["d0_history"][-max_points:]
        y_d1 = stats["d1_history"][-max_points:]
        y_d2 = stats["d2_history"][-max_points:]
        y_m0 = stats["m0_history"][-max_points:]
        y_m1 = stats["m1_history"][-max_points:]

        # /* Calculate X-axis indices for this ECU */
        x_data = list(range(max(0, stats["received"] - len(y_dist_seq)), stats["received"]))
        
        # /* Evaluate if X data exists to dynamically scale the plot boundaries */
        if x_data:
            global_max_x = max(global_max_x, max(x_data))
            global_min_x = max(0, global_max_x - max_points)

        # /* Update line data */
        lines["dist_seq"].set_data(x_data, y_dist_seq)
        lines["motor_seq"].set_data(x_data, y_motor_seq)
        lines["d0"].set_data(x_data, y_d0)
        lines["d1"].set_data(x_data, y_d1)
        lines["d2"].set_data(x_data, y_d2)
        lines["m0"].set_data(x_data, y_m0)
        lines["m1"].set_data(x_data, y_m1)

        # /* Filter cycle data within the current visible window */
        filtered_avg_cycle = [(x, y) for x, y in stats["avg_cycle_history"] if x >= global_min_x]
        
        # /* Evaluate if cycle data is available to update the twin axis */
        if filtered_avg_cycle:
            lines["avg_cycle"].set_data([x for x, y in filtered_avg_cycle], [y for x, y in filtered_avg_cycle])

    # /* Adjust axis limits dynamically based on the most active ECU */
    ax1.set_xlim(global_min_x, global_max_x + 1)
    ax1.set_ylim(-0.5, 3.5)
    
    ax2.set_xlim(global_min_x, global_max_x + 1)
    ax3.set_xlim(global_min_x, global_max_x + 1)
    
    # /* Flush UI events and pause to render without blocking the main loop */
    fig.canvas.draw()
    fig.canvas.flush_events()
    plt.pause(0.01)
    
    # /* Conclude plot rendering and return */
    return

# /**
#  * @brief Renders the graphical chart using Matplotlib for ALL registered ECUs
#  * @param max_points Number of maximum points to display on the X-axis
#  */
def display_plt(max_points=50):
    global_max_x = 0
    global_min_x = 0
    global_max_dist = 0   # /* Thêm biến theo dõi Max Distance */
    global_max_motor = 0  # /* Thêm biến theo dõi Max Motor */
    
    # /* Iterate through every registered ECU to update graphical representation */
    for ecu_id, ecu_data in ecu_registry.items():
        stats = ecu_data["stats"]
        lines = ecu_data["lines"]
        
        # /* Check if data history exists to avoid plotting empty arrays */
        if not stats["dist_sync_history"]:
            continue

        # /* Slice the latest data points */
        y_dist_seq = stats["dist_sync_history"][-max_points:]
        y_motor_seq = stats["motor_sync_history"][-max_points:]
        y_d0 = stats["d0_history"][-max_points:]
        y_d1 = stats["d1_history"][-max_points:]
        y_d2 = stats["d2_history"][-max_points:]
        y_m0 = stats["m0_history"][-max_points:]
        y_m1 = stats["m1_history"][-max_points:]

        # /* Calculate X-axis indices for this ECU */
        x_data = list(range(max(0, stats["received"] - len(y_dist_seq)), stats["received"]))
        
        # /* Evaluate if X data exists to dynamically scale the plot boundaries */
        if x_data:
            global_max_x = max(global_max_x, max(x_data))
            global_min_x = max(0, global_max_x - max_points)
            
            # /* Tìm giá trị cao nhất trong khung hình hiện tại để co giãn trục Y */
            global_max_dist = max(global_max_dist, max(y_d0), max(y_d1), max(y_d2))
            global_max_motor = max(global_max_motor, max(y_m0), max(y_m1))

        # /* Update line data */
        lines["dist_seq"].set_data(x_data, y_dist_seq)
        lines["motor_seq"].set_data(x_data, y_motor_seq)
        lines["d0"].set_data(x_data, y_d0)
        lines["d1"].set_data(x_data, y_d1)
        lines["d2"].set_data(x_data, y_d2)
        lines["m0"].set_data(x_data, y_m0)
        lines["m1"].set_data(x_data, y_m1)

        # /* Filter cycle data within the current visible window */
        filtered_avg_cycle = [(x, y) for x, y in stats["avg_cycle_history"] if x >= global_min_x]
        
        if filtered_avg_cycle:
            lines["avg_cycle"].set_data([x for x, y in filtered_avg_cycle], [y for x, y in filtered_avg_cycle])

    # /* Adjust axis limits dynamically based on the most active ECU */
    ax1.set_xlim(global_min_x, global_max_x + 1)
    ax1.set_ylim(-0.5, 3.5)
    
    # /* Cập nhật giới hạn trục Y cho Distance (Subplot 2) */
    ax2.set_xlim(global_min_x, global_max_x + 1)
    ax2.set_ylim(-10, (global_max_dist * 1.2) if global_max_dist > 0 else 1023)
    
    # /* Cập nhật giới hạn trục Y cho Motor (Subplot 3) */
    ax3.set_xlim(global_min_x, global_max_x + 1)
    ax3.set_ylim(-10, (global_max_motor * 1.2) if global_max_motor > 0 else 1023)
    
    # /* Flush UI events and pause to render without blocking the main loop */
    fig.canvas.draw()
    fig.canvas.flush_events()
    plt.pause(0.01)
    
    return

# /**
#  * @brief Verifies Sync Sequence and calculates Cycle Timing per ECU
#  * @param stats The statistics dictionary specific to the ECU
#  * @param dist_sync The extracted Sync Number
#  */
def process_ecu_timing(stats, dist_sync):
    # /* Verify if a valid sequence history exists to perform comparison */
    if stats["last_seq"] != -1:
        expected_seq = (stats["last_seq"] + 1) % 4
        
        # /* Branch if sequence mismatch is detected (packet loss indicator) */
        if dist_sync != expected_seq:
            stats["sync_errors"] += 1
            log(f"   [!] SYNC ERROR: Expected {expected_seq}, got {dist_sync}")
            
    stats["last_seq"] = dist_sync

    current_time = time.perf_counter()
    
    # /* Verify if an initial timestamp exists to calculate the delta cycle time */
    if stats["last_time"] != 0.0:
        cycle_time = (current_time - stats["last_time"]) * 1000.0
        stats["cycle_buffer"].append((stats["received"], cycle_time))
        
        # /* Evaluate if the cycle buffer has reached the tumbling window size limit */
        if len(stats["cycle_buffer"]) == 30:
            avg_cycle = sum(c[1] for c in stats["cycle_buffer"]) / 30.0
            avg_error = sum(abs(c[1] - avg_cycle) for c in stats["cycle_buffer"]) / 30.0
            
            # /* Iterate through the buffer to register average statistics per packet index */
            for pkt_idx, _ in stats["cycle_buffer"]:
                stats["avg_cycle_history"].append((pkt_idx, avg_cycle))
                stats["avg_error_history"].append((pkt_idx, avg_error))
                
            stats["cycle_buffer"] = []
            
    stats["last_time"] = current_time
    stats["received"] += 1
    
    # /* Return from execution */
    return

# /**
#  * @brief Parses the composite SF_ECUState_t payload (Info + Data)
#  * @param packet The raw bytes of the incoming packet
#  */
def parse_application_data(packet):
    # /* Guard clause: verify packet meets the exact size requirement */
    if len(packet) != EXPECTED_PACKET_SIZE:
        log(f"Dropped fragment: Size {len(packet)} != {EXPECTED_PACKET_SIZE}")
        
        # /* Abort parsing due to invalid packet dimension */
        return
        
    # /* 1. Extract ECU Info Header */
    header_bytes = packet[:ECU_HEADER_SIZE]
    
    # /* Unpack: 32 bytes (Name), 1 byte (ID), ignore the rest for now */
    ecu_name_raw, ecu_id = struct.unpack("<32s B", header_bytes[:33])
    
    # /* Clean up C-string by stripping null terminators */
    ecu_name = ecu_name_raw.decode('utf-8', errors='ignore').split('\x00')[0]
    
    # /* Steering logic: evaluate if the ECU ID is unregistered */
    if ecu_id not in ecu_registry:
        register_new_ecu(ecu_id, ecu_name)
        
    stats = ecu_registry[ecu_id]["stats"]
        
    # /* 2. Extract Application Data (Offset by ECU_HEADER_SIZE) */
    data_bytes = packet[ECU_HEADER_SIZE : ECU_HEADER_SIZE + DATA_PAYLOAD_SIZE]
    dist_raw, motor_raw = struct.unpack("<II", data_bytes)
    
    # /* Parse bitfields representing hardware data */
    d0 = dist_raw & 0x3FF
    d1 = (dist_raw >> 10) & 0x3FF
    d2 = (dist_raw >> 20) & 0x3FF
    dist_sync = (dist_raw >> 30) & 0x03
    
    m0 = motor_raw & 0x3FF
    m1 = (motor_raw >> 10) & 0x3FF
    motor_sync = (motor_raw >> 20) & 0x03
    
    # /* 3. Update Timing and Validate Sync sequence */
    process_ecu_timing(stats, dist_sync)
    
    # /* 4. Record historical metrics for visualization */
    stats["dist_sync_history"].append(dist_sync)
    stats["motor_sync_history"].append(motor_sync)
    stats["d0_history"].append(d0)
    stats["d1_history"].append(d1)
    stats["d2_history"].append(d2)
    stats["m0_history"].append(m0)
    stats["m1_history"].append(m1)
    
    log(f"--- [ECU: {ecu_name}] DECODED STATE ---")
    log(f" > Distance [D0: {d0:4d}, D1: {d1:4d}, D2: {d2:4d}] | Sync: {dist_sync}")
    log(f" > Motor    [M0: {m0:4d}, M1: {m1:4d}]             | Sync: {motor_sync}")
    log(f"-----------------------------------------\n")
    
    # /* Conclude application data decoding and return */
    return

# /* Branch to main execution context if script is executed directly */
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="W5500 UDP Zonal Architecture Listener")
    parser.add_argument("--packets-num", type=int, default=10, 
                        help="Update plot every N packets. Set 0 to disable.")
    args = parser.parse_args()

    self.BIND_IP = "0.0.0.0" 
    self.BIND_PORT = 30490
    enable_stats = 1 <= args.packets_num <= 1000
    
    log(f"Starting Multi-ECU UDP Listener on {self.BIND_IP}:{self.BIND_PORT}...")

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((self.BIND_IP, self.BIND_PORT))
    
    global_packet_count = 0
    
    # /* Attempt network monitoring within a try-catch block for graceful termination */
    try:
        # /* Infinite loop to continuously monitor incoming UDP datagrams */
        while True:
            data, addr = sock.recvfrom(2048)
            global_packet_count += 1
            
            # /* Parse and route application data dynamically based on content */
            parse_application_data(data)
            
            # /* Steering logic to conditionally render GUI updates based on received packet counts */
            if enable_stats and (global_packet_count % args.packets_num == 0):
                display_plt()
            
    # /* Catch manual termination signal from the operator */
    except KeyboardInterrupt:
        log("Monitoring stopped by user. Closing plot...")
        plt.ioff()
        plt.show() 
        
    # /* Unconditionally execute socket cleanup protocol */
    finally:
        log("Closing socket gracefully.")
        sock.close()