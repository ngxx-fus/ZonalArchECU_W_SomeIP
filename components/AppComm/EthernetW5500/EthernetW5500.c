#include "./EthernetW5500.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include <string.h>
#include <stdio.h>

/// #define READ_AFTER_WRITE_LOG_EN 1

#ifndef READ_AFTER_WRITE_LOG_EN
    #define READ_AFTER_WRITE_LOG_EN 0
#endif /*READ_AFTER_WRITE_LOG_EN*/

/* STATIC VARIABLES *************************************************************************************************************/

/* Static handler for the W5500 semaphore*/
SemaphoreHandle_t    W5500_Lock;

/* Static handler for the SPI device instance */
static spi_device_handle_t w5500_spi_handle;

/* W5500 Isr Tracking task function pointer */
void (*W5500_TaskComming_Function)(void*);

/* W5500 Isr Tracking task handle*/
TaskHandle_t W5500_TaskComm_TaskHandler;

/// @brief Handle incoming interrupts from W5500 for MACRAW and ICMP with semaphore protection
/// @param arg Pointer to the EthernetW5500_t structure
/// @return void
static void IRAM_ATTR W5500_IsrHandler(void* arg) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(W5500_TaskComm_TaskHandler, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

/* PUBLIC FUNCTION IMPLEMENTATION ***********************************************************************************************/

ReturnCode_t W5500_InitLock(void) {
    W5500_Lock = xSemaphoreCreateBinary();
    
    if (W5500_Lock == NULL) {
        return STAT_ERR;
    }

    /* Binary semaphores start at 0 (locked); give once to make it available */
    xSemaphoreGive(W5500_Lock);
    
    return STAT_OKE;
}

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
    if (gpio_config(&io_conf) != ESP_OK) {
        free(obj);
        return NULL;
    }
    gpio_set_level(obj->Pinout.SCS, 1);
    gpio_set_level(obj->Pinout.RST, 1);

    /* setup GPIO for Input Interrupt pin (INT) */
    gpio_config_t in_conf = {
        .intr_type = GPIO_INTR_NEGEDGE, /* W5500 INT is active low */
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << obj->Pinout.INT),
        .pull_up_en = 1, .pull_down_en = 0
    };
    if (gpio_config(&in_conf) != ESP_OK) {
        free(obj);
        return NULL;
    }
    gpio_pullup_en(obj->Pinout.INT);
    gpio_pulldown_dis(obj->Pinout.INT);

    /* configure SPI bus */
    spi_bus_config_t bus_cfg = {
        .miso_io_num = MISO, .mosi_io_num = MOSI, .sclk_io_num = CLK,
        .quadwp_io_num = -1, .quadhd_io_num = -1,
        .max_transfer_sz = 1600
    };
    spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);

    /* add W5500 to the bus with manual CS */
    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = 20 * 1000 * 1000,
        .mode = 0, .spics_io_num = -1,
        .queue_size = 7
    };
    spi_bus_add_device(SPI2_HOST, &dev_cfg, &w5500_spi_handle);
    /* install ISR service and add handler for W5500 INT pin */
    esp_err_t isr_res = gpio_install_isr_service(0);
    if (isr_res != ESP_OK && isr_res != ESP_ERR_INVALID_STATE) {
        free(obj);
        return NULL;
    }
    if (gpio_isr_handler_add(obj->Pinout.INT, W5500_IsrHandler, (void*) obj) != ESP_OK) {
        free(obj);
        return NULL;
    }

    /// W5500_Lock = xSemaphoreCreateBinary();
    W5500_InitLock();

    return (EthernetW5500_t*) obj;
}

ReturnCode_t W5500_Delete(EthernetW5500_t** Ptr) {
    if (Ptr != NULL && *Ptr != NULL) {
        /* release hardware resources [cite: 124] */
        spi_bus_remove_device(w5500_spi_handle);
        free(*Ptr);
        *Ptr = NULL;
    }
    return STAT_OKE;
}

ReturnCode_t W5500_IsInInterruptPhase(EthernetW5500_t* Ptr) {
    /* Validate input pointer */
    if (IsNull(Ptr)) {
        return STAT_ERR_NULL;
    }

    /* Read physical level of the INT pin; W5500 pulls this pin LOW when an interrupt occurs [cite: 1684, 2431] */
    if (gpio_get_level(Ptr->Pinout.INT) == 0) {
        return STAT_OKE;
    }

    return STAT_ERR;
}

