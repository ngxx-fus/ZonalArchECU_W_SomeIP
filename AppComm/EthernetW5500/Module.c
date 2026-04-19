#include "Module.h"

EthernetW5500_t *    Eth;
SemaphoreHandle_t    EthLock;

/// @brief Initialize W5500 network interface with static IP, Gateway, Mask, and MAC
/// @param Eth Pointer to the Ethernet W5500 controller structure
/// @return STAT_OKE upon successful initialization
ReturnCode_t EthernetAdapterCtl_Init(EthernetW5500_t * Eth) {
    /* Initialize W5500 common registers and network parameters */
    SysEntry("EthernetAdapterCtl_Init");
    uint64_t Data;

    /* Reset device and set mode register */
    SysLog("EthernetAdapterCtl(...): Set ComReg=0");
    W5500_SetHeader(Eth, eW5500_SelectsCommonRegister, eW5500_ModeMR); W5500_WriteByte(Eth, 0x80);
    SysLog("EthernetAdapterCtl(...): Result: %X", W5500_ReadByte(Eth));

    /* Configure Gateway Address */
    SysLog("EthernetAdapterCtl(...): Set GWAddr=%s", GW_IP_ADDR_STR);
    W5500_SetHeader(Eth,  eW5500_SelectsCommonRegister, eW5500_GWAddr0); 
    W5500_WriteQuartByte(Eth, ConvertByteArrayToInt32_BigEndian(GW_IP_ADDR, 4));
    SysLog("EthernetAdapterCtl(...): Result: %X", W5500_ReadQuartByte(Eth));

    /* Configure Subnet Mask */
    SysLog("EthernetAdapterCtl(...): Set SubnetMask=%s", SUBNET_MASK_STR);
    W5500_SetHeader(Eth, eW5500_SelectsCommonRegister, eW5500_SubnetMask0);
    W5500_WriteQuartByte(Eth, ConvertByteArrayToInt32_BigEndian(SUBNET_MASK, 4));
    SysLog("EthernetAdapterCtl(...): Result: %X", W5500_ReadQuartByte(Eth));

    /* Configure Source IP Address */
    SysLog("EthernetAdapterCtl(...): Set IP=%s", SRC_IP_ADDR_STR);
    W5500_SetHeader(Eth, eW5500_SelectsCommonRegister, eW5500_SrcIPAddr0); 
    W5500_WriteQuartByte(Eth, ConvertByteArrayToInt32_BigEndian(SRC_IP_ADDR, 4));
    SysLog("EthernetAdapterCtl(...): Result: %X", W5500_ReadQuartByte(Eth));
    
    /* Configure Hardware MAC Address */
    SysLog("EthernetAdapterCtl(...): Set MAC=00:08:DC:01:02:03");
    W5500_SetHeader(Eth, eW5500_SelectsCommonRegister, eW5500_SrcMACAddr0);
    Data = ConvertByteArrayToInt64_BigEndian(SRC_MAC_ADDR, 6); 
    W5500_WriteNByte(Eth, (void*) &Data, 6);
    Data = 0; W5500_ReadNByte(Eth, (void*) &Data, 6);
    SysLog("EthernetAdapterCtl(...): Result: %012LX", Data);
    
    SysExit("EthernetAdapterCtl_Init");
    return STAT_OKE;
}

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


