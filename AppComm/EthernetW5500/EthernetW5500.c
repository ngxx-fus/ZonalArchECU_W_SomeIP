#include "EthernetW5500.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include <string.h>
#include <stdio.h>



/* Static handle for the SPI device instance */
static spi_device_handle_t w5500_spi_handle;

static void IRAM_ATTR W5500_OnPacketHandler(void* arg){
    EthernetW5500_t* Ptr = (EthernetW5500_t*)(arg);
    SysLog("W5500_OnPacketHandler(...): Incoming packet!");
}

/// @brief Allocate new EthernetW5500_t with preset pin; Init HSPI (SCS is manually controled)
/// @param MISO Master In Slave Out pin
/// @param MOSI Master Out Slave In pin
/// @param CLK Serial Clock pin
/// @param SCS Slave Chip Select pin
/// @param RST Reset pin
/// @param INT Interrupt pin
/// @return ReturnCode_t* Pointer to the allocated object cast as ReturnCode_t*
EthernetW5500_t* W5500_Create(Pin_t MISO, Pin_t MOSI, Pin_t CLK, Pin_t SCS, Pin_t RST, Pin_t INT) {
    EthernetW5500_t* obj = (EthernetW5500_t*)malloc(sizeof(EthernetW5500_t));
    if (obj == NULL) return NULL;

    /* initialize memory and pins */
    memset(obj, 0, sizeof(EthernetW5500_t));
    obj->Pinout.MISO = MISO; obj->Pinout.MOSI = MOSI;
    obj->Pinout.CLK = CLK;   obj->Pinout.SCS = SCS;
    obj->Pinout.RST = RST;   obj->Pinout.INT = INT;

    /* setup GPIO for manual SCS and RST control */
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << obj->Pinout.SCS) | (1ULL << obj->Pinout.RST),
        .pull_down_en = 0, .pull_up_en = 0
    };
    gpio_config(&io_conf);
    gpio_set_level(obj->Pinout.SCS, 1);
    gpio_set_level(obj->Pinout.RST, 1);

    /* setup GPIO for Input Interrupt pin (INT) */
    gpio_config_t in_conf = {
        .intr_type = GPIO_INTR_NEGEDGE, /* W5500 INT is active low */
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << obj->Pinout.INT),
        .pull_up_en = 1, .pull_down_en = 0
    };
    gpio_config(&in_conf);

    /* configure SPI bus [cite: 110] */
    spi_bus_config_t bus_cfg = {
        .miso_io_num = MISO, .mosi_io_num = MOSI, .sclk_io_num = CLK,
        .quadwp_io_num = -1, .quadhd_io_num = -1,
        .max_transfer_sz = 1600
    };
    spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);

    /* add W5500 to the bus with manual CS */
    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = 25 * 1000 * 1000,
        .mode = 0, .spics_io_num = -1,
        .queue_size = 7
    };
    spi_bus_add_device(SPI2_HOST, &dev_cfg, &w5500_spi_handle);
    /* install ISR service and add handler for W5500 INT pin */
    gpio_install_isr_service(0);
    gpio_isr_handler_add(obj->Pinout.INT, W5500_OnPacketHandler, (void*) obj);
    return (EthernetW5500_t*) obj;
}

/// @brief Delete EthernetW5500_t object and set ptr to NULL
/// @param Ptr Pointer to the pointer of the management object
/// @return ReturnCode_t* Returns NULL
ReturnCode_t W5500_Delete(EthernetW5500_t** Ptr) {
    if (Ptr != NULL && *Ptr != NULL) {
        /* release hardware resources [cite: 124] */
        spi_bus_remove_device(w5500_spi_handle);
        free(*Ptr);
        *Ptr = NULL;
    }
    return STAT_OKE;
}

