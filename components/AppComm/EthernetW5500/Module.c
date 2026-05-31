#include "Module.h"
#include "PacketQueue.h"

#define WAEKUP_CYCLE_TIME   500000

EthernetW5500_t *   Eth;
SemaphoreHandle_t   EthLock;

TaskHandle_t        W5500CommCtl_TaskHandler;

GenericDeque_t  *   TxDeque;
GenericDeque_t  *   RxDeque;

#define SOCKET_1_UDP_PORT SOMEIP_PORT_HEX

/* INTERNAL TASK **********************************************************************************************************/

static EthRxCallback_t Eth_RxCallback = NULL;

/*
 * @brief Log the Ethernet frame details and hex dump (8 bytes per row)
 * @param Data Pointer to the payload buffer
 * @param Size Length of the data to print
 * @param SrcMAC Pointer to the 6-byte source MAC address (can be NULL)
 * @param DstMAC Pointer to the 6-byte destination MAC address (can be NULL)
 */
static void W5500_LogFrame(GenericPtr_t Data, EthSize_t Size, GenericPtr_t SrcMAC, GenericPtr_t DstMAC) {
    SysLog("W5500_LogFrame");

    Byte_t* pData = Data.UInt8;
    
    SysLog("W5500_LogFrame(...) : Ethernet Frame Log | Size: %u bytes", Size);

    /* Check if destination MAC address is provided */
    if (DstMAC.Byte != NULL) {
        Byte_t* mac = DstMAC.Byte;
        SysLog("Dest MAC: %02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }

    /* Check if source MAC address is provided */
    if (SrcMAC.Byte != NULL) {
        Byte_t* mac = SrcMAC.Byte;
        SysLog("Src MAC: %02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }

    /* Iterate through the payload in 8-byte chunks */
    for (Word_t i = 0; i < Size; i += 8) {
        char line_buf[128];
        int32_t pos = 0;
        pos += sprintf(line_buf + pos, "%04X: ", i);

        /* Process hex data */
        for (int32_t j = 0; j < 8; j++) {
            /* Check if within payload bounds */
            if ((i + j) < Size) {
                pos += sprintf(line_buf + pos, "%02X", pData[i + j]);
            } else {
                pos += sprintf(line_buf + pos, "  ");
            }
            
            /* Apply spacing for all but the last hex byte */
            if (j < 7) {
                pos += sprintf(line_buf + pos, " ");
            }
        }
        
        pos += sprintf(line_buf + pos, " | ");

        /* Process ASCII data representation */
        for (int32_t j = 0; j < 8 && (i + j) < Size; j++) {
            Byte_t byte = pData[i + j];
            
            /* Check if byte is a printable ASCII character */
            if (byte >= 0x20 && byte <= 0x7E) {
                pos += sprintf(line_buf + pos, "%c", byte);
            } else {
                pos += sprintf(line_buf + pos, ".");
            }
            
            /* Apply spacing between ASCII characters */
            if (j < 7 && (i + j + 1) < Size) {
                pos += sprintf(line_buf + pos, " ");
            }
        }
        SysLog("%s", line_buf);
    }
    SysExit("W5500_LogFrame");
}

/*
 * @brief Log network information extracted from the received packet
 * @param pkt Pointer to the packet slot in RxPool
 */
static void W5500_LogUDPInfo(PacketSlot_t* pkt) {
    /* Validate packet pointer */
    if (pkt == NULL) {
        /* Abort if packet is null */
        return;
    }
    
    SysLog("W5500_LogUDPInfo(...) : Network Info Extracted");
    SysLog("    |- Src IP   : %u.%u.%u.%u", 
            pkt->SrcIP.Byte[0], pkt->SrcIP.Byte[1], pkt->SrcIP.Byte[2], pkt->SrcIP.Byte[3]);
    SysLog("    |- Src Port : %u", pkt->SrcPort.Word);
    SysLog("    |- Src MAC  : %02X:%02X:%02X:%02X:%02X:%02X", 
            pkt->SrcMAC.Byte[0], pkt->SrcMAC.Byte[1], pkt->SrcMAC.Byte[2], 
            pkt->SrcMAC.Byte[3], pkt->SrcMAC.Byte[4], pkt->SrcMAC.Byte[5]);
    SysLog("    |- Payload  : %u bytes", pkt->Size);
}

/*
 * @brief Send a packet over UDP (Socket 1) utilizing extracted IP/Port
 * @param tx_pkt Pointer to the packet slot in TxPool
 */
static void W5500_TaskComm_SocketNSendUDP(PacketSlot_t* tx_pkt) {
    Word_t bsb_reg = eBSB_Socket1Register;
    Word_t bsb_tx_buf = eBSB_Socket1TxBuffer;

    SysLog("W5500_TaskComm_SocketNSendUDP(...): UDP Transmit %d bytes to %u.%u.%u.%u:%u", 
        tx_pkt->Size, tx_pkt->SrcIP.Byte[0], tx_pkt->SrcIP.Byte[1], tx_pkt->SrcIP.Byte[2], tx_pkt->SrcIP.Byte[3], tx_pkt->SrcPort.Word);

    W5500_WriteNByteReg(Eth, bsb_reg, eSn_DIPR0, tx_pkt->SrcIP.Byte, 4);
    W5500_WriteDoubleByteReg(Eth, bsb_reg, eSn_DPORT0, tx_pkt->SrcPort.Word);

    Word_t tx_wr = (Word_t)W5500_ReadDoubleByteReg(Eth, bsb_reg, eSn_TX_WR0);
    W5500_WriteNByteReg(Eth, bsb_tx_buf, tx_wr, tx_pkt->Data, tx_pkt->Size);
    W5500_WriteDoubleByteReg(Eth, bsb_reg, eSn_TX_WR0, tx_wr + tx_pkt->Size);

    W5500_WriteByteReg(Eth, bsb_reg, eSn_CR, 0x20);
}

/*
 * @brief Receive packet from a specific W5500 socket
 * @param n The socket number (0-7) to process
 */
static void W5500_TaskComm_SocketNReceive(Byte_t n) {
    Word_t bsb_reg = eBSB_Socket0Register + (n * 4);
    Word_t bsb_rx_buf = eBSB_Socket0RxBuffer + (n * 4);
    Byte_t Sn_IR;
    Word_t rx_rd, packet_size;

    Sn_IR = (Byte_t)W5500_ReadByteReg(Eth, bsb_reg, eSn_IR);
    SysLog("W5500_TaskComm_SocketNReceive(...): S%u_IR = %02X", n, Sn_IR);
    
    /* Check if RECV interrupt bit is asserted */
    if (Sn_IR & 0x04) {
        rx_rd = (Word_t)W5500_ReadDoubleByteReg(Eth, bsb_reg, eSn_RX_RD0);

        #if (SOCKET_0_MACRAW_EN == 1)
        /* Check if processing Socket 0 (MACRAW) */
        if (n == 0) { 
            packet_size = (Word_t)W5500_ReadDoubleByteReg(Eth, bsb_rx_buf, rx_rd);
            Word_t frame_len = packet_size - 2;

            /* Validate frame length before processing */
            if (frame_len > 0) {
                Word_t read_len = (frame_len > 1500) ? 1500 : frame_len;
                W5500_ReadNByteReg(Eth, bsb_rx_buf, rx_rd + 2, Eth->RxFrame.Payload.Byte, read_len);
                SysLog("W5500_TaskComm_SocketNReceive(...): Saved %d bytes of MACRAW frame from S%u", read_len, n);
                
                uint8_t src_mac[6] = {0};
                uint8_t src_ip[4] = {0};
                uint16_t src_port = 0;
                memcpy(src_mac, &Eth->RxFrame.Payload.Byte[6], 6);
                
                /* Check if EtherType is IPv4 */
                if (Eth->RxFrame.Payload.Byte[12] == 0x08 && Eth->RxFrame.Payload.Byte[13] == 0x00) { 
                    memcpy(src_ip, &Eth->RxFrame.Payload.Byte[26], 4);
                    /* Check if Protocol is UDP */
                    if (Eth->RxFrame.Payload.Byte[23] == 17) { 
                        src_port = (Eth->RxFrame.Payload.Byte[34] << 8) | Eth->RxFrame.Payload.Byte[35];
                    }
                }
                RxedPacket_Push(read_len, (GenericPtr_t) Eth->RxFrame.Payload.Byte, src_ip, src_port, src_mac);
            }
            rx_rd += packet_size;
        }
        #endif

        #if (SOCKET_1_UDP_EN == 1)
        /* Check if processing Socket 1 (UDP) */
        if (n == 1) { 
            Byte_t udp_header[8];
            Word_t data_len;

            W5500_ReadNByteReg(Eth, bsb_rx_buf, rx_rd, udp_header, 8);
            data_len = (udp_header[6] << 8) | udp_header[7];
            
            uint8_t src_ip[4] = {udp_header[0], udp_header[1], udp_header[2], udp_header[3]};
            uint16_t src_port = (udp_header[4] << 8) | udp_header[5];
            uint8_t src_mac[6] = {0}; 

            /* Ensure extracted data length is valid */
            if (data_len > 0) {
                Word_t read_len = (data_len > 1500) ? 1500 : data_len;
                W5500_ReadNByteReg(Eth, bsb_rx_buf, rx_rd + 8, Eth->RxFrame.Payload.Byte, read_len);
                SysLog("W5500_TaskComm_SocketNReceive(...): Saved %d bytes of UDP payload from S%u", read_len, n);
                RxedPacket_Push(read_len, (GenericPtr_t) Eth->RxFrame.Payload.Byte, src_ip, src_port, src_mac);
            }
            packet_size = 8 + data_len;
            rx_rd += packet_size;
        }
        #endif

        W5500_WriteDoubleByteReg(Eth, bsb_reg, eSn_RX_RD0, rx_rd);
        W5500_WriteByteReg(Eth, bsb_reg, eSn_CR, 0x40);
    }
    
    /* Check if SEND_OK interrupt is asserted */
    if (Sn_IR & 0x10) { 
        SysLog("W5500_TaskComm_SocketNReceive(...): S%u Data Sent", n); 
    }
    
    W5500_WriteByteReg(Eth, bsb_reg, eSn_IR, 0xFF);
}

/*
 * @brief This task will be woken up when an isr is occurred!
 * @param arg Pointer to the EthernetW5500_t structure
 */
void W5500_TaskComm_GetPacket(void* arg) {
    Byte_t Attempt;
    Byte_t const MaxAttempt = 3;
    Byte_t SIR, IR, n;

    /* Infinite loop for the worker task */
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        SysLog("W5500_TaskComm_GetPacket(...): Task is woken up!");
        
        Attempt = 0;
        
        /* Process module interrupts until clear or max attempts reached */
        do {
            SIR = (Byte_t)W5500_ReadByteReg(Eth, eBSB_CommonRegister, eSIR);
            IR  = (Byte_t)W5500_ReadByteReg(Eth, eBSB_CommonRegister, eIR);
            
            /* Check if any socket interrupted */
            if (SIR) {
                /* Iterate through all 8 sockets */
                for (n = 0; n < 8; ++n) {
                    /* Check if socket 'n' is the source of the interrupt */
                    if ((1U << n) & SIR) {
                        W5500_TaskComm_SocketNReceive(n);
                    }
                }
            }
            
            /* Clear global interrupt flag if set */
            if (IR) { 
                W5500_WriteByteReg(Eth, eBSB_CommonRegister, eIR, IR); 
            }
            
        } while (SIR || (Attempt++ < MaxAttempt));
        
        PacketSlot_t* tx_pkt;
        
        /* Process all queued packets for transmission */
        while ((tx_pkt = TxPacket_GetHighestPriority()) != NULL) {
            #if (SOCKET_1_UDP_EN == 1)
            W5500_TaskComm_SocketNSendUDP(tx_pkt);
            #endif
            TxPacket_Release(tx_pkt);
        }
        
        /* Notify control task if new packets have arrived */
        if(RxedPacket_Size() > 0) {
            xTaskNotifyGive(W5500CommCtl_TaskHandler);
        }
    }
}

/* MODULE INIT *************************************************************************************************************/

/*
 * @brief Initialize W5500 network interface for MACRAW and ICMP
 * @param Eth Pointer to the Ethernet W5500 controller structure
 * @return STAT_OKE upon successful initialization
 */
ReturnCode_t W5500CommCtl_Init(EthernetW5500_t * Eth) { 
    uint64_t Data;
    uint8_t dummy_reg;
    
    SysEntry("W5500CommCtl_Init");

    EthLock = xSemaphoreCreateBinary();
    xSemaphoreGive(EthLock);

    W5500_WriteByteReg(Eth, eBSB_CommonRegister, eMR, 0x80);
    Data = W5500_GetModuleVersion(Eth);
    
    /* Validate W5500 silicon version */
    if ((Data & 0xFF) != 0x04) {
        SysErr("W5500CommCtl_Init(...): W5500 Version (=%X) is not supported!", Data & 0xFF);
        /* Abort initialization due to invalid hardware */
        return STAT_ERR_UNSUPPORTED;
    }
    SysLog("W5500CommCtl_Init(...): W5500 Version = %X", Data & 0xFF);

    W5500_WriteQuartByteReg(Eth, eBSB_CommonRegister, eGAR0, ConvertByteArrayToInt32_BigEndian(GW_IP_ADDR, 4));
    W5500_WriteQuartByteReg(Eth, eBSB_CommonRegister, eSUBR0, ConvertByteArrayToInt32_BigEndian(SUBNET_MASK, 4));
    W5500_WriteQuartByteReg(Eth, eBSB_CommonRegister, eSIPR0, ConvertByteArrayToInt32_BigEndian(SRC_IP_ADDR, 4));
    
    Data = ConvertByteArrayToInt64_BigEndian(SRC_MAC_ADDR, 6);
    W5500_WriteNByteReg(Eth, eBSB_CommonRegister, eSHAR0, (void*) &Data, 6);

    Byte_t simr_val = 0;
    #if (SOCKET_0_MACRAW_EN == 1)
        simr_val |= (1 << 0);
    #endif
    #if (SOCKET_1_UDP_EN == 1)
        simr_val |= (1 << 1);
    #endif
    W5500_WriteByteReg(Eth, eBSB_CommonRegister, eSIMR, simr_val);
    W5500_WriteDoubleByteReg(Eth, eBSB_CommonRegister, eINTLEVEL0, 0x03E8);
    W5500_WriteByteReg(Eth, eBSB_CommonRegister, eIMR, 0x90);

    dummy_reg = W5500_ReadByteReg(Eth, eBSB_CommonRegister, eIR); 
    W5500_WriteByteReg(Eth, eBSB_CommonRegister, eIR, dummy_reg);

    dummy_reg = W5500_ReadByteReg(Eth, eBSB_CommonRegister, eSIR);
    W5500_WriteByteReg(Eth, eBSB_CommonRegister, eSIR, dummy_reg);

    #if (SOCKET_0_MACRAW_EN == 1)
    W5500_WriteByteReg(Eth, eBSB_Socket0Register, eSn_RXBUF_SIZE, 8); 
    W5500_WriteByteReg(Eth, eBSB_Socket0Register, eSn_TXBUF_SIZE, 0); 
    #endif
    #if (SOCKET_1_UDP_EN == 1)
    W5500_WriteByteReg(Eth, eBSB_Socket1Register, eSn_RXBUF_SIZE, 8); 
    W5500_WriteByteReg(Eth, eBSB_Socket1Register, eSn_TXBUF_SIZE, 8); 
    #endif
    
    /* Disable memory buffers for unused sockets */
    for (int32_t i = 2; i < 8; i++) {
        W5500_WriteByteReg(Eth, eBSB_Socket0Register + (i * 4), eSn_RXBUF_SIZE, 0);
        W5500_WriteByteReg(Eth, eBSB_Socket0Register + (i * 4), eSn_TXBUF_SIZE, 0);
    }

    #if (SOCKET_0_MACRAW_EN == 1)
    SysLog("W5500CommCtl_Init(...): Configure Socket 0 for MACRAW");
    W5500_WriteByteReg(Eth, eBSB_Socket0Register, eSn_IMR, 0x14); 
    W5500_WriteByteReg(Eth, eBSB_Socket0Register, eSn_MR, 0x04);  
    W5500_WriteByteReg(Eth, eBSB_Socket0Register, eSn_CR, 0x01);
    SysLog("W5500CommCtl_Init(...): Verify eSn_SR=%02X (Expected: 0x42)", W5500_ReadByteReg(Eth, eBSB_Socket0Register, eSn_SR));
    #endif

    #if (SOCKET_1_UDP_EN == 1)
    SysLog("W5500CommCtl_Init(...): Configure Socket 1 for UDP on port %d", SOCKET_1_UDP_PORT);
    W5500_WriteByteReg(Eth, eBSB_Socket1Register, eSn_IMR, 0x04); 
    W5500_WriteByteReg(Eth, eBSB_Socket1Register, eSn_MR, 0x02);  
    W5500_WriteDoubleByteReg(Eth, eBSB_Socket1Register, eSn_PORT0, SOCKET_1_UDP_PORT); 
    W5500_WriteByteReg(Eth, eBSB_Socket1Register, eSn_CR, 0x01); 
    SysLog("W5500CommCtl_Init(...): Verify eSn_SR=%02X (Expected: 0x22)", W5500_ReadByteReg(Eth, eBSB_Socket1Register, eSn_SR));
    #endif

    SysExit("W5500CommCtl_Init");
    
    /* Initialization complete, return success */
    return STAT_OKE;
}

/* MODULE PUBLIC APIs ******************************************************************************************************/

/*
 * @brief Configure the global IPv4 Gateway Address.
 * @param IPv4Addr The 32-bit IP address.
 * @return ReturnCode_t STAT_OKE on success.
 */
ReturnCode_t IPv4SetGateWayAddress(uint32_t IPv4Addr) {
    /* Validate module state */
    if (Eth == NULL) {
        /* Reject operation if uninitialized */
        return STAT_ERR_INVALID_STATE;
    }
    /* Write to hardware and return result */
    return W5500_WriteQuartByteReg(Eth, eBSB_CommonRegister, eGAR0, IPv4Addr);
}

/*
 * @brief Configure the global IPv4 Subnet Mask.
 * @param IPv4SubnetMask The 32-bit subnet mask.
 * @return ReturnCode_t STAT_OKE on success.
 */
ReturnCode_t Eth_SetIPv4SubnetMask(uint32_t IPv4SubnetMask) {
    /* Validate module state */
    if (Eth == NULL) {
        /* Reject operation if uninitialized */
        return STAT_ERR_INVALID_STATE;
    }
    /* Write to hardware and return result */
    return W5500_WriteQuartByteReg(Eth, eBSB_CommonRegister, eSUBR0, IPv4SubnetMask);
}

/*
 * @brief Configure the global Source IPv4 Address.
 * @param IPv4Addr The 32-bit IP address.
 * @return ReturnCode_t STAT_OKE on success.
 */
ReturnCode_t Eth_SetSrcIPv4Address(uint32_t IPv4Addr) {
    /* Validate module state */
    if (Eth == NULL) {
        /* Reject operation if uninitialized */
        return STAT_ERR_INVALID_STATE;
    }
    /* Write to hardware and return result */
    return W5500_WriteQuartByteReg(Eth, eBSB_CommonRegister, eSIPR0, IPv4Addr);
}

/*
 * @brief Wrapper to set the source IPv4 Address.
 * @param IPv4Addr The 32-bit IP address.
 * @return ReturnCode_t STAT_OKE on success.
 */
ReturnCode_t EthSetIPv4SourceAddress(uint32_t IPv4Addr) {
    /* Delegate to primary setter function */
    return Eth_SetSrcIPv4Address(IPv4Addr);
}

/*
 * @brief Configure the source port for Socket 1.
 * @param IPv4Port The 16-bit port number.
 * @return ReturnCode_t STAT_OKE on success.
 */
ReturnCode_t Eth_SetSrcIPv4Port(uint16_t IPv4Port) {
    /* Validate module state */
    if (Eth == NULL) {
        /* Reject operation if uninitialized */
        return STAT_ERR_INVALID_STATE;
    }
    /* Write to hardware and return result */
    return W5500_WriteDoubleByteReg(Eth, eBSB_Socket1Register, eSn_PORT0, IPv4Port);
}

/*
 * @brief Configure the transmission and reception buffer sizes for Socket 1.
 * @param TxSizeKB TX buffer size in Kilobytes.
 * @param RxSizeKB RX buffer size in Kilobytes.
 * @return ReturnCode_t STAT_OKE on success.
 */
ReturnCode_t Eth_SetUDPBufferSize(uint8_t TxSizeKB, uint8_t RxSizeKB) {
    /* Validate module state */
    if (Eth == NULL) {
        /* Reject operation if uninitialized */
        return STAT_ERR_INVALID_STATE;
    }
    W5500_WriteByteReg(Eth, eBSB_Socket1Register, eSn_TXBUF_SIZE, TxSizeKB);
    W5500_WriteByteReg(Eth, eBSB_Socket1Register, eSn_RXBUF_SIZE, RxSizeKB);
    /* Conclude buffer setup */
    return STAT_OKE;
}

/*
 * @brief Retrieve the highest priority packet from the receive queue.
 * @param pkt Pointer to store the packet slot address.
 * @return ReturnCode_t STAT_OKE if packet found.
 */
ReturnCode_t Eth_GetRxPacket(PacketSlot_t** pkt) {
    /* Validate output parameter pointer */
    if (pkt == NULL) {
        /* Abort on null pointer */
        return STAT_ERR_NULL;
    }
    *pkt = RxedPacket_GetHighestPriority();
    
    /* Return OK if valid packet retrieved, else NOT_FOUND */
    return (*pkt != NULL) ? STAT_OKE : STAT_ERR_NOT_FOUND;
}

/*
 * @brief Retrieve the highest priority packet from the transmit queue.
 * @param pkt Pointer to store the packet slot address.
 * @return ReturnCode_t STAT_OKE if packet found.
 */
ReturnCode_t Eth_GetTxPacket(PacketSlot_t** pkt) {
    /* Validate output parameter pointer */
    if (pkt == NULL) {
        /* Abort on null pointer */
        return STAT_ERR_NULL;
    }
    *pkt = TxPacket_GetHighestPriority();
    
    /* Return OK if valid packet retrieved, else NOT_FOUND */
    return (*pkt != NULL) ? STAT_OKE : STAT_ERR_NOT_FOUND;
}

/*
 * @brief Wrapper to retrieve a UDP packet.
 * @param pkt Pointer to store the packet slot address.
 * @return ReturnCode_t STAT_OKE if packet found.
 */
ReturnCode_t Eth_GetUDPPacket(PacketSlot_t** pkt) {
    /* Route to generic receive retrieval */
    return Eth_GetRxPacket(pkt);
}

/*
 * @brief Extract network information from a given packet slot.
 * @param pkt Pointer to the packet slot.
 * @param srcIP Buffer to extract 4-byte IP.
 * @param srcPort Pointer to extract 2-byte port.
 * @param srcMAC Buffer to extract 6-byte MAC.
 * @return ReturnCode_t STAT_OKE on success.
 */
ReturnCode_t Eth_GetUDPInfo(PacketSlot_t* pkt, uint8_t* srcIP, uint16_t* srcPort, uint8_t* srcMAC) {
    /* Validate packet slot pointer */
    if (pkt == NULL) {
        /* Reject null packet */
        return STAT_ERR_NULL;
    }
    /* Extract data conditionally based on valid pointers */
    if (srcIP) {
        memcpy(srcIP, pkt->SrcIP.Byte, 4);
    }
    if (srcPort) {
        *srcPort = pkt->SrcPort.Word;
    }
    if (srcMAC) {
        memcpy(srcMAC, pkt->SrcMAC.Byte, 6);
    }
    /* Return successful extraction */
    return STAT_OKE;
}

/*
 * @brief Extract the raw payload data and size from a packet slot.
 * @param pkt Pointer to the packet slot.
 * @param payload Double pointer to store payload address.
 * @param size Pointer to store payload size.
 * @return ReturnCode_t STAT_OKE on success.
 */
ReturnCode_t Eth_GetUDPPayload(PacketSlot_t* pkt, uint8_t** payload, uint16_t* size) {
    /* Validate all input pointers */
    if (pkt == NULL || payload == NULL || size == NULL) {
        /* Abort due to missing parameters */
        return STAT_ERR_NULL;
    }
    *payload = pkt->Data;
    *size = pkt->Size;
    /* Return successful payload extraction */
    return STAT_OKE;
}

/*
 * @brief Register a callback function for received Ethernet packets.
 * @param cb The callback function pointer.
 * @return ReturnCode_t STAT_OKE on success.
 */
ReturnCode_t Eth_SetRxCallback(EthRxCallback_t cb) {
    Eth_RxCallback = cb;
    /* Confirm callback registration */
    return STAT_OKE;
}

/*
 * @brief Queue a UDP packet for transmission.
 * @param IPv4Address Destination IP address.
 * @param IPv4Port Destination port.
 * @param UDP_Payload Pointer to the data payload.
 * @param PayloadSize Length of the data.
 * @return ReturnCode_t STAT_OKE if successfully queued.
 */
ReturnCode_t Eth_SendUDPPacket(uint32_t IPv4Address, uint16_t IPv4Port, uint8_t* UDP_Payload, uint16_t PayloadSize) {
    uint8_t ip_bytes[4];
    ip_bytes[0] = (IPv4Address >> 24) & 0xFF;
    ip_bytes[1] = (IPv4Address >> 16) & 0xFF;
    ip_bytes[2] = (IPv4Address >>  8) & 0xFF;
    ip_bytes[3] = (IPv4Address      ) & 0xFF;

    ReturnCode_t ret = TxPacket_Push(PayloadSize, (GenericPtr_t)UDP_Payload, ip_bytes, IPv4Port, NULL);
    
    /* Verify push operation success */
    if (ret == STAT_OKE) {
        /* Check if communication task is ready */
        if (W5500_TaskComm_TaskHandler != NULL) {
            xTaskNotifyGive(W5500_TaskComm_TaskHandler);
        }
    }
    
    /* Return queue result */
    return ret;
}

/*
 * @brief Default weak callback for processing incoming packets. Acts as an Echo server.
 * @param pkt Pointer to the received packet slot.
 */
__attribute__((weak)) void Eth_WeakRxCallback(PacketSlot_t* pkt) {
    /* Ignore if packet pointer is invalid */
    if (pkt == NULL) {
        /* Exit early */
        return;
    }
    SysLog("Eth_WeakRxCallback(...):  Process packet, Size=%d, Prio ID=0x%02X", pkt->Size, pkt->Data[0]);
    
    W5500_LogUDPInfo(pkt);
    W5500_LogFrame((GenericPtr_t)pkt->Data, pkt->Size, GenericNullPtr, GenericNullPtr);
    
    TxPacket_Push(pkt->Size, (GenericPtr_t)pkt->Data, pkt->SrcIP.Byte, pkt->SrcPort.Word, pkt->SrcMAC.Byte);
    
    /* Verify communication task readiness before notification */
    if (W5500_TaskComm_TaskHandler != NULL) {
        xTaskNotifyGive(W5500_TaskComm_TaskHandler);
    }
}

/* INTERNAL TASK **********************************************************************************************************/

/*
 * @brief  Serivce handle W5500 module
 * @param arg Ignored
 */
void W5500CommRuntime(void* arg){
    SysEntry("W5500CommCtl");
    
    ReturnCode_t RetVal;
    
    RxedPacket_Init();
    
    SysLog("W5500CommCtl(...) : New motor object...");
    Eth = W5500_Create(PIN_W5500_MISO, PIN_W5500_MOSI, PIN_W5500_CLK, PIN_W5500_SCS, PIN_W5500_RST, PIN_W5500_INT);
    
    /* Ensure the Ethernet hardware object was constructed */
    if (IsNull(Eth)) {
        SysErr("W5500CommCtl(...) : Cannot initialize `Eth`!");
        /* Branch to cleanup on fatal error */
        goto CLEANUP_AND_EXIT;
    }
    
    RetVal = W5500CommCtl_Init(Eth);
    
    /* Validate hardware initialization status */
    if(RetVal != STAT_OKE){
        SysErr("W5500CommCtl(...): Cannot initialize module W5500! ErrorCode=%d", RetVal);
        /* Branch to cleanup if setup failed */
        goto CLEANUP_AND_EXIT;
    }
    
    W5500_TaskComming_Function = W5500_TaskComm_GetPacket;
    W5500CommCtl_TaskHandler = xTaskGetCurrentTaskHandle();
    xTaskCreate(
        W5500_TaskComming_Function,
        "",
        1024,
        NULL,
        eTask_Urgency,
        &W5500_TaskComm_TaskHandler
    );
    
    SysLog("W5500CommCtl(...) : Join forever loop...");
    
    /* Run the main network management routine indefinitely */
    while (1) {
        SysLog("W5500CommCtl(...) : W5500 version=%X", W5500_GetModuleVersion(Eth));

        SysLog("W5500CommCtl(...):  Check RxedPacket and log...");
        SysLog("W5500CommCtl(...):  #Rexed packets=%d", RxedPacket_Size());
        
        PacketSlot_t* pkt;
        
        /* Drain all available packets from the receive queue */
        while((pkt = RxedPacket_GetHighestPriority()) != NULL){
            /* Route to custom callback if registered */
            if (Eth_RxCallback != NULL) {
                Eth_RxCallback(pkt);
            } else {
                /* Fallback to default weak callback */
                Eth_WeakRxCallback(pkt);
            }
            
            RxedPacket_Release(pkt);
            SysLog("");
        }

        SysLog("");

        SysLog("W5500CommCtl(...) : Task will be put to sleep!");
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(WAEKUP_CYCLE_TIME));
        
        SysLog("");
        SysLog("W5500CommCtl(...) : Task woken up!");
    }

    CLEANUP_AND_EXIT:
        vSemaphoreDelete(EthLock);
        EthLock = NULL;
        vTaskDelete(NULL);
}

/* EOF *******************************************************************************************************************/
