import re
import numpy as np
import matplotlib.pyplot as plt

ZONE_ECU_IP = '10.0.0.58'
CCU_IP = '10.0.0.102'

def parse_log_file(filepath):
    """
    @brief Parses the log file and extracts relevant network packets.
    @param filepath Path to the input text file.
    @return List of parsed packet dictionaries.
    """
    packets = []
    
    with open(filepath, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            
            # Route execution to skip empty lines
            if not line:
                continue
                
            parts = re.split(r'\s+', line)
            
            # Route execution to ensure line has enough columns to be valid
            if len(parts) >= 4:
                try:
                    time = float(parts[1])
                    src = parts[2]
                    dst = parts[3]
                    
                    # Route execution to filter out irrelevant packets
                    if (src == ZONE_ECU_IP and dst == CCU_IP) or (src == CCU_IP and dst == ZONE_ECU_IP):
                        packets.append({
                            'time': time, 
                            'src': src, 
                            'dst': dst
                        })
                except ValueError:
                    # Skip lines where time parsing fails
                    continue
                    
    # Terminate function and yield parsed data
    return packets

def analyze_timings(packets):
    """
    @brief Categorizes packets and calculates cycle and response times based on strict segment boundaries.
    @param packets List of packet dictionaries.
    @return Tuple containing list of cycle times and list of response times (in ms).
    """
    cycle_times_ms = []
    response_times_ms = []
    
    waiting_for_response = False
    last_ccu_time = 0.0
    
    reference_time = 0.0
    has_reference_for_cycle = False
    
    for pkt in packets:
        # Route execution for CCU sending a command (Segment Boundary)
        if pkt['src'] == CCU_IP:
            waiting_for_response = True
            last_ccu_time = pkt['time']
            
            # Break the cycle time calculation chain here because CCU interrupted
            has_reference_for_cycle = False 
            
        # Route execution for ZoneECU sending a packet
        elif pkt['src'] == ZONE_ECU_IP:
            if waiting_for_response:
                # Type 2: Immediate response packet after CCU command
                diff_ms = (pkt['time'] - last_ccu_time) * 1000.0
                response_times_ms.append(diff_ms)
                
                # Response completed, reset wait flag
                waiting_for_response = False
                
                # This response packet is an anomaly, absolutely DO NOT use as cycle time reference
                has_reference_for_cycle = False
                
            else:
                # Type 1: Normal Heartbeat
                if has_reference_for_cycle:
                    # Have valid reference from previous Normal Heartbeat -> Calculate cycle
                    diff_ms = (pkt['time'] - reference_time) * 1000.0
                    cycle_times_ms.append(diff_ms)
                    
                # The current Normal Heartbeat packet becomes the reference for the next Normal one
                # (Note: If this is the first packet after an interruption, it only sets reference without calculating diff)
                reference_time = pkt['time']
                has_reference_for_cycle = True
                
    # Terminate function and yield extracted timings
    return cycle_times_ms, response_times_ms

def plot_distributions(cycle_times, response_times):
    """
    @brief Plots histograms and mean lines for the calculated timings.
    @param cycle_times List of cycle intervals in milliseconds.
    @param response_times List of response delays in milliseconds.
    """
    fig, axes = plt.subplots(1, 2, figsize=(14, 6))
    
    # Process Plot 1: Normal Heartbeat Cycle Time
    if cycle_times:
        mean_cycle = np.mean(cycle_times)
        axes[0].hist(cycle_times, bins=20, color='skyblue', edgecolor='black', alpha=0.7)
        axes[0].axvline(mean_cycle, color='red', linestyle='dashed', linewidth=2, label=f'Mean: {mean_cycle:.2f} ms')
        axes[0].set_title('Type 1: Normal Heartbeat Cycle Distribution')
        axes[0].set_xlabel('Cycle Time (ms)')
        axes[0].set_ylabel('Frequency')
        axes[0].legend()
        print(f"[*] Average Cycle Time (Type 1): {mean_cycle:.2f} ms")
    else:
        axes[0].text(0.5, 0.5, 'No cycle data found', horizontalalignment='center', verticalalignment='center')
        
    # Process Plot 2: Immediate Response Time
    if response_times:
        mean_resp = np.mean(response_times)
        axes[1].hist(response_times, bins=15, color='lightgreen', edgecolor='black', alpha=0.7)
        axes[1].axvline(mean_resp, color='red', linestyle='dashed', linewidth=2, label=f'Mean: {mean_resp:.2f} ms')
        axes[1].set_title('Type 2: Immediate Response Time Distribution')
        axes[1].set_xlabel('Response Time (ms)')
        axes[1].set_ylabel('Frequency')
        axes[1].legend()
        print(f"[*] Average Response Time (Type 2): {mean_resp:.2f} ms")
    else:
        axes[1].text(0.5, 0.5, 'No response data found', horizontalalignment='center', verticalalignment='center')
        
    plt.tight_layout()
    plt.show()
    
    return

if __name__ == "__main__":
    """
    @brief Main execution entry point.
    """
    filepath = 'input.txt'
    
    parsed_packets = parse_log_file(filepath)
    cycles, responses = analyze_timings(parsed_packets)
    
    print("-" * 40)
    plot_distributions(cycles, responses)
    print("-" * 40)