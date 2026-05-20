#include "Module.h"
#include "PacketQueue.h"

#define WAEKUP_CYCLE_TIME   500000

EthernetW5500_t *   Eth;
SemaphoreHandle_t   EthLock;

EthernetW5500_t *   Eth;
TaskHandle_t        W5500CommCtl_TaskHandler;

GenericDeque_t  *   TxDeque;
GenericDeque_t  *   RxDeque;

#define SOCKET_1_UDP_PORT 7979

/* INTERNAL TASK **********************************************************************************************************/

static EthRxCallback_t g_EthRxCallback = NULL;

/// @brief Log the Ethernet frame details and hex dump (8 bytes per row)
/// @param Data Pointer to the payload buffer
/// @param Size Length of the data to print
/// @param SrcMAC Pointer to the 6-byte source MAC address (can be NULL)
/// @param DstMAC Pointer to the 6-byte destination MAC address (can be NULL)
static void W5500_LogFrame(GenericPtr_t Data, EthSize_t Size, GenericPtr_t SrcMAC, GenericPtr_t DstMAC) {
    SysLog("W5500_LogFrame");

    Byte_t* pData = Data.UInt8;
    
    /* Log frame summary and check if MAC address pointers are valid before printing */
    SysLog("W5500_LogFrame(...) : Ethernet Frame Log | Size: %u bytes", Size);

    if (DstMAC.Byte != NULL) {
        Byte_t* mac = DstMAC.Byte;
        SysLog("Dest MAC: %02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }

    if (SrcMAC.Byte != NULL) {
        Byte_t* mac = SrcMAC.Byte;
        SysLog("Src MAC: %02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }

    /* Iterate through the data buffer and generate a hex/ASCII dump 8 bytes at a time */
    for (Word_t i = 0; i < Size; i += 8) {
        char line_buf[128];
        int32_t pos = 0;
        pos += sprintf(line_buf + pos, "%04X: ", i);

        /* Hex Data */
        for (int32_t j = 0; j < 8; j++) {
            if ((i + j) < Size) {
                pos += sprintf(line_buf + pos, "%02X", pData[i + j]);
            } else {
                pos += sprintf(line_buf + pos, "  ");
            }
            if (j < 7) {
                pos += sprintf(line_buf + pos, " ");
            }
        }
        
        pos += sprintf(line_buf + pos, " | ");

        /* ASCII Data */
        for (int32_t j = 0; j < 8 && (i + j) < Size; j++) {
            Byte_t byte = pData[i + j];
            if (byte >= 0x20 && byte <= 0x7E) {
                pos += sprintf(line_buf + pos, "%c", byte);
            }
            else {
                pos += sprintf(line_buf + pos, ".");
            }
            if (j < 7 && (i + j + 1) < Size) {
                pos += sprintf(line_buf + pos, " ");
            }
        }
        SysLog("%s", line_buf);
    }
    SysExit("W5500_LogFrame");
}

/// @brief Log network information extracted from the received packet
/// @param pkt Pointer to the packet slot in RxPool
static void W5500_LogUDPInfo(PacketSlot_t* pkt) {
    if (pkt == NULL) return;
    
    SysLog("W5500_LogUDPInfo(...) : Network Info Extracted");
    SysLog("    |- Src IP   : %u.%u.%u.%u", 
            pkt->SrcIP.Byte[0], pkt->SrcIP.Byte[1], pkt->SrcIP.Byte[2], pkt->SrcIP.Byte[3]);
    SysLog("    |- Src Port : %u", pkt->SrcPort.Word);
    SysLog("    |- Src MAC  : %02X:%02X:%02X:%02X:%02X:%02X", 
            pkt->SrcMAC.Byte[0], pkt->SrcMAC.Byte[1], pkt->SrcMAC.Byte[2], 
            pkt->SrcMAC.Byte[3], pkt->SrcMAC.Byte[4], pkt->SrcMAC.Byte[5]);
    SysLog("    |- Payload  : %u bytes", pkt->Size);
}

/// @brief Send a packet over UDP (Socket 1) utilizing extracted IP/Port
/// @param tx_pkt Pointer to the packet slot in TxPool
static void W5500_TaskComm_SocketNSendUDP(PacketSlot_t* tx_pkt) {
    Word_t bsb_reg = eBSB_Socket1Register;
    Word_t bsb_tx_buf = eBSB_Socket1TxBuffer;

    SysLog("W5500_TaskComm_SocketNSendUDP(...): UDP Transmit %d bytes to %u.%u.%u.%u:%u", 
        tx_pkt->Size, tx_pkt->SrcIP.Byte[0], tx_pkt->SrcIP.Byte[1], tx_pkt->SrcIP.Byte[2], tx_pkt->SrcIP.Byte[3], tx_pkt->SrcPort.Word);

    /* Configure Destination IP and Port Registers */
    W5500_WriteNByteReg(Eth, bsb_reg, eSn_DIPR0, tx_pkt->SrcIP.Byte, 4);
    W5500_WriteDoubleByteReg(Eth, bsb_reg, eSn_DPORT0, tx_pkt->SrcPort.Word);

    /* Retrieve TX Write Pointer, Copy Payload, then update Pointer */
    Word_t tx_wr = (Word_t)W5500_ReadDoubleByteReg(Eth, bsb_reg, eSn_TX_WR0);
    W5500_WriteNByteReg(Eth, bsb_tx_buf, tx_wr, tx_pkt->Data, tx_pkt->Size);
    W5500_WriteDoubleByteReg(Eth, bsb_reg, eSn_TX_WR0, tx_wr + tx_pkt->Size);

    /* Issue SEND command (0x20) */
    W5500_WriteByteReg(Eth, bsb_reg, eSn_CR, 0x20);
}

/// @brief Receive packet from a specific W5500 socket
/// @param n The socket number (0-7) to process
static void W5500_TaskComm_SocketNReceive(Byte_t n) {
    Word_t bsb_reg = eBSB_Socket0Register + (n * 4);
    Word_t bsb_rx_buf = eBSB_Socket0RxBuffer + (n * 4);
    Byte_t Sn_IR;
    Word_t rx_rd, packet_size;

    /* Read Socket n Interrupt Register to check event status */
    Sn_IR = (Byte_t)W5500_ReadByteReg(Eth, bsb_reg, eSn_IR);
    SysLog("W5500_TaskComm_SocketNReceive(...): S%u_IR = %02X", n, Sn_IR);
    
    /* Process incoming data if RECV interrupt (bit 2) is asserted */
    if (Sn_IR & 0x04) {
        /* Read current RX Read Pointer */
        rx_rd = (Word_t)W5500_ReadDoubleByteReg(Eth, bsb_reg, eSn_RX_RD0);

        #if (SOCKET_0_MACRAW_EN == 1)
        if (n == 0) { /* MACRAW on Socket 0 */
            /* Fetch 2-byte packet size header from the RX buffer */
            packet_size = (Word_t)W5500_ReadDoubleByteReg(Eth, bsb_rx_buf, rx_rd);
            
            Word_t frame_len = packet_size - 2;

            if (frame_len > 0) {
                Word_t read_len = (frame_len > 1500) ? 1500 : frame_len;
                /* Read payload, skipping the 2-byte size header */
                W5500_ReadNByteReg(Eth, bsb_rx_buf, rx_rd + 2, Eth->RxFrame.Payload.Byte, read_len);
                SysLog("W5500_TaskComm_SocketNReceive(...): Saved %d bytes of MACRAW frame from S%u", read_len, n);
                
                uint8_t src_mac[6] = {0};
                uint8_t src_ip[4] = {0};
                uint16_t src_port = 0;
                memcpy(src_mac, &Eth->RxFrame.Payload.Byte[6], 6);
                if (Eth->RxFrame.Payload.Byte[12] == 0x08 && Eth->RxFrame.Payload.Byte[13] == 0x00) { /* Is IPv4 */
                    memcpy(src_ip, &Eth->RxFrame.Payload.Byte[26], 4);
                    if (Eth->RxFrame.Payload.Byte[23] == 17) { /* Is UDP */
                        src_port = (Eth->RxFrame.Payload.Byte[34] << 8) | Eth->RxFrame.Payload.Byte[35];
                    }
                }
                RxedPacket_Push(read_len, (GenericPtr_t) Eth->RxFrame.Payload.Byte, src_ip, src_port, src_mac);
            }
            
            rx_rd += packet_size;
        }
        #endif /* SOCKET_0_MACRAW_EN */

        #if (SOCKET_1_UDP_EN == 1)
        if (n == 1) { /* UDP on Socket 1 */
            Byte_t udp_header[8];
            Word_t data_len;

            /* W5500 UDP RX Buffer: [Dest IP(4)] [Dest Port(2)] [Packet Size(2)] [Data(...)] */
            W5500_ReadNByteReg(Eth, bsb_rx_buf, rx_rd, udp_header, 8);
            
            /* Extract data length from the header (offset 6, Big-Endian) */
            data_len = (udp_header[6] << 8) | udp_header[7];
            
            /* Extract info from W5500 UDP header */
            uint8_t src_ip[4] = {udp_header[0], udp_header[1], udp_header[2], udp_header[3]};
            uint16_t src_port = (udp_header[4] << 8) | udp_header[5];
            uint8_t src_mac[6] = {0}; /* Cannot extract MAC natively from W5500 UDP Socket */

            if (data_len > 0) {
                Word_t read_len = (data_len > 1500) ? 1500 : data_len;
                /* Read the actual UDP payload which starts after the 8-byte header */
                W5500_ReadNByteReg(Eth, bsb_rx_buf, rx_rd + 8, Eth->RxFrame.Payload.Byte, read_len);
                SysLog("W5500_TaskComm_SocketNReceive(...): Saved %d bytes of UDP payload from S%u", read_len, n);
                RxedPacket_Push(read_len, (GenericPtr_t) Eth->RxFrame.Payload.Byte, src_ip, src_port, src_mac);
            }
            packet_size = 8 + data_len;
            rx_rd += packet_size;
        }
        #endif /* SOCKET_1_UDP_EN */

        /* Update RX Read Pointer Register with new address */
        W5500_WriteDoubleByteReg(Eth, bsb_reg, eSn_RX_RD0, rx_rd);
        /* Issue RECV command (0x40) to notify W5500 of RX buffer update */
        W5500_WriteByteReg(Eth, bsb_reg, eSn_CR, 0x40);
    }
    
    /* Log if SEND_OK interrupt (bit 4) is asserted */
    if (Sn_IR & 0x10) { 
        SysLog("W5500_TaskComm_SocketNReceive(...): S%u Data Sent", n); 
    }
    
    /* Clear all Socket n interrupts by writing 1s to Sn_IR */
    W5500_WriteByteReg(Eth, bsb_reg, eSn_IR, 0xFF);
}

/// @brief This task will be woken up when an isr is occurred!
/// @param arg Pointer to the EthernetW5500_t structure
void W5500_TaskComm_GetPacket(void* arg) {
    Byte_t Attempt;
    Byte_t const MaxAttempt = 3;
    Byte_t SIR, IR, n;

    while (1) {
        /* Wait until an interrupt occurs OR woken up to Transmit */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        
        /* Task wake up */
        SysLog("W5500_TaskComm_GetPacket(...): Task is woken up!");
        
        /* --- Process RX Queue (Fetch Data from W5500) --- */
        Attempt = 0;
        do {
            SIR = (Byte_t)W5500_ReadByteReg(Eth, eBSB_CommonRegister, eSIR);
            IR  = (Byte_t)W5500_ReadByteReg(Eth, eBSB_CommonRegister, eIR);
            
            if (SIR) {
                for (n = 0; n < 8; ++n) {
                    if ((1U << n) & SIR) {
                        /* Socket N has an interrupt event, process it */
                        W5500_TaskComm_SocketNReceive(n);
                    }
                }
            }
            
            /* Clear Global Interrupts */
            if (IR) { W5500_WriteByteReg(Eth, eBSB_CommonRegister, eIR, IR); }
            
        } while (SIR || (Attempt++ < MaxAttempt));
        
        
        /* --- Process TX Queue (Push Data to W5500) --- */
        PacketSlot_t* tx_pkt;
        while ((tx_pkt = TxPacket_GetHighestPriority()) != NULL) {
            #if (SOCKET_1_UDP_EN == 1)
            W5500_TaskComm_SocketNSendUDP(tx_pkt);
            #endif
            TxPacket_Release(tx_pkt);
        }
        
        /* Notify W5500CommCtl if there are newly received packets */
        if(RxedPacket_Size() > 0) {
            xTaskNotifyGive(W5500CommCtl_TaskHandler);
        }
    }
}

/* MODULE INIT *************************************************************************************************************/

/// @brief Initialize W5500 network interface for MACRAW and ICMP
/// @param Eth Pointer to the Ethernet W5500 controller structure
/// @return STAT_OKE upon successful initialization
ReturnCode_t W5500CommCtl_Init(EthernetW5500_t * Eth) { 
    uint64_t Data;
    uint8_t dummy_reg;
    
    SysEntry("W5500CommCtl_Init");

    EthLock = xSemaphoreCreateBinary();
    xSemaphoreGive(EthLock);

    /* Reset device and set mode register */
    W5500_WriteByteReg(Eth, eBSB_CommonRegister, eMR, 0x80);

    Data = W5500_GetModuleVersion(Eth);
    
    /* Ensure W5500 module is version 4 before proceeding with setup */
    if ((Data & 0xFF) != 0x04) {
        SysErr("W5500CommCtl_Init(...): W5500 Version (=%X) is not supported!", Data & 0xFF);
        /* Return error code to prevent initialization with unsupported hardware */
        return STAT_ERR_UNSUPPORTED;
    }
    SysLog("W5500CommCtl_Init(...): W5500 Version = %X", Data & 0xFF);

    /* Configure Gateway Address */
    W5500_WriteQuartByteReg(Eth, eBSB_CommonRegister, eGAR0, ConvertByteArrayToInt32_BigEndian(GW_IP_ADDR, 4));

    /* Configure Subnet Mask */
    W5500_WriteQuartByteReg(Eth, eBSB_CommonRegister, eSUBR0, ConvertByteArrayToInt32_BigEndian(SUBNET_MASK, 4));

    /* Configure Source IP Address */
    W5500_WriteQuartByteReg(Eth, eBSB_CommonRegister, eSIPR0, ConvertByteArrayToInt32_BigEndian(SRC_IP_ADDR, 4));
    
    /* Configure Hardware MAC Address */
    Data = ConvertByteArrayToInt64_BigEndian(SRC_MAC_ADDR, 6);
    W5500_WriteNByteReg(Eth, eBSB_CommonRegister, eSHAR0, (void*) &Data, 6);

    /* Enable global interrupts for active sockets in SIMR */
    Byte_t simr_val = 0;
    #if (SOCKET_0_MACRAW_EN == 1)
        simr_val |= (1 << 0);
    #endif
    #if (SOCKET_1_UDP_EN == 1)
        simr_val |= (1 << 1);
    #endif
    W5500_WriteByteReg(Eth, eBSB_CommonRegister, eSIMR, simr_val);

    /* Configure Interrupt Assert Wait Time (INTLEVEL) */
    W5500_WriteDoubleByteReg(Eth, eBSB_CommonRegister, eINTLEVEL0, 0x03E8);

    /* Configure Global Interrupt Mask (IMR): IP Conflict (0x80) & Dest Unreachable (0x10) */
    W5500_WriteByteReg(Eth, eBSB_CommonRegister, eIMR, 0x90);

    /* Clear existing interrupts in IR */
    dummy_reg = W5500_ReadByteReg(Eth, eBSB_CommonRegister, eIR); 
    W5500_WriteByteReg(Eth, eBSB_CommonRegister, eIR, dummy_reg);

    /* Clear existing interrupts in SIR */
    dummy_reg = W5500_ReadByteReg(Eth, eBSB_CommonRegister, eSIR);
    W5500_WriteByteReg(Eth, eBSB_CommonRegister, eSIR, dummy_reg);

    /* Configure Socket RX/TX buffer sizes */
    #if (SOCKET_0_MACRAW_EN == 1)
    W5500_WriteByteReg(Eth, eBSB_Socket0Register, eSn_RXBUF_SIZE, 8); /* 8KB */
    W5500_WriteByteReg(Eth, eBSB_Socket0Register, eSn_TXBUF_SIZE, 0); /* Not used */
    #endif
    #if (SOCKET_1_UDP_EN == 1)
    W5500_WriteByteReg(Eth, eBSB_Socket1Register, eSn_RXBUF_SIZE, 8); /* 8KB */
    W5500_WriteByteReg(Eth, eBSB_Socket1Register, eSn_TXBUF_SIZE, 8); /* 8KB */
    #endif
    /* S2-S7 are not used, set their buffers to 0KB */
    for (int32_t i = 2; i < 8; i++) {
        W5500_WriteByteReg(Eth, eBSB_Socket0Register + (i * 4), eSn_RXBUF_SIZE, 0);
        W5500_WriteByteReg(Eth, eBSB_Socket0Register + (i * 4), eSn_TXBUF_SIZE, 0);
    }

    #if (SOCKET_0_MACRAW_EN == 1)
    SysLog("W5500CommCtl_Init(...): Configure Socket 0 for MACRAW");
    W5500_WriteByteReg(Eth, eBSB_Socket0Register, eSn_IMR, 0x14); /* Enable SEND_OK, RECV interrupts */
    W5500_WriteByteReg(Eth, eBSB_Socket0Register, eSn_MR, 0x04);  /* Set MACRAW mode */
    W5500_WriteByteReg(Eth, eBSB_Socket0Register, eSn_CR, 0x01);
    SysLog("W5500CommCtl_Init(...): Verify eSn_SR=%02X (Expected: 0x42)", W5500_ReadByteReg(Eth, eBSB_Socket0Register, eSn_SR));
    #endif

    #if (SOCKET_1_UDP_EN == 1)
    SysLog("W5500CommCtl_Init(...): Configure Socket 1 for UDP on port %d", SOCKET_1_UDP_PORT);
    W5500_WriteByteReg(Eth, eBSB_Socket1Register, eSn_IMR, 0x04); /* Enable RECV interrupt */
    W5500_WriteByteReg(Eth, eBSB_Socket1Register, eSn_MR, 0x02);  /* Set UDP mode */
    W5500_WriteDoubleByteReg(Eth, eBSB_Socket1Register, eSn_PORT0, SOCKET_1_UDP_PORT); /* Set port */
    W5500_WriteByteReg(Eth, eBSB_Socket1Register, eSn_CR, 0x01); /* Issue OPEN command */
    SysLog("W5500CommCtl_Init(...): Verify eSn_SR=%02X (Expected: 0x22)", W5500_ReadByteReg(Eth, eBSB_Socket1Register, eSn_SR));
    #endif

    SysExit("W5500CommCtl_Init");
    
    /* Conclude configuration successfully and return OK status */
    return STAT_OKE;
}

/* MODULE PUBLIC APIs ******************************************************************************************************/

ReturnCode_t IPv4SetGateWayAddress(uint32_t IPv4Addr) {
    if (Eth == NULL) return STAT_ERR_INVALID_STATE;
    return W5500_WriteQuartByteReg(Eth, eBSB_CommonRegister, eGAR0, IPv4Addr);
}

ReturnCode_t Eth_SetIPv4SubnetMask(uint32_t IPv4SubnetMask) {
    if (Eth == NULL) return STAT_ERR_INVALID_STATE;
    return W5500_WriteQuartByteReg(Eth, eBSB_CommonRegister, eSUBR0, IPv4SubnetMask);
}

ReturnCode_t Eth_SetSrcIPv4Address(uint32_t IPv4Addr) {
    if (Eth == NULL) return STAT_ERR_INVALID_STATE;
    return W5500_WriteQuartByteReg(Eth, eBSB_CommonRegister, eSIPR0, IPv4Addr);
}

ReturnCode_t EthSetIPv4SourceAddress(uint32_t IPv4Addr) {
    return Eth_SetSrcIPv4Address(IPv4Addr);
}

ReturnCode_t Eth_SetSrcIPv4Port(uint16_t IPv4Port) {
    if (Eth == NULL) return STAT_ERR_INVALID_STATE;
    return W5500_WriteDoubleByteReg(Eth, eBSB_Socket1Register, eSn_PORT0, IPv4Port);
}

ReturnCode_t Eth_SetUDPBufferSize(uint8_t TxSizeKB, uint8_t RxSizeKB) {
    if (Eth == NULL) return STAT_ERR_INVALID_STATE;
    W5500_WriteByteReg(Eth, eBSB_Socket1Register, eSn_TXBUF_SIZE, TxSizeKB);
    W5500_WriteByteReg(Eth, eBSB_Socket1Register, eSn_RXBUF_SIZE, RxSizeKB);
    return STAT_OKE;
}

ReturnCode_t Eth_GetRxPacket(PacketSlot_t** pkt) {
    if (pkt == NULL) return STAT_ERR_NULL;
    *pkt = RxedPacket_GetHighestPriority();
    return (*pkt != NULL) ? STAT_OKE : STAT_ERR_NOT_FOUND;
}

ReturnCode_t Eth_GetTxPacket(PacketSlot_t** pkt) {
    if (pkt == NULL) return STAT_ERR_NULL;
    *pkt = TxPacket_GetHighestPriority();
    return (*pkt != NULL) ? STAT_OKE : STAT_ERR_NOT_FOUND;
}

ReturnCode_t Eth_GetUDPPacket(PacketSlot_t** pkt) {
    return Eth_GetRxPacket(pkt);
}

ReturnCode_t Eth_GetUDPInfo(PacketSlot_t* pkt, uint8_t* srcIP, uint16_t* srcPort, uint8_t* srcMAC) {
    if (pkt == NULL) return STAT_ERR_NULL;
    if (srcIP) memcpy(srcIP, pkt->SrcIP.Byte, 4);
    if (srcPort) *srcPort = pkt->SrcPort.Word;
    if (srcMAC) memcpy(srcMAC, pkt->SrcMAC.Byte, 6);
    return STAT_OKE;
}

ReturnCode_t Eth_GetUDPPayload(PacketSlot_t* pkt, uint8_t** payload, uint16_t* size) {
    if (pkt == NULL || payload == NULL || size == NULL) return STAT_ERR_NULL;
    *payload = pkt->Data;
    *size = pkt->Size;
    return STAT_OKE;
}

ReturnCode_t Eth_SetRxCallback(EthRxCallback_t cb) {
    g_EthRxCallback = cb;
    return STAT_OKE;
}

ReturnCode_t Eth_SendUDPPacket(uint32_t IPv4Address, uint16_t IPv4Port, uint8_t* UDP_Payload, uint16_t PayloadSize) {
    uint8_t ip_bytes[4];
    ip_bytes[0] = (IPv4Address >> 24) & 0xFF;
    ip_bytes[1] = (IPv4Address >> 16) & 0xFF;
    ip_bytes[2] = (IPv4Address >>  8) & 0xFF;
    ip_bytes[3] = (IPv4Address      ) & 0xFF;

    ReturnCode_t ret = TxPacket_Push(PayloadSize, (GenericPtr_t)UDP_Payload, ip_bytes, IPv4Port, NULL);
    if (ret == STAT_OKE) {
        if (W5500_TaskComm_TaskHandler != NULL) {
            xTaskNotifyGive(W5500_TaskComm_TaskHandler);
        }
    }
    return ret;
}

__attribute__((weak)) void Eth_WeakRxCallback(PacketSlot_t* pkt) {
    if (pkt == NULL) return;
    SysLog("Eth_WeakRxCallback(...):  Process packet, Size=%d, Prio ID=0x%02X", pkt->Size, pkt->Data[0]);
    
    W5500_LogUDPInfo(pkt);
    W5500_LogFrame((GenericPtr_t)pkt->Data, pkt->Size, GenericNullPtr, GenericNullPtr);
    
    /* --- TEST ECHO --- 
     * Push the exact same packet into the Transmit Queue and trigger the worker */
    TxPacket_Push(pkt->Size, (GenericPtr_t)pkt->Data, pkt->SrcIP.Byte, pkt->SrcPort.Word, pkt->SrcMAC.Byte);
    if (W5500_TaskComm_TaskHandler != NULL) {
        xTaskNotifyGive(W5500_TaskComm_TaskHandler);
    }
}

/* INTERNAL TASK **********************************************************************************************************/

/// @brief  Serivce handle W5500 module
/// @param arg Ignored
void W5500CommRuntime(void* arg){
    SysEntry("W5500CommCtl");
    
    ReturnCode_t RetVal;
    
    RxedPacket_Init();
    
    SysLog("W5500CommCtl(...) : New motor object...");
    Eth = W5500_Create(PIN_W5500_MISO, PIN_W5500_MOSI, PIN_W5500_CLK, PIN_W5500_SCS, PIN_W5500_RST, PIN_W5500_INT);
    
    if (IsNull(Eth)) {
        SysErr("W5500CommCtl(...) : Cannot initialize `Eth`!");
        goto CLEANUP_AND_EXIT;
    }
    
    RetVal = W5500CommCtl_Init(Eth);
    if(RetVal != STAT_OKE){
        SysErr("W5500CommCtl(...): Cannot initialize module W5500! ErrorCode=%d", RetVal);
        goto CLEANUP_AND_EXIT;
    }
    /*Create subtask to perform copy rxed packet, handle by IsrHandler*/
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
    /*Runtime service to handle internal communication*/
    SysLog("W5500CommCtl(...) : Join forever loop...");
    while (1) {
        SysLog("W5500CommCtl(...) : W5500 version=%X", W5500_GetModuleVersion(Eth));

        SysLog("W5500CommCtl(...):  Check RxedPacket and log...");
        SysLog("W5500CommCtl(...):  #Rexed packets=%d", RxedPacket_Size());
        
        PacketSlot_t* pkt;
        /* Lấy liên tục các gói có độ ưu tiên cao nhất cho tới khi rỗng */
        while((pkt = RxedPacket_GetHighestPriority()) != NULL){
            if (g_EthRxCallback != NULL) {
                g_EthRxCallback(pkt);
            } else {
                Eth_WeakRxCallback(pkt);
            }
            
            /* Đọc xong phải trả Slot lại cho Pool */
            RxedPacket_Release(pkt);
            SysLog("");
        }

        SysLog("");

        /*Sleep and wake-up for every 5000ms/a triggered event*/
        SysLog("W5500CommCtl(...) : Task will be put to sleep!");
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(WAEKUP_CYCLE_TIME));
        
        SysLog("");
        SysLog("W5500CommCtl(...) : Task woken up!");
    }

    /*For exit and delete task when an unexpected error occurred*/
    CLEANUP_AND_EXIT:
        vSemaphoreDelete(EthLock);
        EthLock = NULL;
        vTaskDelete(NULL);
}

/* EOF *******************************************************************************************************************/