/// @brief Pull SCS pin to down then send the 3 bytes of header
/// @param Ptr Pointer to management object
/// @param Header W5500 SPI header
/// @return ReturnCode_t STAT_OKE
ReturnCode_t W5500_SendHeader(EthernetW5500_t* Ptr, W5500Header_t Header) {
    EthernetW5500_t* obj = (EthernetW5500_t*)Ptr;
    if (obj == NULL) return STAT_ERR_NULL;

    /* begin transaction: pull SCS low  */
    gpio_set_level(obj->Pinout.SCS, 0);

    spi_transaction_t t = {
        .length = 8 * 3,
        .tx_buffer = &Header
    };
    /* synchronous polling transmit */
    spi_device_polling_transmit(w5500_spi_handle, &t);
    return STAT_OKE;
}

/// @brief Send N bytes of payload; Set SCS to HIGH
/// @param Ptr Pointer to management object
/// @param Payload Data buffer
/// @param PayloadLength Byte count
/// @return ReturnCode_t STAT_OKE
ReturnCode_t W5500_SendPayload(EthernetW5500_t* Ptr, void* Payload, uint16_t PayloadLength) {
    EthernetW5500_t* obj = (EthernetW5500_t*)Ptr;
    if (obj == NULL || Payload == NULL) return STAT_ERR_NULL;

    spi_transaction_t t = {
        .length = 8 * PayloadLength,
        .tx_buffer = Payload
    };
    /* transmit data phase [cite: 35] */
    spi_device_polling_transmit(w5500_spi_handle, &t);
    
    /* end transaction: pull SCS high [cite: 17] */
    gpio_set_level(obj->Pinout.SCS, 1);
    return STAT_OKE;
}

/// @brief Send header followed by 1-4 byte of payload
/// @param Ptr Pointer to management object
/// @param Header W5500 header with FDM bits
/// @param Payload Data buffer
/// @return ReturnCode_t Result status
ReturnCode_t W5500_SendCommand(EthernetW5500_t* Ptr, W5500Header_t Header, void* Payload) {
    uint16_t len = 0;
    /* determine length from OpMode bits */
    if (Header.OpMode == eW5500_FDM1) len = 1;
    else if (Header.OpMode == eW5500_FDM2) len = 2;
    else if (Header.OpMode == eW5500_FDM4) len = 4;
    else return STAT_ERR_INVALID_ARG;

    W5500_SendHeader(Ptr, Header);
    return W5500_SendPayload(Ptr, Payload, len);
}

/// @brief Send header followed by N bytes of variable payload
/// @param Ptr Pointer to management object
/// @param Header W5500 header with VDM bits
/// @param Payload Data buffer
/// @param PayloadLength Byte count
/// @return ReturnCode_t Result status
ReturnCode_t W5500_SendData(EthernetW5500_t* Ptr, W5500Header_t Header, void* Payload, uint16_t PayloadLength) {
    /* ensure Write bit is set correctly */
    Header.nRW = eW5500_Write;
    W5500_SendHeader(Ptr, Header);
    return W5500_SendPayload(Ptr, Payload, PayloadLength);
}

/// @brief Read data from W5500
/// @param Ptr Pointer to management object
/// @param Header W5500 header with Read bit
/// @param Payload RX buffer
/// @param PayloadLength Byte count
/// @return ReturnCode_t Result status
ReturnCode_t W5500_ReceiveData(EthernetW5500_t* Ptr, W5500Header_t Header, void* Payload, uint16_t PayloadLength) {
    EthernetW5500_t* obj = (EthernetW5500_t*)Ptr;
    if (obj == NULL || Payload == NULL) return STAT_ERR_NULL;

    /* ensure Read bit is set [cite: 35] */
    Header.nRW = eW5500_Read;
    W5500_SendHeader(Ptr, Header);

    spi_transaction_t t = {
        .length = 8 * PayloadLength,
        .rxlength = 8 * PayloadLength,
        .rx_buffer = Payload
    };
    /* receive data phase [cite: 35, 45] */
    spi_device_polling_transmit(w5500_spi_handle, &t);
    
    gpio_set_level(obj->Pinout.SCS, 1);
    return STAT_OKE;
}

/* NEW FUNCTION **********************************************************************************************************/

