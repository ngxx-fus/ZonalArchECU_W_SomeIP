#include "Module.h"

EthernetW5500_t *    Eth;
SemaphoreHandle_t    EthLock;

/// @brief Handle incoming interrupts from W5500 for MACRAW and ICMP with semaphore protection
/// @param arg Pointer to the EthernetW5500_t structure
/// @return void
static void IRAM_ATTR W5500_OnPacketHandler_Legacy(void* arg) {
    /* Attempt to acquire the hardware lock; Note: Use Binary Semaphore for ISR compatibility */
    // if (xSemaphoreTakeFromISR(EthLock, NULL) != pdTRUE) {
        //     return;
        // }
        
    int Attempt;
    uint16_t TempRegVal;
    EthernetW5500_t* Ptr = (EthernetW5500_t*)(arg);
    
    SysEntry("W5500_OnPacketHandler(...)");

    
    Attempt = 0;
    while ((Attempt++ < 1) && W5500_IsInInterruptPhase(Ptr) == STAT_OKE) {
        
        W5500_SetHeader(Ptr, eBSB_CommonRegister, 0x0015); uint8_t ir = W5500_ReadByte(Ptr);
        
        /* Read Socket 0 Interrupt Register */
        W5500_SetHeader(Ptr, eBSB_Socket0Register, 0x0002); uint8_t s0_ir = W5500_ReadByte(Ptr);
        
        SysLog("W5500_OnPacketHandler(...): Attempt=%d IR=%X S0_IR=%X", Attempt, ir, s0_ir);

        /* Process MACRAW Receive Interrupt on Socket 0 */
        if (s0_ir & 0x04) {
            uint16_t rx_rd;
            uint16_t packet_size;
            uint8_t size_header[2];
            
            /* Locate and read 2-byte size header containing the total frame length */
            W5500_SetHeader(Ptr, eBSB_Socket0Register, 0x0028); rx_rd = W5500_ReadDoubleByte(Ptr);
            W5500_SetHeader(Ptr, eBSB_Socket0RxBuffer, rx_rd); W5500_ReadNByte(Ptr, size_header, 2);
            packet_size = (size_header[0] << 8) | size_header[1];
            
            /* Calculate actual frame length and extract payload into internal buffer */
            uint16_t frame_len = packet_size - 2;
            if (frame_len > 0) {
                uint16_t read_len = (frame_len > 1514) ? 1514 : frame_len;
                W5500_SetHeader(Ptr, eBSB_Socket0RxBuffer, rx_rd + 2); W5500_ReadNByte(Ptr, Ptr->RxFrame.Payload.Byte, read_len);
                
                SysLog("MACRAW Received! Size: %u bytes", frame_len);
                SysLog("Dest MAC: %02X:%02X:%02X:%02X:%02X:%02X", Ptr->RxFrame.Payload.Byte[0], Ptr->RxFrame.Payload.Byte[1], Ptr->RxFrame.Payload.Byte[2], Ptr->RxFrame.Payload.Byte[3], Ptr->RxFrame.Payload.Byte[4], Ptr->RxFrame.Payload.Byte[5]);
                SysLog("Src MAC: %02X:%02X:%02X:%02X:%02X:%02X", Ptr->RxFrame.Payload.Byte[6], Ptr->RxFrame.Payload.Byte[7], Ptr->RxFrame.Payload.Byte[8], Ptr->RxFrame.Payload.Byte[9], Ptr->RxFrame.Payload.Byte[10], Ptr->RxFrame.Payload.Byte[11]);
                
                /* Iterate through raw payload and log characters or hex values for debugging */
                for (uint16_t i = 0; i < read_len; i += 16) {
                    char line_buf[128];
                    int pos = 0;
                    /* Add offset address at the beginning of the line */
                    pos += sprintf(line_buf + pos, "%04X: ", i);
                    
                    for (int j = 0; j < 16 && (i + j) < read_len; j++) {
                        uint8_t byte = Ptr->RxFrame.Payload.Byte[i + j];
                        /* Filter printable ASCII characters vs non-printable hex */
                        if (byte >= 0x20 && byte <= 0x7E) {
                            pos += sprintf(line_buf + pos, " %c ", byte);
                        }
                        else {
                            pos += sprintf(line_buf + pos, "%02X ", byte);
                        }
                    }
                    SysLog("%s", line_buf);
                }
            }
            
            /* Update the RX Read Pointer and notify W5500 with RECV command */
            rx_rd += packet_size;
            W5500_SetHeader(Ptr, eBSB_Socket0Register, 0x0028); W5500_WriteDoubleByte(Ptr, rx_rd);
            W5500_SetHeader(Ptr, eBSB_Socket0Register, 0x0001); W5500_WriteByte(Ptr, 0x40);
        }

        /* Process MACRAW Send OK Interrupt */
        if (s0_ir & 0x10) {
            SysLog("W5500_OnPacketHandler(...): MACRAW Data Sent");
        }

        /* Clear Socket 0 interrupts by writing '1' to set bits */
        if (0xFF | s0_ir) {
            W5500_SetHeader(Ptr, eBSB_Socket0Register, 0x0002); W5500_WriteByte(Ptr, 0xFF);
        }

        // /* Read and process Socket 7 Interrupt Register */
        // W5500_SetHeader(Ptr, eW5500_SelectsSocket7Register, 0x0002); uint8_t s7_ir = W5500_ReadByte(Ptr);

        
        // /* Clear Socket 7 interrupts */
        // if (0xFF | s7_ir) {
        //     SysLog("W5500_OnPacketHandler(...): ICMP Interrupt on S7");
        //     W5500_SetHeader(Ptr, eW5500_SelectsSocket7Register, 0x0002); W5500_WriteByte(Ptr, 0xFF);
        // }

        /* Clear Global Interrupt status bit by writing '1' */
        if (ir) {
            W5500_SetHeader(Ptr, eBSB_CommonRegister, 0x0015); W5500_WriteByte(Ptr, ir);
        }
    }

    // /* Release the hardware lock */
    // xSemaphoreGiveFromISR(EthLock, NULL);
}

