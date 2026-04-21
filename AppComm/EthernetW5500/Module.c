#include "Module.h"

EthernetW5500_t *    Eth;
SemaphoreHandle_t    EthLock;

// /// @brief Handle incoming interrupts from W5500 for MACRAW and ICMP
// /// @param arg Pointer to the EthernetW5500_t structure
// /// @return void
// void IRAM_ATTR W5500_OnPacketHandler_OldV0(void* arg) {
//     /* Retrieve Ethernet structure and check Global Interrupt Register */
//     EthernetW5500_t* Ptr = (EthernetW5500_t*)(arg);
//     W5500_SetHeader(Ptr, eW5500_SelectsCommonRegister, 0x0015); uint8_t ir = W5500_ReadByte(Ptr);

//     /* Read Socket 0 Interrupt Register */
//     W5500_SetHeader(Ptr, eW5500_SelectsSocket0Register, 0x0002); uint8_t s0_ir = W5500_ReadByte(Ptr);

//     /* Process MACRAW Receive Interrupt on Socket 0 */
//     if (s0_ir & 0x04) {
//         uint16_t rx_rd;
//         uint16_t packet_size;
//         uint8_t size_header[2];
        
//         /* Locate and read 2-byte size header */
//         W5500_SetHeader(Ptr, eW5500_SelectsSocket0Register, 0x0028); rx_rd = W5500_ReadDoubleByte(Ptr);
//         W5500_SetHeader(Ptr, eW5500_SelectsSocket0RXBuffer, rx_rd); W5500_ReadNByte(Ptr, size_header, 2);
//         packet_size = (size_header[0] << 8) | size_header[1];
        
//         /* Calculate frame length and extract payload */
//         uint16_t frame_len = packet_size - 2;
//         if (frame_len > 0) {
//             uint16_t read_len = (frame_len > 1514) ? 1514 : frame_len;
//             W5500_SetHeader(Ptr, eW5500_SelectsSocket0RXBuffer, rx_rd + 2); W5500_ReadNByte(Ptr, Ptr->RxFrame.Payload.Byte, read_len);
            
//             SysLog("MACRAW Received! Size: %u bytes", frame_len);
//             SysLog("Dest MAC: %02X:%02X:%02X:%02X:%02X:%02X", Ptr->RxFrame.Payload.Byte[0], Ptr->RxFrame.Payload.Byte[1], Ptr->RxFrame.Payload.Byte[2], Ptr->RxFrame.Payload.Byte[3], Ptr->RxFrame.Payload.Byte[4], Ptr->RxFrame.Payload.Byte[5]);
//             SysLog("Src MAC: %02X:%02X:%02X:%02X:%02X:%02X", Ptr->RxFrame.Payload.Byte[6], Ptr->RxFrame.Payload.Byte[7], Ptr->RxFrame.Payload.Byte[8], Ptr->RxFrame.Payload.Byte[9], Ptr->RxFrame.Payload.Byte[10], Ptr->RxFrame.Payload.Byte[11]);
//         }
        
//         /* Flush Socket 0 buffer */
//         rx_rd += packet_size;
//         W5500_SetHeader(Ptr, eW5500_SelectsSocket0Register, 0x0028); W5500_WriteDoubleByte(Ptr, rx_rd);
//         W5500_SetHeader(Ptr, eW5500_SelectsSocket0Register, 0x0001); W5500_WriteByte(Ptr, 0x40);
//     }

//     /* Process MACRAW Send OK Interrupt */
//     if (s0_ir & 0x10) {
//         SysLog("W5500_OnPacketHandler(...): MACRAW Data Sent");
//     }

//     /* Clear Socket 0 interrupts */
//     if (s0_ir) {
//         W5500_SetHeader(Ptr, eW5500_SelectsSocket0Register, 0x0002); W5500_WriteByte(Ptr, 0xFF);
//     }

//     /* Read and process Socket 7 Interrupt Register */
//     W5500_SetHeader(Ptr, eW5500_SelectsSocket7Register, 0x0002); uint8_t s7_ir = W5500_ReadByte(Ptr);
    
//     /* Clear Socket 7 interrupts */
//     if (s7_ir) {
//         SysLog("W5500_OnPacketHandler(...): ICMP Interrupt on S7");
//         W5500_SetHeader(Ptr, eW5500_SelectsSocket7Register, 0x0002); W5500_WriteByte(Ptr, s7_ir);
//     }

//     /* Clear Global Interrupt */
//     if (ir) {
//         W5500_SetHeader(Ptr, eW5500_SelectsCommonRegister, 0x0015); W5500_WriteByte(Ptr, ir);
//     }
// }