ReturnCode_t W5500_SetHeader(EthernetW5500_t* Ptr, uint8_t BlockSelBits, uint16_t RegAddr){
    if (IsNull(Ptr)) return STAT_ERR_NULL;
    Ptr->TxFrame.Header.AddrH       = (RegAddr & 0xFF00)>>8;
    Ptr->TxFrame.Header.AddrL       = (RegAddr & 0x00FF)>>0;
    Ptr->TxFrame.Header.BlockSel    = BlockSelBits  & 0x1F;
    Ptr->TxFrame.Header.OpMode      = eW5500_VDM;
    Ptr->TxFrame.Header.nRW         = eW5500_Read;
    /// SysLog("W5500_SetHeader(...) : Header=%02X-%02X-%02X", Ptr->TxFrame.Byte[0], Ptr->TxFrame.Byte[1], Ptr->TxFrame.Byte[2]);
    return STAT_OKE;
}

ReturnCode_t W5500_SetTxPayload(EthernetW5500_t* Ptr, void* Payload, int32_t N){
    if (IsNull(Ptr) || IsNull(Payload)) return STAT_ERR_NULL;
    /*Adjust for size*/
    N = (N < 0) ? 0 : (N > 1500) ? 1500 : N;
    Ptr->TxFrame.Payload.Length = N;
    /*Perform transfer*/
    memcpy(Ptr->TxFrame.Payload.Byte, Payload, N);
    return STAT_OKE;
}

ReturnCode_t W5500_GetTxPayload(EthernetW5500_t* Ptr, void* Payload, int32_t N){
    if (IsNull(Ptr) || IsNull(Payload)) return STAT_ERR_NULL;
    /*Adjust for size*/
    N = (N < 0) ? 0 : (N > 1500) ? 1500 : N;
    /*Perform transfer*/
    memcpy(Payload, Ptr->TxFrame.Payload.Byte, N);
    return STAT_OKE;
}

ReturnCode_t W5500_SetRxPayload(EthernetW5500_t* Ptr, void* Payload, int32_t N){
    if (IsNull(Ptr) || IsNull(Payload)) return STAT_ERR_NULL;
    /*Adjust for size*/
    N = (N < 0) ? 0 : (N > 1500) ? 1500 : N;
    Ptr->RxFrame.Payload.Length = N;
    /*Perform transfer*/
    memcpy(Ptr->RxFrame.Payload.Byte, Payload, N);
    return STAT_OKE;
}

ReturnCode_t W5500_GetRxPayload(EthernetW5500_t* Ptr, void* Payload, int32_t N){
    if (IsNull(Ptr) || IsNull(Payload)) return STAT_ERR_NULL;
    /*Adjust for size*/
    N = (N < 0) ? 0 : (N > 1500) ? 1500 : N;
    /*Perform transfer*/
    memcpy(Payload, Ptr->RxFrame.Payload.Byte, N);
    return STAT_OKE;
}

ReturnCode_t W5500_AccessNByte(EthernetW5500_t* Ptr, uint8_t nRW) {
    int32_t N;
    if (IsNull(Ptr)) return STAT_ERR_NULL;

    /* Determine transaction length based on the operation type */
    switch (nRW) {
        case eW5500_Read:
            Ptr->TxFrame.Header.nRW = eW5500_Read;
            N = Ptr->RxFrame.Payload.Length;
            break;
        case eW5500_Write:
            Ptr->TxFrame.Header.nRW = eW5500_Write;
            N = Ptr->TxFrame.Payload.Length;
            break;
        default:
            return STAT_ERR_INVALID_ARG;
    }

    // SysLog("W5500_AccessNByte(...): # payload bytes: %d", N);

    /* Validate and clamp N based on the operational mode */
    switch (Ptr->TxFrame.Header.OpMode) {
        case eW5500_VDM:  N = (N < 1) ? 1 : (N > 1500) ? 1500 : N; break;
        case eW5500_FDM1: N = 1; break;
        case eW5500_FDM2: N = 2; break;
        case eW5500_FDM4: N = 4; break;
    }
    
    // SysLog("W5500_AccessNByte(...): # payload bytes (after adjusted): %d", N);
    // SysLog("W5500_AccessNByte(...): Header: %02X-%02X-%02X", Ptr->TxFrame.Byte[0], Ptr->TxFrame.Byte[1], Ptr->TxFrame.Byte[2]);
    
    /* Synchronize frame lengths and prepare buffers */
    Ptr->TxFrame.Payload.Length = N;
    Ptr->RxFrame.Payload.Length = N;
    memset(Ptr->RxFrame.Byte, 0, 3 + N);
    
    /* Only clear TX payload if we are in READ mode to avoid sending garbage */
    if (nRW == eW5500_Read) {
        memset(Ptr->TxFrame.Payload.Byte, 0, N);
    }

    // SysLog("W5500_AccessNByte(...): # TX/RX  Bits: %d", 8 * (N + 3));

    /* Execute the SPI transaction with manual CS control */
    spi_transaction_t t = {
        .length = 8 * (N + 3),
        .tx_buffer = Ptr->TxFrame.Byte,
        .rx_buffer = Ptr->RxFrame.Byte
    };

    gpio_set_level(Ptr->Pinout.SCS, 0);
    spi_device_polling_transmit(w5500_spi_handle, &t); 
    gpio_set_level(Ptr->Pinout.SCS, 1);
    // esp_rom_delay_us(100);

    return STAT_OKE;
}