ReturnCode_t W5500_SetHeader(EthernetW5500_t* Ptr, Byte_t BlockSelBits, Word_t RegAddr){
    if (IsNull(Ptr)) return STAT_ERR_NULL;
    Ptr->TxFrame.Header.AddrH       = (RegAddr & 0xFF00)>>8;
    Ptr->TxFrame.Header.AddrL       = (RegAddr & 0x00FF)>>0;
    Ptr->TxFrame.Header.BlockSel    = BlockSelBits  & 0x1F;
    Ptr->TxFrame.Header.OpMode      = eW5500_VDM;
    Ptr->TxFrame.Header.nRW         = eW5500_Read;
    /// SysLog("W5500_SetHeader(...) : Header=%02X-%02X-%02X", Ptr->TxFrame.Byte[0], Ptr->TxFrame.Byte[1], Ptr->TxFrame.Byte[2]);
    return STAT_OKE;
}

ReturnCode_t W5500_SetTxPayload(EthernetW5500_t* Ptr, void* Payload, Dword_t N){
    if (IsNull(Ptr) || IsNull(Payload)) return STAT_ERR_NULL;
    /*Adjust for size*/
    N = /*(N < 0) ? 0 :*/ (N > 1500) ? 1500 : N;
    Ptr->TxFrame.Payload.Length = N;
    /*Perform transfer*/
    memcpy(Ptr->TxFrame.Payload.Byte, Payload, N);
    return STAT_OKE;
}

ReturnCode_t W5500_GetTxPayload(EthernetW5500_t* Ptr, void* Payload, Dword_t N){
    if (IsNull(Ptr) || IsNull(Payload)) return STAT_ERR_NULL;
    /*Adjust for size*/
    N = /*(N < 0) ? 0 :*/ (N > 1500) ? 1500 : N;
    /*Perform transfer*/
    memcpy(Payload, Ptr->TxFrame.Payload.Byte, N);
    return STAT_OKE;
}

ReturnCode_t W5500_SetRxPayload(EthernetW5500_t* Ptr, void* Payload, Dword_t N){
    if (IsNull(Ptr) || IsNull(Payload)) return STAT_ERR_NULL;
    /*Adjust for size*/
    N = /*(N < 0) ? 0 :*/ (N > 1500) ? 1500 : N;
    Ptr->RxFrame.Payload.Length = N;
    /*Perform transfer*/
    memcpy(Ptr->RxFrame.Payload.Byte, Payload, N);
    return STAT_OKE;
}

ReturnCode_t W5500_GetRxPayload(EthernetW5500_t* Ptr, void* Payload, Dword_t N){
    if (IsNull(Ptr) || IsNull(Payload)) return STAT_ERR_NULL;
    /*Adjust for size*/
    N = /*(N < 0) ? 0 :*/ (N > 1500) ? 1500 : N;
    /*Perform transfer*/
    memcpy(Payload, Ptr->RxFrame.Payload.Byte, N);
    return STAT_OKE;
}

