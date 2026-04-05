#include "EthernetW550.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include <string.h>

/* Global SPI handle for the W5500 device */
static spi_device_handle_t W5500_SPI_Handle;

/// @brief Allocate new EthernetW5500_t with preset pin; Init HSPI (SCS is manually controlled)
/// @param MISO Master In Slave Out pin
/// @param MOSI Master Out Slave In pin
/// @param CLK SPI Clock pin
/// @param SCS Slave Chip Select pin
/// @param RST Reset pin
/// @param INT Interrupt pin
/// @return ReturnCode_t* Pointer to the allocated management structure cast as ReturnCode_t*
ReturnCode_t* W5500_Create(Pin_t MISO, Pin_t MOSI, Pin_t CLK, Pin_t SCS, Pin_t RST, Pin_t INT) {
    EthernetW5500_t* obj = (EthernetW5500_t*)malloc(sizeof(EthernetW5500_t));
    if (obj == NULL) {
        return NULL;
    }

    /* Initialize memory and assign pins to the structure */
    memset(obj->Raw, 0, sizeof(EthernetW5500_t));
    obj->Pinout.MISO = MISO; obj->Pinout.MOSI = MOSI;
    obj->Pinout.CLK = CLK;   obj->Pinout.SCS = SCS;
    obj->Pinout.RST = RST;   obj->Pinout.INT = INT;

    /* Configure Reset and SCS pins as output */
    gpio_reset_pin(SCS);
    gpio_set_direction(SCS, GPIO_MODE_OUTPUT);
    gpio_set_level(SCS, 1);

    if (RST != 0xFF) {
        gpio_reset_pin(RST);
        gpio_set_direction(RST, GPIO_MODE_OUTPUT);
        /* Hardware reset pulse */
        gpio_set_level(RST, 0);
        espromdelay
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(RST, 1);
    }

    /* Setup SPI bus configuration */
    spi_bus_config_t bus_cfg = {
        .miso_io_num = MISO,
        .mosi_io_num = MOSI,
        .sclk_io_num = CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1
    };

    /* Setup SPI device configuration with manual CS */
    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = 20 * 1000 * 1000, /* 20MHz */
        .mode = 0,
        .spics_io_num = -1, /* Manual CS control */
        .queue_size = 7
    };

    /* Initialize SPI bus and add device */
    spi_bus_initialize(HSPI_HOST, &bus_cfg, 1);
    spi_bus_add_device(HSPI_HOST, &dev_cfg, &W5500_SPI_Handle);

    return (ReturnCode_t*)obj;
}

/// @brief Delete EthernetW5500_t object and set ptr to NULL to prevent dangling pointer
/// @param Ptr Pointer to the pointer of the management object
/// @return ReturnCode_t* Returns NULL after successful deletion
ReturnCode_t* W5500_Delete(ReturnCode_t** Ptr) {
    if (Ptr != NULL && *Ptr != NULL) {
        /* Remove SPI device and free allocated memory */
        spi_bus_remove_device(W5500_SPI_Handle);
        free(*Ptr);
        *Ptr = NULL;
    }
    return NULL;
}

/// @brief Pull SCS pin to down then send the 3 bytes of header
/// @param Ptr Pointer to the management object
/// @param Header W5500 header structure containing address and control
/// @return ReturnCode_t STAT_OKE if successful
ReturnCode_t W5500_SendHeader(ReturnCode_t* Ptr, W5500Header_t Header) {
    EthernetW5500_t* obj = (EthernetW5500_t*)Ptr;
    if (obj == NULL) return STAT_ERR_NULL;

    /* Pull CS Low to start transaction */
    gpio_set_level(obj->Pinout.SCS, 0);

    spi_transaction_t t = {
        .length = 8 * 3,
        .tx_buffer = &Header
    };

    /* Send 3-byte header via SPI */
    spi_device_polling_transmit(W5500_SPI_Handle, &t);
    return STAT_OKE;
}

/// @brief Pull SCS pin to down; Send N bytes of payload; Set SCS to HIGH
/// @param Ptr Pointer to the management object
/// @param Payload Pointer to the data buffer
/// @param PayloadLength Number of bytes to transfer
/// @return ReturnCode_t STAT_OKE if successful
ReturnCode_t W5500_SendPayload(ReturnCode_t* Ptr, void* Payload, uint16_t PayloadLength) {
    EthernetW5500_t* obj = (EthernetW5500_t*)Ptr;
    if (obj == NULL || Payload == NULL) return STAT_ERR_NULL;

    spi_transaction_t t = {
        .length = 8 * PayloadLength,
        .tx_buffer = Payload
    };

    /* Transfer data and release CS */
    spi_device_polling_transmit(W5500_SPI_Handle, &t);
    gpio_set_level(obj->Pinout.SCS, 1);
    
    return STAT_OKE;
}

/// @brief Send header followed by 1-4 bytes of fixed payload
/// @param Ptr Pointer to the management object
/// @param Header W5500 header structure (OpMode should be FDM1/2/4)
/// @param Payload Pointer to the small data buffer
/// @return ReturnCode_t STAT_OKE if successful
ReturnCode_t W5500_SendCommand(ReturnCode_t* Ptr, W5500Header_t Header, void* Payload) {
    uint8_t len = 0;
    /* Determine fixed length from OpMode bits */
    if (Header.OpMode == eW5500_FDM1) len = 1;
    else if (Header.OpMode == eW5500_FDM2) len = 2;
    else if (Header.OpMode == eW5500_FDM4) len = 4;

    /* Execute header phase then payload phase */
    W5500_SendHeader(Ptr, Header);
    return W5500_SendPayload(Ptr, Payload, len);
}

/// @brief Send header followed by N bytes of variable payload
/// @param Ptr Pointer to the management object
/// @param Header W5500 header structure (OpMode should be VDM)
/// @param Payload Pointer to the data buffer
/// @param PayloadLength Total data length to send
/// @return ReturnCode_t STAT_OKE if successful
ReturnCode_t W5500_SendData(ReturnCode_t* Ptr, W5500Header_t Header, void* Payload, uint16_t PayloadLength) {
    /* Ensure OpMode is set to Variable Data Mode */
    Header.OpMode = eW5500_VDM;
    
    /* Execute full SPI burst with CS control */
    W5500_SendHeader(Ptr, Header);
    return W5500_SendPayload(Ptr, Payload, PayloadLength);
}