ReturnCode_t W5500_ReadNByte(EthernetW5500_t* Ptr, uint8_t* Buffer, uint16_t Len) {
    if (IsNull(Ptr) || IsNull(Buffer)) return STAT_ERR_NULL;

    /* Configure for Variable Data Length Mode and execute transaction */
    Ptr->TxFrame.Header.OpMode = eW5500_VDM;
    Ptr->RxFrame.Payload.Length = Len;
    if (W5500_AccessNByte(Ptr, eW5500_Read) != STAT_OKE) {
        return STAT_ERR_IO;
    }

    /* Copy received data from internal payload to user buffer */
    memcpy(Buffer, Ptr->RxFrame.Payload.Byte, Len);
    return STAT_OKE;
}

ReturnCode_t W5500_ReadByte(EthernetW5500_t* Ptr) {
    if (IsNull(Ptr)) return STAT_ERR_NULL;

    /* Override operational mode for single byte access */
    Ptr->TxFrame.Header.OpMode = eW5500_FDM1;
    Ptr->RxFrame.Payload.Length = 1;
    
    /* Execute transaction and return the first byte */
    if (W5500_AccessNByte(Ptr, eW5500_Read) != STAT_OKE) return 0;
    return (ReturnCode_t)Ptr->RxFrame.Payload.Byte[0];
}

ReturnCode_t W5500_ReadDoubleByte(EthernetW5500_t* Ptr) {
    if (IsNull(Ptr)) return STAT_ERR_NULL;

    /* Set mode to 2-byte fixed data length */
    Ptr->TxFrame.Header.OpMode = eW5500_FDM2;
    Ptr->RxFrame.Payload.Length = 2;
    
    /* Execute transaction and assemble 16-bit result */
    if (W5500_AccessNByte(Ptr, eW5500_Read) != STAT_OKE) return 0;
    return (ReturnCode_t)((Ptr->RxFrame.Payload.Byte[0] << 8) | Ptr->RxFrame.Payload.Byte[1]);
}

ReturnCode_t W5500_ReadQuartByte(EthernetW5500_t* Ptr) {
    if (IsNull(Ptr)) return STAT_ERR_NULL;

    /* Set mode to 4-byte fixed data length */
    Ptr->TxFrame.Header.OpMode = eW5500_FDM4;
    Ptr->RxFrame.Payload.Length = 4;
    
    /* Execute transaction and assemble 32-bit result */
    if (W5500_AccessNByte(Ptr, eW5500_Read) != STAT_OKE) return 0;
    return (ReturnCode_t)((Ptr->RxFrame.Payload.Byte[0] << 24) | (Ptr->RxFrame.Payload.Byte[1] << 16) | 
                          (Ptr->RxFrame.Payload.Byte[2] << 8)  |  Ptr->RxFrame.Payload.Byte[3]);
}

