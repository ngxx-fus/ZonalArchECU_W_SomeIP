#include "Module.h"
#include "RxedPacketQueue.h"

EthernetW5500_t *   Eth;
SemaphoreHandle_t   EthLock;

EthernetW5500_t *   Eth;
TaskHandle_t        W5500CommCtl_TaskHandler;

GenericDeque_t  *   TxDeque;
GenericDeque_t  *   RxDeque;

/* INTERNAL TASK **********************************************************************************************************/

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
        int pos = 0;
        pos += sprintf(line_buf + pos, "%04X: ", i);

        for (int j = 0; j < 8 && (i + j) < Size; j++) {
            Byte_t byte = pData[i + j];
            /* Distinguish between printable ASCII and non-printable hex values */
            if (byte >= 0x20 && byte <= 0x7E) {
                pos += sprintf(line_buf + pos, " %c ", byte);
            }
            else {
                pos += sprintf(line_buf + pos, "%02X ", byte);
            }
        }
        SysLog("%s", line_buf);
    }
    SysExit("W5500_LogFrame");
}

/// @brief Receive packet from W5500
static void W5500_TaskComm_Socket0Receive(){
    Byte_t SIR, IR, Sn_IR, n;
    Word_t rx_rd, packet_size;
    Sn_IR = (Byte_t)W5500_ReadByteReg(Eth, eBSB_Socket0Register, eSn_IR);
    SysLog("W5500_TaskComm(...): S0_IR=%02X", Sn_IR);
    
    
    /* Handle Received Data */
    if (Sn_IR & 0x04) {
        rx_rd       = (Word_t)W5500_ReadDoubleByteReg(Eth, eBSB_Socket0Register, eSn_RX_RD0);
        packet_size = (Word_t)W5500_ReadDoubleByteReg(Eth, eBSB_Socket0RxBuffer, rx_rd);
        
        
        /* Calculate frame length (excluding 2-byte header) and read data */
        Word_t frame_len = packet_size - 2;
        if (frame_len > 0) {
            Word_t read_len = (frame_len > 1514) ? 1514 : frame_len;
            W5500_ReadNByteReg(Eth, eBSB_Socket0RxBuffer, rx_rd + 2, Eth->RxFrame.Payload.Byte, read_len);
            // W5500_LogFrame(Eth, read_len);
            SysLog("W5500_TaskComm(...): Saved %d bytes of frame", read_len);
            RxedPacket_Push(read_len, (GenericPtr_t) Eth->RxFrame.Payload.Byte);
        }
        
        
        /* Advance read pointer and confirm with RECV command */
        rx_rd += packet_size;
        W5500_WriteDoubleByteReg(Eth, eBSB_Socket0Register, eSn_RX_RD0, rx_rd);
        W5500_WriteByteReg(Eth, eBSB_Socket0Register, eSn_CR, 0x40);
    }
    
    if (Sn_IR & 0x10) { SysLog("W5500_TaskComm(...): MACRAW Data Sent"); }
    
    /* Clear Socket 0 interrupts */
    W5500_WriteByteReg(Eth, eBSB_Socket0Register, eSn_IR, 0xFF);
}

