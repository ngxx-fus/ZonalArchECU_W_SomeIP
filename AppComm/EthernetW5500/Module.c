#include "Module.h"

EthernetW5500_t *    Eth;
SemaphoreHandle_t    EthLock;

void EthernetAdapterCtl(void* arg){
    SysEntry("EthernetAdapterCtl");
    
    W5500Header_t   EthHeader;
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

    memset(&EthHeader, 0, sizeof(EthHeader));
    EthHeader.AddrPhase = eW5500_VersionReg;
    EthHeader.nRW       = eW5500_Read;
    EthHeader.BlockSel  = 0;
    EthHeader.OpMode    = 2;
    
    SysLog("[AppService_HBridge] Join forever loop...");
    while (1) {
        uint64_t Data0, Data1;
        Data = 0;
        Data0 = 0x17;
        Data1 = 0;
        W5500_LoopbackTest(Eth, (uint8_t*)&Data0, (uint8_t*)&Data1);
        SysLog("[EthernetAdapterCtl] Loopback test [%X], [%X]", Data0, Data1);

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