/// @brief Initialize W5500 network interface for MACRAW and ICMP
/// @param Eth Pointer to the Ethernet W5500 controller structure
/// @return STAT_OKE upon successful initialization
ReturnCode_t EthernetAdapterCtl_Init(EthernetW5500_t * Eth) {
    /* Initialize W5500 common registers and network parameters */
    SysEntry("EthernetAdapterCtl_Init");
    uint64_t Data;

    /* Reset device and set mode register */
    W5500_SetHeader(Eth, eBSB_CommonRegister, eMR); W5500_WriteByte(Eth, 0x80);

    /* Configure Gateway Address */
    W5500_SetHeader(Eth, eBSB_CommonRegister, eGAR0); W5500_WriteQuartByte(Eth, ConvertByteArrayToInt32_BigEndian(GW_IP_ADDR, 4));

    /* Configure Subnet Mask */
    W5500_SetHeader(Eth, eBSB_CommonRegister, eSUBR0); W5500_WriteQuartByte(Eth, ConvertByteArrayToInt32_BigEndian(SUBNET_MASK, 4));

    /* Configure Source IP Address */
    W5500_SetHeader(Eth, eBSB_CommonRegister, eSIPR0); W5500_WriteQuartByte(Eth, ConvertByteArrayToInt32_BigEndian(SRC_IP_ADDR, 4));
    
    /* Configure Hardware MAC Address (Hardware auto-replies to ARP using this) */
    W5500_SetHeader(Eth, eBSB_CommonRegister, eSHAR0); Data = ConvertByteArrayToInt64_BigEndian(SRC_MAC_ADDR, 6); W5500_WriteNByte(Eth, (void*) &Data, 6);

    /* Enable Socket 0 and Socket 7 global interrupt in SIMR */
    W5500_SetHeader(Eth, eBSB_CommonRegister, eIMR); W5500_WriteByte(Eth, 0x81);

    /* Configure Interrupt Assert Wait Time (INTLEVEL) to prevent missed edge interrupts */
    W5500_SetHeader(Eth, eBSB_CommonRegister, eINTLEVEL0); W5500_WriteDoubleByte(Eth, 0x03E8);

    /* Configure Global Interrupt Mask (IMR) */
    /* Enable IP Conflict (0x80) and Destination Unreachable (0x10) interrupts */
    W5500_SetHeader(Eth, eBSB_CommonRegister, 0x0016);  W5500_WriteByte(Eth, 0x90); 

    /* Clear any existing ghost interrupts in IR and SIR (Write 1 to clear) */
    W5500_SetHeader(Eth, eBSB_CommonRegister, 0x0015); 
    uint8_t dummy_ir = W5500_ReadByte(Eth); 
    W5500_WriteByte(Eth, dummy_ir);

    W5500_SetHeader(Eth, eBSB_CommonRegister, 0x0017); 
    uint8_t dummy_sir = W5500_ReadByte(Eth);
    W5500_WriteByte(Eth, dummy_sir);

    /* Configure Socket 0 for MACRAW */
    SysLog("EthernetAdapterCtl(...): Configure Socket 0 for MACRAW");
    W5500_SetHeader(Eth, eBSB_Socket0Register, 0x002C); W5500_WriteByte(Eth, 0x14); /* Sn_IMR: RECV & SEND_OK */
    W5500_SetHeader(Eth, eBSB_Socket0Register, 0x0000); W5500_WriteByte(Eth, 0x04); /* Sn_MR: MACRAW */
    W5500_SetHeader(Eth, eBSB_Socket0Register, 0x0001); W5500_WriteByte(Eth, 0x01); /* Sn_CR: OPEN */

    W5500_WriteByteReg(Eth, eBSB_CommonRegister, eSIMR, 0x01);
    W5500_WriteByteReg(Eth, eBSB_Socket0Register, eSn_RXBUF_SIZE, 0x08);

    /* Configure Socket 7 for Ping (ICMP) */
    // SysLog("EthernetAdapterCtl(...): Configure Socket 7 for ICMP");
    // W5500_SetHeader(Eth, eW5500_SelectsSocket7Register, 0x0014); W5500_WriteByte(Eth, 0x01); /* Sn_PROTO: ICMP */
    // W5500_SetHeader(Eth, eW5500_SelectsSocket7Register, 0x0000); W5500_WriteByte(Eth, 0x03); /* Sn_MR: IPRAW */
    // W5500_SetHeader(Eth, eW5500_SelectsSocket7Register, 0x0001); W5500_WriteByte(Eth, 0x01); /* Sn_CR: OPEN */

    SysExit("EthernetAdapterCtl_Init");
    return STAT_OKE;
}