ReturnCode_t W5500_WriteNByte(EthernetW5500_t* Ptr, uint8_t* Data, uint16_t Len) {
    if (IsNull(Ptr) || IsNull(Data)) return STAT_ERR_NULL;

    /* Copy data to internal buffer and set VDM mode */
    W5500_SetTxPayload(Ptr, Data, Len);
    Ptr->TxFrame.Header.OpMode = eW5500_VDM;

    /* Execute SPI write transaction */
    return W5500_AccessNByte(Ptr, eW5500_Write);
}

ReturnCode_t W5500_WriteByte(EthernetW5500_t* Ptr, uint8_t Data) {
    if (IsNull(Ptr)) return STAT_ERR_NULL;

    /* Place the actual comment here */
    Ptr->TxFrame.Header.OpMode = eW5500_FDM1;
    Ptr->TxFrame.Payload.Byte[0] = Data;
    Ptr->TxFrame.Payload.Length = 1;
    
    return W5500_AccessNByte(Ptr, eW5500_Write);
}

ReturnCode_t W5500_WriteDoubleByte(EthernetW5500_t* Ptr, uint16_t Data) {
    if (IsNull(Ptr)) return STAT_ERR_NULL;

    /* Store data in Big-Endian format before transmission */
    Ptr->TxFrame.Header.OpMode = eW5500_FDM2;
    Ptr->TxFrame.Payload.Byte[0] = (uint8_t)(Data >> 8);
    Ptr->TxFrame.Payload.Byte[1] = (uint8_t)(Data & 0xFF);
    Ptr->TxFrame.Payload.Length = 2;
    
    return W5500_AccessNByte(Ptr, eW5500_Write);
}

ReturnCode_t W5500_WriteQuartByte(EthernetW5500_t* Ptr, uint32_t Data) {
    if (IsNull(Ptr)) return STAT_ERR_NULL;

    /* Store 32-bit data in Big-Endian format */
    Ptr->TxFrame.Header.OpMode = eW5500_FDM4;
    Ptr->TxFrame.Payload.Byte[0] = (uint8_t)(Data >> 24);
    Ptr->TxFrame.Payload.Byte[1] = (uint8_t)(Data >> 16);
    Ptr->TxFrame.Payload.Byte[2] = (uint8_t)(Data >> 8);
    Ptr->TxFrame.Payload.Byte[3] = (uint8_t)(Data & 0xFF);
    Ptr->TxFrame.Payload.Length = 4;
    
    return W5500_AccessNByte(Ptr, eW5500_Write);
}

/* TEST ******************************************************************************************************************/

/// @brief Perform a true 5-byte loopback test (Header + Data)
/// @param tx_data Pointer to 2 bytes of test data
/// @param rx_full_frame Pointer to a 5-byte buffer to store the result
ReturnCode_t W5500_LoopbackTest(EthernetW5500_t* Ptr, uint8_t tx_data[2], uint8_t rx_full_frame[5]) {
    EthernetW5500_t* obj = (EthernetW5500_t*)Ptr;
    if (obj == NULL) return STAT_ERR_NULL;

    /* Prepare a single 5-byte buffer for the entire transaction [cite: 40] */
    uint8_t tx_buffer[5];
    
    /* 3 bytes Header (Assume reading VersionReg 0x0039) */
    tx_buffer[0] = 0x00;
    tx_buffer[1] = 0x39;
    tx_buffer[2] = 0x02; /* Block 0, Read, FDM2 */
    
    /* 2 bytes Payload */
    tx_buffer[3] = tx_data[0];
    tx_buffer[4] = tx_data[1];

    spi_transaction_t t = {
        .length = 8 * 5,          /* 5 bytes = 40 bits [cite: 44, 915] */
        .tx_buffer = tx_buffer,   /* Data to send [cite: 926] */
        .rx_buffer = rx_full_frame /* Buffer to catch the echo [cite: 930] */
    };

    /* Execute a single continuous transaction with manual CS control */
    gpio_set_level(obj->Pinout.SCS, 0);
    spi_device_polling_transmit(w5500_spi_handle, &t); /* [cite: 122, 760] */
    gpio_set_level(obj->Pinout.SCS, 1);

    SysLog("[Loopback] RX Full: %02X %02X %02X %02X %02X", 
       rx_full_frame[0], rx_full_frame[1], rx_full_frame[2], 
       rx_full_frame[3], rx_full_frame[4]);

    return STAT_OKE;
}