/// @brief This task will be woken up when an isr is occurred!
/// @param arg Pointer to the EthernetW5500_t structure
void W5500_TaskComm(void* arg) {
    Byte_t Attempt;
    Byte_t const MaxAttempt = 3;
    Byte_t SIR, IR, Sn_IR, n;

    while (1) {
        /* Reset local variables before sleep */
        Attempt = 0;
        SysLog("");
        SysLog("W5500_TaskComm(...): Task will wake W5500CommCtl wake before sleep!\n");
        xTaskNotifyGive(W5500CommCtl_TaskHandler);
        SysLog("W5500_TaskComm(...): Task will be put to sleep!\n");
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        
        /* Task wake up */
        SysLog("W5500_TaskComm(...): Task is woken up!");
        
        /* Loop until no more interrupts or max attempts reached */
        do {
            SIR = (Byte_t)W5500_ReadByteReg(Eth, eBSB_CommonRegister, eSIR);
            IR  = (Byte_t)W5500_ReadByteReg(Eth, eBSB_CommonRegister, eIR);
            SysLog("W5500_TaskComm(...): SIR=%02X, IR=%02X", SIR, IR);
            
            if (SIR) {
                for (n = 0; n < 8; ++n) {
                    if ((1U << n) & SIR) {
                        /* Socket N has interrupt event */
                        switch (n) {
                            case 0:
                                W5500_TaskComm_Socket0Receive();
                            break;
                            
                            default:
                                /* Clear interrupts for other sockets to prevent lock-up */
                                Byte_t sn_ir_other = (Byte_t)W5500_ReadByteReg(Eth, eBSB_Socket0Register + (n * 4), eSn_IR);
                                W5500_WriteByteReg(Eth, eBSB_Socket0Register + (n * 4), eSn_IR, sn_ir_other);
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

/* MODULE INIT *************************************************************************************************************/

/// @brief Initialize W5500 network interface for MACRAW and ICMP
/// @param Eth Pointer to the Ethernet W5500 controller structure
/// @return STAT_OKE upon successful initialization
ReturnCode_t W5500CommCtl_Init(EthernetW5500_t * Eth) { 
    /* Initialize W5500 common registers and network parameters */
    SysEntry("W5500CommCtl_Init");
    uint64_t Data;

    EthLock = xSemaphoreCreateBinary();
    xSemaphoreGive(EthLock);

    /* Reset device and set mode register */
    W5500_SetHeader(Eth, eBSB_CommonRegister, eMR); W5500_WriteByte(Eth, 0x80);

    Data = W5500_GetModuleVersion(Eth);
    if((Data&0xFF) != 0x4){
        SysErr("W5500CommCtl_Init(...): W5500 Version (=%X) is not supported!", Data & 0xFF);
        return STAT_ERR_UNSUPPORTED;
    }
    SysLog("W5500CommCtl_Init(...): W5500 Version = %X", Data & 0xFF);

    /* Configure Gateway Address */
    W5500_WriteQuartByteReg(Eth, eBSB_CommonRegister, eGAR0, ConvertByteArrayToInt32_BigEndian(GW_IP_ADDR, 4));
    SysLog("W5500CommCtl_Init(...): GATEWAY IP=%X", W5500_ReadQuartByteReg(Eth, eBSB_CommonRegister, eGAR0));

    /* Configure Subnet Mask */
    W5500_WriteQuartByteReg(Eth, eBSB_CommonRegister, eSUBR0, ConvertByteArrayToInt32_BigEndian(SUBNET_MASK, 4));
    SysLog("W5500CommCtl_Init(...): SUBNET MASK=%X", W5500_ReadQuartByteReg(Eth, eBSB_CommonRegister, eSUBR0));

    /* Configure Source IP Address */
    W5500_WriteQuartByteReg(Eth, eBSB_CommonRegister, eSIPR0, ConvertByteArrayToInt32_BigEndian(SRC_IP_ADDR, 4));
    SysLog("W5500CommCtl_Init(...): SOURCE IP=%X", W5500_ReadQuartByteReg(Eth, eBSB_CommonRegister, eSIPR0));
    
    /* Configure Hardware MAC Address (Hardware auto-replies to ARP using this) */
    Data = ConvertByteArrayToInt64_BigEndian(SRC_MAC_ADDR, 6);
    W5500_WriteNByteReg(Eth, eBSB_CommonRegister, eSHAR0, (void*) &Data, 6);
    Data = 0;
    W5500_ReadNByteReg(Eth, eBSB_CommonRegister, eSHAR0, (void*) &Data, 6);
    SysLog("W5500CommCtl_Init(...): MAC ADDR=%012llX", Data);


    /* Enable Socket 0 and Socket 7 global interrupt in SIMR */
    W5500_WriteByteReg(Eth, eBSB_CommonRegister, eIMR, 0x81);
    SysLog("W5500CommCtl_Init(...): SIMR=%X", W5500_ReadByteReg(Eth, eBSB_CommonRegister, eIMR));

    /* Configure Interrupt Assert Wait Time (INTLEVEL) to prevent missed edge interrupts */
    W5500_WriteDoubleByteReg(Eth, eBSB_CommonRegister, eINTLEVEL0, 0x03E8);
    SysLog("W5500CommCtl_Init(...): INTLEVEL0=%X", W5500_ReadDoubleByteReg(Eth, eBSB_CommonRegister, eINTLEVEL0));

    /* Configure Global Interrupt Mask (IMR) */
    /* Enable IP Conflict (0x80) and Destination Unreachable (0x10) interrupts */
    W5500_WriteByteReg(Eth, eBSB_CommonRegister, eIMR, 0x90);
    SysLog("W5500CommCtl_Init(...): IMR=%X", W5500_ReadByteReg(Eth, eBSB_CommonRegister, eIMR));

    /* Clear any existing ghost interrupts in IR and SIR (Write 1 to clear) */
    W5500_SetHeader(Eth, eBSB_CommonRegister, 0x0015); 
    uint8_t dummy_ir = W5500_ReadByte(Eth); 
    W5500_WriteByte(Eth, dummy_ir);

    W5500_SetHeader(Eth, eBSB_CommonRegister, 0x0017); 
    uint8_t dummy_sir = W5500_ReadByte(Eth);
    W5500_WriteByte(Eth, dummy_sir);

    /* Configure Socket 0 for MACRAW */
    SysLog("W5500CommCtl(...): Configure Socket 0 for MACRAW");
    W5500_SetHeader(Eth, eBSB_Socket0Register, 0x002C); W5500_WriteByte(Eth, 0x14); /* Sn_IMR: RECV & SEND_OK */
    W5500_SetHeader(Eth, eBSB_Socket0Register, 0x0000); W5500_WriteByte(Eth, 0x04); /* Sn_MR: MACRAW */
    W5500_SetHeader(Eth, eBSB_Socket0Register, 0x0001); W5500_WriteByte(Eth, 0x01); /* Sn_CR: OPEN */

    W5500_WriteByteReg(Eth, eBSB_CommonRegister, eSIMR, 0x01);
    W5500_WriteByteReg(Eth, eBSB_Socket0Register, eSn_RXBUF_SIZE, 0x08);

    /* Configure Socket 7 for Ping (ICMP) */
    // SysLog("W5500CommCtl(...): Configure Socket 7 for ICMP");
    // W5500_SetHeader(Eth, eW5500_SelectsSocket7Register, 0x0014); W5500_WriteByte(Eth, 0x01); /* Sn_PROTO: ICMP */
    // W5500_SetHeader(Eth, eW5500_SelectsSocket7Register, 0x0000); W5500_WriteByte(Eth, 0x03); /* Sn_MR: IPRAW */
    // W5500_SetHeader(Eth, eW5500_SelectsSocket7Register, 0x0001); W5500_WriteByte(Eth, 0x01); /* Sn_CR: OPEN */

    SysExit("W5500CommCtl_Init");
    return STAT_OKE;
}

/* INTERNAL TASK **********************************************************************************************************/

/// @brief  Serivce handle W5500 module
/// @param arg Ignored
void W5500CommCtl(void* arg){
    SysEntry("W5500CommCtl");
    
    ReturnCode_t RetVal;
    static Word_t Size;
    static Byte_t PayloadByte[512];
    
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
    W5500_TaskComming_Function = W5500_TaskComm;
    xTaskCreate(
        W5500_TaskComming_Function,
        "",
        1024,
        NULL,
        eTask_Urgency,
        &W5500_TaskComm_TaskHandler
    );

    /*Get current handler*/
    W5500CommCtl_TaskHandler = xTaskGetCurrentTaskHandle();
    /*Runtime service to handle internal communication*/
    SysLog("W5500CommCtl(...) : Join forever loop...");
    while (1) {
        uint64_t Data;
        SysLog("W5500CommCtl(...) : W5500 version=%X", W5500_GetModuleVersion(Eth));

        SysLog("W5500CommCtl(...):  Check RxedPacket and log...");
        EthSize_t i;
        SysLog("W5500CommCtl(...):  #Rexed packets=%d", RxedPacket_Size());
        while(RxedPacket_Size() > 0){
            SysLog("W5500CommCtl(...):  Print packet #Top, Size=%d", RxedPacket_Size());
            xSemaphoreTake(RxedPacket_Lock, portMAX_DELAY);
            memcpy(PayloadByte, RxedPacket_Top()->Ptr, Min(RxedPacket_Top()->Size, 120));
            Size = Min(RxedPacket_Top()->Size, 120);
            xSemaphoreGive(RxedPacket_Lock);
            W5500_LogFrame((GenericPtr_t)PayloadByte, Size, GenericNullPtr, GenericNullPtr);
            
            RxedPacket_Pop();
            SysLog("");
        }

        SysLog("");

        /*Sleep and wake-up for every 5000ms/a triggered event*/
        SysLog("W5500CommCtl(...) : Task will be put to sleep!");
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(5000));
        
        SysLog("");
        SysLog("W5500CommCtl(...) : Task woken up!");
    }

    /*For exit and delete task when an unexpected error occurred*/
    CLEANUP_AND_EXIT:
        vSemaphoreDelete(EthLock);
        vSemaphoreDelete(RxedPacket_Lock);
        EthLock = NULL;
        RxedPacket_Lock = NULL;
        vTaskDelete(NULL);
}

/* EOF *******************************************************************************************************************/
