import socket
import struct
import select

# Target IPs and Ports
LISTEN_IP_LIST = ["10.0.0.100", "10.0.0.47", "10.0.0.58"]
LISTEN_IP_PORT = [30490, 51786]

# Map FrameType ID to readable name
FRAME_TYPES = {
    0xFFFFFFFF: "Broadcast",
    0x55000001: "PairingRequest",
    0xAA00000A: "AuthTX",
    0xAA00000B: "AuthRX",
    0xAA00000C: "AuthFail",
    0x5500AA00: "KeepAlive",
    0xFF00FF0A: "HeartBeat",
    0xFF00CC0A: "EngineControl1",
    0xFF00CC0B: "EngineControl2",
    0xFF00CC0C: "EngineControl3",
    0xF0F0F0FA: "EmergencyStop"
}

def format_mac(mac_bytes):
    return ":".join(f"{b:02X}" for b in mac_bytes)

def parse_zecu_frame(payload, src_ip, src_port, dst_port):
    # 88 bytes (Header) + 8 bytes (Trailer) = 96 bytes minimum
    if len(payload) < 96:
        return 

    print("\n" + "="*80)
    print("PACKET[\"UDP\"]:")
    # Note: Standard UDP recvfrom only provides the local bound port (dst_port), 
    # not the exact destination IP if bound to 0.0.0.0
    print(f"SRC:{src_ip}:{src_port:<12} DST_PORT: {dst_port}")
    
    try:
        # --- PARSE HEADER (88 Bytes) ---
        # Format: 4x uint8, 64s char, 6s uint8, 1x uint8, 4s uint8, 1x uint16, 3x uint8, 1x uint32
        hdr_fmt = "<BBBB 64s 6s B 4s H BBB I"
        header = struct.unpack(hdr_fmt, payload[:88])
        
        magic0, version, magic1, magic2 = header[0:4]
        label = header[4].decode('utf-8', errors='ignore').strip('\x00')
        mac = format_mac(header[5])
        magic3 = header[6]
        ipv4 = socket.inet_ntoa(header[7])
        port = header[8]
        magic4, magic5, magic6 = header[9:12]
        frame_type = header[12]
        frame_name = FRAME_TYPES.get(frame_type, "UNKNOWN")

        print("APPFRAME.HEADER:")
        print(f"        uint8_t             MagicByte0;             : 0x{magic0:02X}")
        print(f"        uint8_t             Version;                : {version}")
        print(f"        uint8_t             MagicByte1;             : 0x{magic1:02X}")
        print(f"        uint8_t             MagicByte2;             : 0x{magic2:02X}")
        print("        ")
        print("        Info {")
        print(f"            char            Label[];                : {label}")
        print(f"            uint8_t         MAC[];                  : {mac}")
        print(f"            uint8_t         MagicByte3;             : 0x{magic3:02X}")
        print(f"            uint8_t         IPv4Address[];          : {ipv4}")
        print(f"            uint16_t        IPv4Port;               : {port}")
        print(f"            uint8_t         MagicByte4;             : 0x{magic4:02X}")
        print(f"            uint8_t         MagicByte5;             : 0x{magic5:02X}")
        print(f"            uint8_t         MagicByte6;             : 0x{magic6:02X}")
        print("        }")
        print(f"        uint32_t            FrameType;              : 0x{frame_type:08X} ({frame_name})")

        # --- PARSE BODY ---
        body_bytes = payload[88:-8]
        print(f"\nAPPFRAME.BODY.{frame_name}:")
        
        if frame_type == 0xFFFFFFFF: # Broadcast
            bd_fmt = "< I B 3s"
            b_dword, s_count, _ = struct.unpack(bd_fmt, body_bytes[:8])
            print(f"        uint32_t            MagicDWord0;            : 0x{b_dword:08X}")
            print(f"        uint8_t             ServiceCount;           : {s_count}")
            print("        uint8_t             _Padding[3];            : <Padding>")
            print("        ZECUFrame_Service_t Services[];             :")
            
            srv_offset = 8
            for i in range(min(s_count, 8)):
                srv_bytes = body_bytes[srv_offset : srv_offset+64]
                if len(srv_bytes) < 64: break
                s_id, s_type, s_access, s_desc = struct.unpack("< H H B 59s", srv_bytes)
                desc_str = s_desc.decode('utf-8', errors='ignore').strip('\x00')
                print(f"            [{i}] ID: 0x{s_id:04X}   Type: 0x{s_type:04X}   Access: 0x{s_access:02X}     Desc: {desc_str}")
                srv_offset += 64

        elif frame_type == 0xFF00FF0A: # HeartBeat
            hb_fmt = "< B B B H H H B b b B I I B B B"
            hb = struct.unpack(hb_fmt, body_bytes[:24])
            print(f"        uint8_t             Magic0;                 : 0x{hb[0]:02X}")
            print(f"        uint8_t             SyncNum;                : {hb[1]}")
            print(f"        uint16_t            D0;                     : {hb[3]}")
            print(f"        uint16_t            D1;                     : {hb[4]}")
            print(f"        uint16_t            D2;                     : {hb[5]}")
            print(f"        int8_t              M0;                     : {hb[7]}")
            print(f"        int8_t              M1;                     : {hb[8]}")
            print(f"        uint32_t            Location_Long;          : {hb[10]}")
            print(f"        uint32_t            Location_Lat;           : {hb[11]}")

        elif frame_type == 0xF0F0F0FA: # EmergencyStop
            es_fmt = "< B I B H H H B b b B I I B B H"
            es = struct.unpack(es_fmt, body_bytes[:28])
            print(f"        uint32_t            ReasonID;               : 0x{es[1]:08X}")
            print(f"        uint16_t            D0;                     : {es[3]}")
            print(f"        uint16_t            D1;                     : {es[4]}")
            print(f"        uint16_t            D2;                     : {es[5]}")
            print(f"        int8_t              M0;                     : {es[7]}")
            print(f"        int8_t              M1;                     : {es[8]}")
            print(f"        uint32_t            Location_Long;          : {es[10]}")
            print(f"        uint32_t            Location_Lat;           : {es[11]}")

        elif frame_type in [0xAA00000A, 0xAA00000B]: # AuthTX / AuthRX
            auth_fmt = "< B Q B B B"
            auth = struct.unpack(auth_fmt, body_bytes[:12])
            print(f"        uint64_t            Chal/Sig;               : 0x{auth[1]:016X}")

        else:
            print(f"        <Payload Data Hex>                          : {body_bytes.hex()[:60]}...")

        # --- PARSE TRAILER (Last 8 Bytes) ---
        trl_fmt = "< B H B I"
        trailer = struct.unpack(trl_fmt, payload[-8:])
        
        print("\nAPPFRAME.TRAILER:")
        print(f"        uint8_t              MagicByte0;            : 0x{trailer[0]:02X}")
        print(f"        uint16_t             CheckSum;              : 0x{trailer[1]:04X}")
        print(f"        uint8_t              MagicByte1;            : 0x{trailer[2]:02X}")
        print(f"        uint32_t             CRC;                   : 0x{trailer[3]:08X}")

    except Exception as e:
        print(f"\n[!] Parsing error: {e}")