/* UTILS *****************************************************************************************************************/

/// @brief Convert a 4-byte array to an IPv4 string
/// @param IPv4Addr Input array of 4 bytes
/// @param IPv4Str Output buffer (minimum 16 bytes)
/// @return ReturnCode_t STAT_OKE if successful
ReturnCode_t ConvertByteArr2IPvToAddress(uint8_t IPv4Addr[], char IPv4Str[]) {
    if (IPv4Addr == NULL || IPv4Str == NULL) {
        return STAT_ERR_NULL;
    }

    /* this is a comment - format bytes into string x.x.x.x */
    sprintf(IPv4Str, "%u.%u.%u.%u", IPv4Addr[0], IPv4Addr[1], IPv4Addr[2], IPv4Addr[3]);

    return STAT_OKE;
}

/// @brief Convert an IPv4 string to a 4-byte array
/// @param IPv4Str Input dotted-decimal string
/// @param IPv4Addr Output array of 4 bytes
/// @return ReturnCode_t STAT_OKE if successful
ReturnCode_t ConvertIPv4AddressToByteArr(char IPv4Str[], uint8_t IPv4Addr[]) {
    if (IPv4Str == NULL || IPv4Addr == NULL) {
        return STAT_ERR_NULL;
    }

    int ip[4];
    /* this is a comment - parse and validate 4 segments */
    if (sscanf(IPv4Str, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) != 4) {
        return STAT_ERR_INVALID_ARG;
    }

    /* assign results with casting */
    for (int i = 0; i < 4; i++) {
        IPv4Addr[i] = (uint8_t)ip[i];
    }

    return STAT_OKE;
}

/// @brief Convert an IPv4 string to a 64-bit integer
/// @param IPv4Str Input dotted-decimal string
/// @param IPv4Addr Output pointer to uint64_t
/// @return ReturnCode_t STAT_OKE if successful
ReturnCode_t ConvertIPv4AddressToUInt64(char IPv4Str[], uint64_t *IPv4Addr) {
    if (IPv4Str == NULL || IPv4Addr == NULL) {
        return STAT_ERR_NULL;
    }

    int ip[4];
    /* parse string to temporary integers */
    if (sscanf(IPv4Str, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) != 4) {
        return STAT_ERR_INVALID_ARG;
    }

    /* this is a comment - shift and combine into 64-bit (Network Byte Order) */
    *IPv4Addr = ((uint64_t)(uint8_t)ip[0] << 24) | 
                ((uint64_t)(uint8_t)ip[1] << 16) | 
                ((uint64_t)(uint8_t)ip[2] << 8)  | 
                ((uint64_t)(uint8_t)ip[3]);

    return STAT_OKE;
}

/// @brief Convert a 64-bit integer to an IPv4 string
/// @param IPv4Addr Input 64-bit integer representing IP
/// @param IPv4Str Output buffer (minimum 16 bytes)
/// @return ReturnCode_t STAT_OKE if successful
ReturnCode_t ConvertUInt64ToIPv4Address(uint64_t IPv4Addr, char IPv4Str[]) {
    if (IPv4Str == NULL) {
        return STAT_ERR_NULL;
    }

    /* extract bytes from integer using masking and shifting */
    uint8_t b0 = (uint8_t)((IPv4Addr >> 24) & 0xFF);
    uint8_t b1 = (uint8_t)((IPv4Addr >> 16) & 0xFF);
    uint8_t b2 = (uint8_t)((IPv4Addr >> 8)  & 0xFF);
    uint8_t b3 = (uint8_t)(IPv4Addr & 0xFF);

    /* this is a comment - write bytes to string buffer */
    sprintf(IPv4Str, "%u.%u.%u.%u", b0, b1, b2, b3);

    return STAT_OKE;
}