/* INTERNAL TASK **********************************************************************************************************/

/// @brief Log the received MACRAW frame details and hex dump (8 bytes per row)
/// @param Ptr Pointer to the W5500 controller structure
/// @param Len Length of the payload to print
static void W5500_LogFrame(EthernetW5500_t* Ptr, Word_t Len) {
    
    /* Log basic frame info and MAC addresses */
    SysLog("MACRAW Received! Size: %u bytes", Len);
    SysLog("Dest MAC: %02X:%02X:%02X:%02X:%02X:%02X", Ptr->RxFrame.Payload.Byte[0], Ptr->RxFrame.Payload.Byte[1], Ptr->RxFrame.Payload.Byte[2], Ptr->RxFrame.Payload.Byte[3], Ptr->RxFrame.Payload.Byte[4], Ptr->RxFrame.Payload.Byte[5]);
    SysLog("Src MAC: %02X:%02X:%02X:%02X:%02X:%02X", Ptr->RxFrame.Payload.Byte[6], Ptr->RxFrame.Payload.Byte[7], Ptr->RxFrame.Payload.Byte[8], Ptr->RxFrame.Payload.Byte[9], Ptr->RxFrame.Payload.Byte[10], Ptr->RxFrame.Payload.Byte[11]);

    
    /* Process payload in 8-byte segments */
    for (Word_t i = 0; i < Len; i += 8) {
        char line_buf[128];
        int pos = 0;
        pos += sprintf(line_buf + pos, "%04X: ", i);
        
        for (int j = 0; j < 8 && (i + j) < Len; j++) {
            Byte_t byte = Ptr->RxFrame.Payload.Byte[i + j];
            /* Filter printable ASCII or format as hex */
            if (byte >= 0x20 && byte <= 0x7E) {
                pos += sprintf(line_buf + pos, " %c ", byte);
            }
            else {
                pos += sprintf(line_buf + pos, "%02X ", byte);
            }
        }
        SysLog("%s", line_buf);
    }
}