def main():
    sockets = []
    port_map = {}

    # Initialize a shared socket for each port in the target list
    for port in LISTEN_IP_PORT:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        
        # Ensure the socket does not block the port exclusively
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        
        # Required on some Unix/Linux systems to allow true port sharing
        if hasattr(socket, 'SO_REUSEPORT'):
            try:
                sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
            except AttributeError:
                pass
        
        # Bind to all available interfaces to catch incoming traffic
        sock.bind(('0.0.0.0', port))
        sockets.append(sock)
        port_map[sock] = port
        
        print(f"[*] Listening on shared UDP port {port}...")

    print("[*] Press Ctrl+C to stop...\n")

    try:
        # Use select to multiplex blocking I/O across all bound sockets
        while True:
            readable, _, _ = select.select(sockets, [], [])
            
            for s in readable:
                payload, addr = s.recvfrom(65535)
                src_ip, src_port = addr
                dst_port = port_map[s]

                # Filter incoming packets by allowed IP list
                if src_ip in LISTEN_IP_LIST:
                    parse_zecu_frame(payload, src_ip, src_port, dst_port)

    except KeyboardInterrupt:
        print("\n[*] Sniffer stopped by user.")
    finally:
        # Clean up sockets gracefully
        for s in sockets:
            s.close()

if __name__ == "__main__":
    main()