/// @brief Convert an IPv4 string to a 32-bit integer
/// @param IPv4Str Input dotted-decimal string
/// @param IPv4Addr Output pointer to uint32_t
/// @return ReturnCode_t STAT_OKE if successful
ReturnCode_t ConvertIPv4AddressToUInt32(char IPv4Str[], uint32_t *IPv4Addr) {
    if (IPv4Str == NULL || IPv4Addr == NULL) {
        return STAT_ERR_NULL;
    }

    int ip[4];
    /* parse and validate 4 segments from input string [cite: 35, 46] */
    if (sscanf(IPv4Str, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) != 4) {
        return STAT_ERR_INVALID_ARG;
    }

    /* combine octets into 32-bit value in Network Byte Order */
    *IPv4Addr = ((uint32_t)(uint8_t)ip[0] << 24) | 
                ((uint32_t)(uint8_t)ip[1] << 16) | 
                ((uint32_t)(uint8_t)ip[2] << 8)  | 
                ((uint32_t)(uint8_t)ip[3]);

    return STAT_OKE;
}

/// @brief Convert a 32-bit integer to an IPv4 string
/// @param IPv4Addr Input 32-bit integer representing IP
/// @param IPv4Str Output buffer (minimum 16 bytes)
/// @return ReturnCode_t STAT_OKE if successful
ReturnCode_t ConvertUInt32ToIPv4Address(uint32_t IPv4Addr, char IPv4Str[]) {
    if (IPv4Str == NULL) {
        return STAT_ERR_NULL;
    }

    /* extract bytes via bit shifting to handle endianness [cite: 141, 142] */
    uint8_t b0 = (uint8_t)((IPv4Addr >> 24) & 0xFF);
    uint8_t b1 = (uint8_t)((IPv4Addr >> 16) & 0xFF);
    uint8_t b2 = (uint8_t)((IPv4Addr >> 8)  & 0xFF);
    uint8_t b3 = (uint8_t)(IPv4Addr & 0xFF);

    /* format extracted bytes into dotted-decimal string */
    sprintf(IPv4Str, "%u.%u.%u.%u", b0, b1, b2, b3);

    return STAT_OKE;
}

/// @brief Convert IPv4 octets to a 32-bit integer in Big-endian (Network) order
/// @param Octet0 First octet (e.g., 192)
/// @param Octet1 Second octet (e.g., 168)
/// @param Octet2 Third octet (e.g., 1)
/// @param Octet3 Fourth octet (e.g., 10)
/// @return uint32_t The packed 32-bit IP address
uint32_t IPv4ToUint32(uint8_t Octet0, uint8_t Octet1, uint8_t Octet2, uint8_t Octet3) {
    /* Shift octets into a 32-bit container in Big-endian order */
    return ((uint32_t)Octet0 << 24) | 
           ((uint32_t)Octet1 << 16) | 
           ((uint32_t)Octet2 << 8)  | 
           ((uint32_t)Octet3);
}

/// @brief Convert 6 MAC address octets to a 64-bit integer
/// @param Octet0 High byte of MAC (Most Significant Byte)
/// @param Octet1 Second byte
/// @param Octet2 Third byte
/// @param Octet3 Fourth byte
/// @param Octet4 Fifth byte
/// @param Octet5 Low byte of MAC (Least Significant Byte)
/// @return uint64_t The packed MAC address
uint64_t MACToUint64(uint8_t Octet0, uint8_t Octet1, uint8_t Octet2, uint8_t Octet3, uint8_t Octet4, uint8_t Octet5) {
    /* Pack 48-bit MAC address into a 64-bit container in Big-endian order */
    return ((uint64_t)Octet0 << 40) |
           ((uint64_t)Octet1 << 32) |
           ((uint64_t)Octet2 << 24) |
           ((uint64_t)Octet3 << 16) |
           ((uint64_t)Octet4 << 8)  |
           ((uint64_t)Octet5);
}

/* EOF *******************************************************************************************************************/