// /// @brief Handle incoming interrupts from W5500 for MACRAW and ICMP
// /// @param arg Pointer to the EthernetW5500_t structure
// /// @return void
// void IRAM_ATTR W5500_OnPacketHandler_OldV1(void* arg) {
//     /* Retrieve Ethernet structure and check Global Interrupt Register */
//     EthernetW5500_t* Ptr = (EthernetW5500_t*)(arg);
//     W5500_SetHeader(Ptr, eW5500_SelectsCommonRegister, 0x0015); uint8_t ir = W5500_ReadByte(Ptr);

//     /* Read Socket 0 Interrupt Register */
//     W5500_SetHeader(Ptr, eW5500_SelectsSocket0Register, 0x0002); uint8_t s0_ir = W5500_ReadByte(Ptr);

//     /* Process MACRAW Receive Interrupt on Socket 0 */
//     if (s0_ir & 0x04) {
//         uint16_t rx_rd;
//         uint16_t packet_size;
//         uint8_t size_header[2];
        
//         /* Locate and read 2-byte size header */
//         W5500_SetHeader(Ptr, eW5500_SelectsSocket0Register, 0x0028); rx_rd = W5500_ReadDoubleByte(Ptr);
//         W5500_SetHeader(Ptr, eW5500_SelectsSocket0RXBuffer, rx_rd); W5500_ReadNByte(Ptr, size_header, 2);
//         packet_size = (size_header[0] << 8) | size_header[1];
        
//         /* Calculate frame length and extract payload */
//         uint16_t frame_len = packet_size - 2;
//         if (frame_len > 0) {
//             uint16_t read_len = (frame_len > 1514) ? 1514 : frame_len;
//             W5500_SetHeader(Ptr, eW5500_SelectsSocket0RXBuffer, rx_rd + 2); W5500_ReadNByte(Ptr, Ptr->RxFrame.Payload.Byte, read_len);
            
//             SysLog("MACRAW Received! Size: %u bytes", frame_len);
//             SysLog("Dest MAC: %02X:%02X:%02X:%02X:%02X:%02X", Ptr->RxFrame.Payload.Byte[0], Ptr->RxFrame.Payload.Byte[1], Ptr->RxFrame.Payload.Byte[2], Ptr->RxFrame.Payload.Byte[3], Ptr->RxFrame.Payload.Byte[4], Ptr->RxFrame.Payload.Byte[5]);
//             SysLog("Src MAC: %02X:%02X:%02X:%02X:%02X:%02X", Ptr->RxFrame.Payload.Byte[6], Ptr->RxFrame.Payload.Byte[7], Ptr->RxFrame.Payload.Byte[8], Ptr->RxFrame.Payload.Byte[9], Ptr->RxFrame.Payload.Byte[10], Ptr->RxFrame.Payload.Byte[11]);
            
//             /* Print entire raw frame payload converting unprintable to '#' in chunks to save ISR stack */
//             char print_buf[65];
//             uint16_t buf_idx = 0;
//             for (uint16_t i = 0; i < read_len; i++) {
//                 uint8_t c = Ptr->RxFrame.Payload.Byte[i];
//                 print_buf[buf_idx++] = (c >= 32 && c <= 126) ? c : '#';
                
//                 if (buf_idx == 64 || i == read_len - 1) {
//                     print_buf[buf_idx] = '\0';
//                     SysLog("Raw Data: %s", print_buf);
//                     buf_idx = 0;
//                 }
//             }
//         }
        
//         /* Flush Socket 0 buffer */
//         rx_rd += packet_size;
//         W5500_SetHeader(Ptr, eW5500_SelectsSocket0Register, 0x0028); W5500_WriteDoubleByte(Ptr, rx_rd);
//         W5500_SetHeader(Ptr, eW5500_SelectsSocket0Register, 0x0001); W5500_WriteByte(Ptr, 0x40);
//     }

//     /* Process MACRAW Send OK Interrupt */
//     if (s0_ir & 0x10) {
//         SysLog("W5500_OnPacketHandler(...): MACRAW Data Sent");
//     }

//     /* Clear Socket 0 interrupts */
//     if (s0_ir) {
//         W5500_SetHeader(Ptr, eW5500_SelectsSocket0Register, 0x0002); W5500_WriteByte(Ptr, 0xFF);
//     }

//     /* Read and process Socket 7 Interrupt Register */
//     W5500_SetHeader(Ptr, eW5500_SelectsSocket7Register, 0x0002); uint8_t s7_ir = W5500_ReadByte(Ptr);
    