/// @brief This task will be woken up when an isr is occurred!
/// @param arg Pointer to the EthernetW5500_t structure
void W5500_TaskIsrTrack(void* arg) {
    Byte_t Attempt;
    Byte_t const MaxAttempt = 3;
    Byte_t SIR, IR, Sn_IR, n;

    while (1) {
        /* Reset local variables before sleep */
        Attempt = 0;
        SysLog("W5500_IsrTrack(...): Task will be put to sleep!\n");
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        
        /* Task wake up */
        SysLog("W5500_IsrTrack(...): Task is woken up!");
        
        
        /* Loop until no more interrupts or max attempts reached */
        do {
            SIR = (Byte_t)W5500_ReadByteReg(Eth, eBSB_CommonRegister, eSIR);
            IR  = (Byte_t)W5500_ReadByteReg(Eth, eBSB_CommonRegister, eIR);
            SysLog("W5500_IsrTrack(...): SIR=%02X, IR=%02X", SIR, IR);
            
            if (SIR) {
                for (n = 0; n < 8; ++n) {
                    if ((1U << n) & SIR) {
                        /* Socket N has interrupt event */
                        switch (n) {
                            case 0: {
                                Word_t rx_rd, packet_size;
                                Sn_IR = (Byte_t)W5500_ReadByteReg(Eth, eBSB_Socket0Register, eSn_IR);
                                SysLog("W5500_IsrTrack(...): S0_IR=%02X", Sn_IR);
                                
                                
                                /* Handle Received Data */
                                if (Sn_IR & 0x04) {
                                    rx_rd       = (Word_t)W5500_ReadDoubleByteReg(Eth, eBSB_Socket0Register, eSn_RX_RD0);
                                    packet_size = (Word_t)W5500_ReadDoubleByteReg(Eth, eBSB_Socket0RxBuffer, rx_rd);
                                    
                                    
                                    /* Calculate frame length (excluding 2-byte header) and read data */
                                    Word_t frame_len = packet_size - 2;
                                    if (frame_len > 0) {
                                        Word_t read_len = (frame_len > 1514) ? 1514 : frame_len;
                                        W5500_ReadNByteReg(Eth, eBSB_Socket0RxBuffer, rx_rd + 2, Eth->RxFrame.Payload.Byte, read_len);
                                        W5500_LogFrame(Eth, read_len);
                                    }
                                    
                                    
                                    /* Advance read pointer and confirm with RECV command */
                                    rx_rd += packet_size;
                                    W5500_WriteDoubleByteReg(Eth, eBSB_Socket0Register, eSn_RX_RD0, rx_rd);
                                    W5500_WriteByteReg(Eth, eBSB_Socket0Register, eSn_CR, 0x40);
                                }
                                
                                if (Sn_IR & 0x10) { SysLog("W5500_IsrTrack(...): MACRAW Data Sent"); }
                                
                                /* Clear Socket 0 interrupts */
                                W5500_WriteByteReg(Eth, eBSB_Socket0Register, eSn_IR, 0xFF);
                            }
                            break;
                            
                            default: {
                                /* Clear interrupts for other sockets to prevent lock-up */
                                Byte_t sn_ir_other = (Byte_t)W5500_ReadByteReg(Eth, eBSB_Socket0Register + (n * 4), eSn_IR);
                                W5500_WriteByteReg(Eth, eBSB_Socket0Register + (n * 4), eSn_IR, sn_ir_other);
                            }
                            break;
                        }
                    }
                }
            }
            
            /* Clear Global Interrupts */
            if (IR) { W5500_WriteByteReg(Eth, eBSB_CommonRegister, eIR, IR); }
            
        } while (SIR || (Attempt++ < MaxAttempt));
    }
}