ReturnCode_t W5500_AccessNByte(EthernetW5500_t* Ptr, Byte_t nRW) {
    Dword_t N;
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

ReturnCode_t W5500_ReadNByte(EthernetW5500_t* Ptr, Byte_t* Buffer, Word_t Len) {
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

ReturnCode_t W5500_WriteNByte(EthernetW5500_t* Ptr, Byte_t* Data, Word_t Len) {
    if (IsNull(Ptr) || IsNull(Data)) return STAT_ERR_NULL;

    /* Copy data to internal buffer and set VDM mode */
    W5500_SetTxPayload(Ptr, Data, Len);
    Ptr->TxFrame.Header.OpMode = eW5500_VDM;

    /* Execute SPI write transaction */
    return W5500_AccessNByte(Ptr, eW5500_Write);
}

ReturnCode_t W5500_WriteByte(EthernetW5500_t* Ptr, Byte_t Data) {
    if (IsNull(Ptr)) return STAT_ERR_NULL;


    Ptr->TxFrame.Header.OpMode = eW5500_FDM1;
    Ptr->TxFrame.Payload.Byte[0] = Data;
    Ptr->TxFrame.Payload.Length = 1;
    
    return W5500_AccessNByte(Ptr, eW5500_Write);
}

ReturnCode_t W5500_WriteDoubleByte(EthernetW5500_t* Ptr, Word_t Data) {
    if (IsNull(Ptr)) return STAT_ERR_NULL;

    /* Store data in Big-Endian format before transmission */
    Ptr->TxFrame.Header.OpMode = eW5500_FDM2;
    Ptr->TxFrame.Payload.Byte[0] = (Byte_t)(Data >> 8);
    Ptr->TxFrame.Payload.Byte[1] = (Byte_t)(Data & 0xFF);
    Ptr->TxFrame.Payload.Length = 2;
    
    return W5500_AccessNByte(Ptr, eW5500_Write);
}

ReturnCode_t W5500_WriteQuartByte(EthernetW5500_t* Ptr, uint32_t Data) {
    if (IsNull(Ptr)) return STAT_ERR_NULL;

    /* Store 32-bit data in Big-Endian format */
    Ptr->TxFrame.Header.OpMode = eW5500_FDM4;
    Ptr->TxFrame.Payload.Byte[0] = (Byte_t)(Data >> 24);
    Ptr->TxFrame.Payload.Byte[1] = (Byte_t)(Data >> 16);
    Ptr->TxFrame.Payload.Byte[2] = (Byte_t)(Data >> 8);
    Ptr->TxFrame.Payload.Byte[3] = (Byte_t)(Data & 0xFF);
    Ptr->TxFrame.Payload.Length = 4;
    
    return W5500_AccessNByte(Ptr, eW5500_Write);
}

/* TEST *************************************************************************************************************************/

ReturnCode_t W5500_LoopbackTest(EthernetW5500_t* Ptr, Byte_t tx_data[2], Byte_t rx_full_frame[5]) {
    EthernetW5500_t* obj = (EthernetW5500_t*)Ptr;
    if (obj == NULL) return STAT_ERR_NULL;

    /* Prepare a single 5-byte buffer for the entire transaction [cite: 40] */
    Byte_t tx_buffer[5];
    
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

ReturnCode_t W5500_GetModuleVersion(EthernetW5500_t* Ptr) {
    EthernetW5500_t* obj = (EthernetW5500_t*)Ptr;
    if (obj == NULL) return STAT_ERR_NULL;

    /* Prepare 4-byte buffer for Read transaction (3 Header + 1 Data) */
    Byte_t tx_buf[4] = {0x00, 0x39, 0x01, 0x00};
    Byte_t rx_buf[4] = {0};

    spi_transaction_t t = {
        .length = 8 * 4,
        .tx_buffer = tx_buf,
        .rx_buffer = rx_buf
    };

    /* Execute continuous SPI transaction to fetch version byte */
    gpio_set_level(obj->Pinout.SCS, 0);
    spi_device_polling_transmit(w5500_spi_handle, &t);
    gpio_set_level(obj->Pinout.SCS, 1);

    /// SysLog("W5500_GetModuleVersion(...): Hardware Version: 0x%02X", rx_buf[3]);

    return rx_buf[3];
}

/* COMPOSE FN *******************************************************************************************************************/


ReturnCode_t W5500_ReadByteReg(EthernetW5500_t* Ptr, Word_t BLockSelNum, Word_t RegAddr) {
    ReturnCode_t result = STAT_ERR_NULL;

    /* Validate pointer */
    if (Ptr == NULL) {
        return STAT_ERR_NULL;
    }

    /* Acquire the hardware lock to protect SPI transaction */
    if (xSemaphoreTake(W5500_Lock, portMAX_DELAY) == pdTRUE) {
        W5500_SetHeader(Ptr, BLockSelNum, RegAddr);
        result = W5500_ReadByte(Ptr);

        #if (READ_AFTER_WRITE_LOG_EN == 1)
        SysLog("W5500_ReadByteReg(...): %04X=%02X", RegAddr, (Byte_t)result);
        #endif

        /* Release the hardware lock */
        xSemaphoreGive(W5500_Lock);
    }
    
    return result;
}

ReturnCode_t W5500_WriteByteReg(EthernetW5500_t* Ptr, Word_t BLockSelNum, Word_t RegAddr, Byte_t ByteValue) {
    ReturnCode_t result = STAT_ERR_NULL;

    /* Validate pointer */
    if (Ptr == NULL) {
        return STAT_ERR_NULL;
    }

    /* Acquire the hardware lock to protect SPI transaction */
    if (xSemaphoreTake(W5500_Lock, portMAX_DELAY) == pdTRUE) {
        W5500_SetHeader(Ptr, BLockSelNum, RegAddr);
        result = W5500_WriteByte(Ptr, ByteValue);

        #if (READ_AFTER_WRITE_LOG_EN == 1)
        /* Set header again for reading to verify */
        W5500_SetHeader(Ptr, BLockSelNum, RegAddr);
        SysLog("W5500_WriteByteReg(...): Write %04X=%02X Result: %02X", RegAddr, ByteValue, (Byte_t)W5500_ReadByte(Ptr));
        #endif

        /* Release the hardware lock */
        xSemaphoreGive(W5500_Lock);
    }
    
    return result;
}

ReturnCode_t W5500_ReadDoubleByteReg(EthernetW5500_t* Ptr, Word_t BLockSelNum, Word_t RegAddr) {
    ReturnCode_t result = STAT_ERR_NULL;

    /* Validate pointer */
    if (Ptr == NULL) {
        return STAT_ERR_NULL;
    }

    /* Acquire the hardware lock to protect SPI transaction */
    if (xSemaphoreTake(W5500_Lock, portMAX_DELAY) == pdTRUE) {
        /* Set header and execute 16-bit read */
        W5500_SetHeader(Ptr, BLockSelNum, RegAddr);
        result = W5500_ReadDoubleByte(Ptr);

        #if (READ_AFTER_WRITE_LOG_EN == 1)
        SysLog("W5500_ReadDoubleByteReg(...): %04X=%04X", RegAddr, (Word_t)result);
        #endif

        /* Release the hardware lock */
        xSemaphoreGive(W5500_Lock);
    }
    
    return result;
}

ReturnCode_t W5500_WriteDoubleByteReg(EthernetW5500_t* Ptr, Word_t BLockSelNum, Word_t RegAddr, Word_t Data) {
    ReturnCode_t result = STAT_ERR_NULL;

    /* Validate pointer */
    if (Ptr == NULL) {
        return STAT_ERR_NULL;
    }

    /* Acquire the hardware lock to protect SPI transaction */
    if (xSemaphoreTake(W5500_Lock, portMAX_DELAY) == pdTRUE) {
        /* Set header and execute 16-bit write */
        W5500_SetHeader(Ptr, BLockSelNum, RegAddr);
        result = W5500_WriteDoubleByte(Ptr, Data);

        #if (READ_AFTER_WRITE_LOG_EN == 1)
        /* Set header again for reading to verify */
        W5500_SetHeader(Ptr, BLockSelNum, RegAddr);
        SysLog("W5500_WriteDoubleByteReg(...): Write %04X=%04X Result: %04X", RegAddr, Data, (Word_t)W5500_ReadDoubleByte(Ptr));
        #endif

        /* Release the hardware lock */
        xSemaphoreGive(W5500_Lock);
    }
    
    return result;
}

ReturnCode_t W5500_ReadQuartByteReg(EthernetW5500_t* Ptr, Word_t BLockSelNum, Word_t RegAddr) {
    ReturnCode_t result = STAT_ERR_NULL;

    /* Validate pointer */
    if (Ptr == NULL) {
        return STAT_ERR_NULL;
    }

    /* Acquire lock and execute 4-byte SPI read */
    if (xSemaphoreTake(W5500_Lock, portMAX_DELAY) == pdTRUE) {
        W5500_SetHeader(Ptr, (Byte_t)BLockSelNum, RegAddr);
        result = W5500_ReadQuartByte(Ptr);
        
        #if (READ_AFTER_WRITE_LOG_EN == 1)
        SysLog("W5500_ReadQuartByteReg(...): %04X=%08lX", RegAddr, (unsigned long)result);
        #endif

        /* Release the hardware lock */
        xSemaphoreGive(W5500_Lock);
    }
    
    return result;
}

ReturnCode_t W5500_WriteQuartByteReg(EthernetW5500_t* Ptr, Word_t BLockSelNum, Word_t RegAddr, Dword_t Data) {
    ReturnCode_t result = STAT_ERR_NULL;

    /* Validate pointer */
    if (Ptr == NULL) {
        return STAT_ERR_NULL;
    }

    /* Acquire lock and execute 4-byte SPI write */
    if (xSemaphoreTake(W5500_Lock, portMAX_DELAY) == pdTRUE) {
        W5500_SetHeader(Ptr, (Byte_t)BLockSelNum, RegAddr);
        result = W5500_WriteQuartByte(Ptr, Data);

        #if (READ_AFTER_WRITE_LOG_EN == 1)
        /* Set header again for reading to verify */
        W5500_SetHeader(Ptr, (Byte_t)BLockSelNum, RegAddr);
        SysLog("W5500_WriteQuartByteReg(...): Write %04X=%08lX Result: %08lX", RegAddr, (unsigned long)Data, (unsigned long)W5500_ReadQuartByte(Ptr));
        #endif

        /* Release the hardware lock */
        xSemaphoreGive(W5500_Lock);
    }
    
    return result;
}

ReturnCode_t W5500_ReadNByteReg(EthernetW5500_t* Ptr, Word_t BLockSelNum, Word_t RegAddr, Byte_t* Buffer, Word_t Len) {
    ReturnCode_t result = STAT_ERR_NULL;

    /* Validate pointers */
    if (Ptr == NULL || Buffer == NULL) {
        return STAT_ERR_NULL;
    }

    /* Lock SPI bus to read a burst of N bytes */
    if (xSemaphoreTake(W5500_Lock, portMAX_DELAY) == pdTRUE) {
        W5500_SetHeader(Ptr, (Byte_t)BLockSelNum, RegAddr);
        result = W5500_ReadNByte(Ptr, Buffer, Len);
        
        #if (READ_AFTER_WRITE_LOG_EN == 1)
        /* Format hexadecimal string representations (max 6 bytes for log) */
        char rd_str[19] = {0};
        Word_t read_len = (Len > 6) ? 6 : Len;

        for (int32_t i = 0; i < read_len; i++) {
            sprintf(&rd_str[i * 3], "%02X ", Buffer[i]);
        }
        
        SysLog("W5500_ReadNByteReg(...): %04X=[%s]", RegAddr, rd_str);
        #endif

        /* Release the hardware lock */
        xSemaphoreGive(W5500_Lock);
    }
    
    return result;
}

ReturnCode_t W5500_WriteNByteReg(EthernetW5500_t* Ptr, Word_t BLockSelNum, Word_t RegAddr, Byte_t* Data, Word_t Len) {
    ReturnCode_t result = STAT_ERR_NULL;

    /* Validate pointers */
    if (Ptr == NULL || Data == NULL) {
        return STAT_ERR_NULL;
    }

    /* Lock SPI bus to write a burst of N bytes */
    if (xSemaphoreTake(W5500_Lock, portMAX_DELAY) == pdTRUE) {
        W5500_SetHeader(Ptr, (Byte_t)BLockSelNum, RegAddr);
        result = W5500_WriteNByte(Ptr, Data, Len);

        #if (READ_AFTER_WRITE_LOG_EN == 1)
        /* Read back up to 6 bytes for logging */
        Byte_t read_buf[6] = {0};
        char wr_str[19] = {0};
        char rd_str[19] = {0};
        Word_t read_len = (Len > 6) ? 6 : Len;

        W5500_SetHeader(Ptr, (Byte_t)BLockSelNum, RegAddr);
        W5500_ReadNByte(Ptr, read_buf, read_len);

        /* Format hexadecimal string representations */
        for (int32_t i = 0; i < read_len; i++) {
            sprintf(&wr_str[i * 3], "%02X ", Data[i]);
            sprintf(&rd_str[i * 3], "%02X ", read_buf[i]);
        }
        
        SysLog("W5500_WriteNByteReg(...): Write %04X=[%s] Result: [%s]", RegAddr, wr_str, rd_str);
        #endif

        /* Release the hardware lock */
        xSemaphoreGive(W5500_Lock);
    }
    
    return result;
}

/* EOF **************************************************************************************************************************/
