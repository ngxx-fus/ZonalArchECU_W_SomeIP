#include "Module.h"

EthernetW5500_t *    Eth;
SemaphoreHandle_t    EthLock;

void EthernetAdapterCtl(void* arg){
    SysEntry("EthernetAdapterCtl");
    
    uint64_t        Data; 

    /* initialize mutex before creating motor object */
    EthLock = xSemaphoreCreateMutex();
    
    SysLog("[EthernetAdapterCtl] New motor object...");
    Eth = W5500_Create(PIN_W5500_MISO, PIN_W5500_MOSI, PIN_W5500_CLK, PIN_W5500_SCS, PIN_W5500_RST, PIN_W5500_INT);
    
    if (IsNull(Eth)) {
        SysErr("[EthernetAdapterCtl] Cannot initialize `Eth`!");
        vTaskDelete(NULL);
        return;
    }

    SysLog("EthernetAdapterCtl(...): Set ComReg=0");
    W5500_SetHeader(Eth, eW5500_CommonRegisters, eW5500_ModeMR); W5500_WriteByte(Eth, 0);
    SysLog("EthernetAdapterCtl(...): Result: %X", W5500_ReadByte(Eth));


    SysLog("EthernetAdapterCtl(...): Set GWAddr=10.0.0.1");
    W5500_SetHeader(Eth,  eW5500_CommonRegisters, eW5500_GWAddr0); 
    W5500_WriteQuartByte(Eth, IPv4ToUint32(10, 0, 0, 1));
    SysLog("EthernetAdapterCtl(...): Result: %X", W5500_ReadQuartByte(Eth));

    SysLog("EthernetAdapterCtl(...): Set SubnetMask=255.255.255.0");
    W5500_SetHeader(Eth, eW5500_CommonRegisters, eW5500_SubnetMask0);
    W5500_WriteQuartByte(Eth, IPv4ToUint32(255,255,255,0));
    SysLog("EthernetAdapterCtl(...): Result: %X", W5500_ReadQuartByte(Eth));


    SysLog("EthernetAdapterCtl(...): Set IP=10.0.0.19");
    W5500_SetHeader(Eth, eW5500_CommonRegisters, eW5500_SrcIPAddr0); 
    W5500_WriteQuartByte(Eth, IPv4ToUint32(10, 0, 0, 19));
    SysLog("EthernetAdapterCtl(...): Result: %X", W5500_ReadQuartByte(Eth));
    

    SysLog("EthernetAdapterCtl(...): Set MAC=00:08:DC:01:02:03");
    W5500_SetHeader(Eth, eW5500_CommonRegisters, eW5500_SrcMACAddr0);
    Data = MACToUint64(0x00, 0x02, 0xDC, 0x01, 0x02, 0x03); W5500_WriteNByte(Eth, (void*) &Data, 4);
    SysLog("EthernetAdapterCtl(...): Result: %X", W5500_ReadQuartByte(Eth));
    

    SysLog("[AppService_HBridge] Join forever loop...");
    while (1) {
        uint64_t Data0, Data1;
        Data = 0;
        Data0 = 0x17;
        Data1 = 0;
        /// W5500_LoopbackTest(Eth, (uint8_t*)&Data0, (uint8_t*)&Data1);
        W5500_SetHeader(Eth, eW5500_CommonRegisters, eW5500_VersionReg);
        Data = W5500_ReadByte(Eth);
        SysLog("[EthernetAdapterCtl] Get version %X", Data);

        // SysLog("[EthernetAdapterCtl] SPI Send header = {%02X%02X %02X}", 
        //     ((uint8_t*)&EthHeader)[2],
        //     ((uint8_t*)&EthHeader)[1],
        //     ((uint8_t*)&EthHeader)[0]
        // );

        // /// W5500_SendHeader(Eth, EthHeader);
        // SysLog("[EthernetAdapterCtl] SPI Receive 2 bytes data...");
        // W5500_ReceiveData(Eth, EthHeader, (void *)&Data, 2);
        // SysLog("[EthernetAdapterCtl] Result = %X", Data & 0xFFFFFFFFFF);

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}