/* INTERNAL TASK **********************************************************************************************************/

ReturnCode_t W5500_ForceClearRx(EthernetW5500_t* Ptr) {
    Word_t rx_wr;

    /* Place the actual comment here */
    /* Read current Write Pointer and sync Read Pointer to it */
    rx_wr = (Word_t)W5500_ReadDoubleByteReg(Ptr, eBSB_Socket0Register, eSn_RX_WR0);
    W5500_WriteDoubleByteReg(Ptr, eBSB_Socket0Register, eSn_RX_RD0, rx_wr);

    /* Place the actual comment here */
    /* Issue RECV command to tell chip we consumed everything */
    return W5500_WriteByteReg(Ptr, eBSB_Socket0Register, eSn_CR, 0x40);
}

/// @brief  Serivce handle W5500 module
/// @param arg Ignored
void EthernetAdapterCtl(void* arg){
    SysEntry("EthernetAdapterCtl");
    
    /* initialize mutex before creating motor object */
    /// EthLock = xSemaphoreCreateMutex();
    EthLock = xSemaphoreCreateBinary();
    
    SysLog("[EthernetAdapterCtl] New motor object...");
    Eth = W5500_Create(PIN_W5500_MISO, PIN_W5500_MOSI, PIN_W5500_CLK, PIN_W5500_SCS, PIN_W5500_RST, PIN_W5500_INT);
    
    if (IsNull(Eth)) {
        SysErr("[EthernetAdapterCtl] Cannot initialize `Eth`!");
        vTaskDelete(NULL);
        return;
    }
    
    EthernetAdapterCtl_Init(Eth);
    
    W5500_IsrTracking_Function = W5500_TaskIsrTrack;
    xTaskCreate(
        W5500_IsrTracking_Function,
        "",
        1024,
        NULL,
        eTask_Urgency,
        &W5500_IsrTracking_TaskHandle
    );


    SysLog("[AppService_HBridge] Join forever loop...");
    while (1) {
        uint64_t Data;
        Data = W5500_GetModuleVersion(Eth);

        // SysLog("[EthernetAdapterCtl] SPI Send header = {%02X%02X %02X}", 
        //     ((uint8_t*)&EthHeader)[2],
        //     ((uint8_t*)&EthHeader)[1],
        //     ((uint8_t*)&EthHeader)[0]
        // );

        // /// W5500_SendHeader(Eth, EthHeader);
        // SysLog("[EthernetAdapterCtl] SPI Receive 2 bytes data...");
        // W5500_ReceiveData(Eth, EthHeader, (void *)&Data, 2);
        // SysLog("[EthernetAdapterCtl] Result = %X", Data & 0xFFFFFFFFFF);

        Byte_t sir  = W5500_ReadByteReg(Eth, eBSB_CommonRegister, eSIR);
        Byte_t simr = W5500_ReadByteReg(Eth, eBSB_CommonRegister, eSIMR);
        Byte_t s0ir = W5500_ReadByteReg(Eth, eBSB_Socket0Register, eSn_IR);
        Byte_t s0imr = W5500_ReadByteReg(Eth, eBSB_Socket0Register, eSn_IMR);

        SysLog("EthernetAdapterCtl(...):  SIR=%02X (Mask=%02X) | S0_IR=%02X (Mask=%02X)", sir, simr, s0ir, s0imr);
        SysLog("EthernetAdapterCtl(...): Sn_RX_RSR0=%X", W5500_ReadDoubleByteReg(Eth, eBSB_Socket0Register, eSn_RX_RSR0));

        W5500_ForceClearRx(Eth);

        SysLog("");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* EOF *******************************************************************************************************************/