//     /* Clear Socket 7 interrupts */
//     if (s7_ir) {
//         SysLog("W5500_OnPacketHandler(...): ICMP Interrupt on S7");
//         W5500_SetHeader(Ptr, eW5500_SelectsSocket7Register, 0x0002); W5500_WriteByte(Ptr, s7_ir);
//     }

//     /* Clear Global Interrupt */
//     if (ir) {
//         W5500_SetHeader(Ptr, eW5500_SelectsCommonRegister, 0x0015); W5500_WriteByte(Ptr, ir);
//     }
// }

/// @brief Handle incoming interrupts from W5500 for MACRAW and ICMP
/// @param arg Pointer to the EthernetW5500_t structure
/// @return void
void IRAM_ATTR W5500_OnPacketHandler(void* arg) {
    /* Retrieve Ethernet structure and check Global Interrupt Register [cite: 2429] */
    EthernetW5500_t* Ptr = (EthernetW5500_t*)(arg);
    W5500_SetHeader(Ptr, eW5500_SelectsCommonRegister, 0x0015); uint8_t ir = W5500_ReadByte(Ptr);

    /* Read Socket 0 Interrupt Register [cite: 2587] */
    W5500_SetHeader(Ptr, eW5500_SelectsSocket0Register, 0x0002); uint8_t s0_ir = W5500_ReadByte(Ptr);

    /* Process MACRAW Receive Interrupt on Socket 0 [cite: 2592] */
    if (s0_ir & 0x04) {
        uint16_t rx_rd;
        uint16_t packet_size;
        uint8_t size_header[2];
        
        /* Locate and read 2-byte size header containing the total frame length [cite: 2113, 2118] */
        W5500_SetHeader(Ptr, eW5500_SelectsSocket0Register, 0x0028); rx_rd = W5500_ReadDoubleByte(Ptr);
        W5500_SetHeader(Ptr, eW5500_SelectsSocket0RXBuffer, rx_rd); W5500_ReadNByte(Ptr, size_header, 2);
        packet_size = (size_header[0] << 8) | size_header[1];
        
        /* Calculate actual frame length and extract payload into internal buffer  */
        uint16_t frame_len = packet_size - 2;
        if (frame_len > 0) {
            uint16_t read_len = (frame_len > 1514) ? 1514 : frame_len;
            W5500_SetHeader(Ptr, eW5500_SelectsSocket0RXBuffer, rx_rd + 2); W5500_ReadNByte(Ptr, Ptr->RxFrame.Payload.Byte, read_len);
            
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
                    /* Filter printable ASCII characters vs non-printable hex [cite: 2022] */
                    if (byte >= 0x20 && byte <= 0x7E) {
                        pos += sprintf(line_buf + pos, "'%c' ", byte);
                    } else {
                        pos += sprintf(line_buf + pos, "[%02X] ", byte);
                    }
                }
                SysLog("%s", line_buf);
            }
        }
        
        /* Update the RX Read Pointer and notify W5500 with RECV command [cite: 2749, 2586] */
        rx_rd += packet_size;
        W5500_SetHeader(Ptr, eW5500_SelectsSocket0Register, 0x0028); W5500_WriteDoubleByte(Ptr, rx_rd);
        W5500_SetHeader(Ptr, eW5500_SelectsSocket0Register, 0x0001); W5500_WriteByte(Ptr, 0x40);
    }

    /* Process MACRAW Send OK Interrupt */
    if (s0_ir & 0x10) {
        SysLog("W5500_OnPacketHandler(...): MACRAW Data Sent");
    }

    /* Clear Socket 0 interrupts by writing '1' to set bits [cite: 2590] */
    if (s0_ir) {
        W5500_SetHeader(Ptr, eW5500_SelectsSocket0Register, 0x0002); W5500_WriteByte(Ptr, 0xFF);
    }

    /* Read and process Socket 7 Interrupt Register */
    W5500_SetHeader(Ptr, eW5500_SelectsSocket7Register, 0x0002); uint8_t s7_ir = W5500_ReadByte(Ptr);
    
    /* Clear Socket 7 interrupts */
    if (s7_ir) {
        SysLog("W5500_OnPacketHandler(...): ICMP Interrupt on S7");
        W5500_SetHeader(Ptr, eW5500_SelectsSocket7Register, 0x0002); W5500_WriteByte(Ptr, s7_ir);
    }

    /* Clear Global Interrupt status bit by writing '1' [cite: 2430] */
    if (ir) {
        W5500_SetHeader(Ptr, eW5500_SelectsCommonRegister, 0x0015); W5500_WriteByte(Ptr, ir);
    }
}

/// @brief Initialize W5500 network interface for MACRAW and ICMP
/// @param Eth Pointer to the Ethernet W5500 controller structure
/// @return STAT_OKE upon successful initialization
ReturnCode_t EthernetAdapterCtl_Init(EthernetW5500_t * Eth) {
    /* Initialize W5500 common registers and network parameters */
    SysEntry("EthernetAdapterCtl_Init");
    uint64_t Data;

    /* Reset device and set mode register */
    W5500_SetHeader(Eth, eW5500_SelectsCommonRegister, eW5500_ModeMR); W5500_WriteByte(Eth, 0x80);

    /* Configure Gateway Address */
    W5500_SetHeader(Eth, eW5500_SelectsCommonRegister, eW5500_GWAddr0); W5500_WriteQuartByte(Eth, ConvertByteArrayToInt32_BigEndian(GW_IP_ADDR, 4));

    /* Configure Subnet Mask */
    W5500_SetHeader(Eth, eW5500_SelectsCommonRegister, eW5500_SubnetMask0); W5500_WriteQuartByte(Eth, ConvertByteArrayToInt32_BigEndian(SUBNET_MASK, 4));

    /* Configure Source IP Address */
    W5500_SetHeader(Eth, eW5500_SelectsCommonRegister, eW5500_SrcIPAddr0); W5500_WriteQuartByte(Eth, ConvertByteArrayToInt32_BigEndian(SRC_IP_ADDR, 4));
    
    /* Configure Hardware MAC Address (Hardware auto-replies to ARP using this) */
    W5500_SetHeader(Eth, eW5500_SelectsCommonRegister, eW5500_SrcMACAddr0); Data = ConvertByteArrayToInt64_BigEndian(SRC_MAC_ADDR, 6); W5500_WriteNByte(Eth, (void*) &Data, 6);

    /* Enable Socket 0 and Socket 7 global interrupt in SIMR */
    W5500_SetHeader(Eth, eW5500_SelectsCommonRegister, eW5500_SocketIntMaskReg); W5500_WriteByte(Eth, 0x81);

    /* Configure Interrupt Assert Wait Time (INTLEVEL) to prevent missed edge interrupts */
    W5500_SetHeader(Eth, eW5500_SelectsCommonRegister, eW5500_IntLowLevel0); W5500_WriteDoubleByte(Eth, 0x03E8);

    /* Configure Socket 0 for MACRAW */
    SysLog("EthernetAdapterCtl(...): Configure Socket 0 for MACRAW");
    W5500_SetHeader(Eth, eW5500_SelectsSocket0Register, 0x002C); W5500_WriteByte(Eth, 0x14); /* Sn_IMR: RECV & SEND_OK */
    W5500_SetHeader(Eth, eW5500_SelectsSocket0Register, 0x0000); W5500_WriteByte(Eth, 0x04); /* Sn_MR: MACRAW */
    W5500_SetHeader(Eth, eW5500_SelectsSocket0Register, 0x0001); W5500_WriteByte(Eth, 0x01); /* Sn_CR: OPEN */

    /* Configure Socket 7 for Ping (ICMP) */
    SysLog("EthernetAdapterCtl(...): Configure Socket 7 for ICMP");
    W5500_SetHeader(Eth, eW5500_SelectsSocket7Register, 0x0014); W5500_WriteByte(Eth, 0x01); /* Sn_PROTO: ICMP */
    W5500_SetHeader(Eth, eW5500_SelectsSocket7Register, 0x0000); W5500_WriteByte(Eth, 0x03); /* Sn_MR: IPRAW */
    W5500_SetHeader(Eth, eW5500_SelectsSocket7Register, 0x0001); W5500_WriteByte(Eth, 0x01); /* Sn_CR: OPEN */

    SysExit("EthernetAdapterCtl_Init");
    return STAT_OKE;
}

/// @brief  Serivce handle W5500 module
/// @param arg Ignored
void EthernetAdapterCtl(void* arg){
    SysEntry("EthernetAdapterCtl");
    
    /* initialize mutex before creating motor object */
    EthLock = xSemaphoreCreateMutex();
    
    SysLog("[EthernetAdapterCtl] New motor object...");
    Eth = W5500_Create(PIN_W5500_MISO, PIN_W5500_MOSI, PIN_W5500_CLK, PIN_W5500_SCS, PIN_W5500_RST, PIN_W5500_INT);
    
    if (IsNull(Eth)) {
        SysErr("[EthernetAdapterCtl] Cannot initialize `Eth`!");
        vTaskDelete(NULL);
        return;
    }

    EthernetAdapterCtl_Init(Eth);
    

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

        vTaskDelay(pdMS_TO_TICKS(20000));
    }
}


