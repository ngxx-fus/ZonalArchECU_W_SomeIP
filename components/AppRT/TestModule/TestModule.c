#include "__CommonHeaders.h"
#include "TestModule.h"

/* NOTE:
* Due to the mistake of HW engineer ;v There's only has one GPIO to use.
* So I'll add a macro to hot-fix this problem.
*/
#define TEST_SINGLE_GPIO_PIN    PIN_TEST_TRIG /* Use PIN_TEST_TRIG as the single pin */

static TaskHandle_t TestTaskHandle = NULL;

static void IRAM_ATTR Test_ISR_Handler(void* arg) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (TestTaskHandle != NULL) {
        vTaskNotifyGiveFromISR(TestTaskHandle, &xHigherPriorityTaskWoken);
    }
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

ReturnCode_t Test_Init() {
#if (ZECU_ID_VAL == 0x1D0A) /* Trigger ZECU: Pin is an INPUT with Interrupt */
    if (!IsValidPin(TEST_SINGLE_GPIO_PIN)) {
        return STAT_ERR_INVALID_ARG;
    }
    // Configure the single pin as an input with a falling edge interrupt
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << TEST_SINGLE_GPIO_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE
    };
    gpio_config(&io_conf);
    
    // Install the ISR service and handler for the trigger pin
    gpio_install_isr_service(0);
    gpio_isr_handler_add(TEST_SINGLE_GPIO_PIN, Test_ISR_Handler, NULL);
    SysLog("TestModule: Configured as TRIGGER ZECU (0x1D0A) on GPIO %d.", TEST_SINGLE_GPIO_PIN);

#elif (ZECU_ID_VAL == 0x1D0B) /* Echo ZECU: Pin is an OUTPUT */
    if (!IsValidPin(TEST_SINGLE_GPIO_PIN)) {
        return STAT_ERR_INVALID_ARG;
    }
    // Configure the single pin as an output with pull-up enabled
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << TEST_SINGLE_GPIO_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    
    // Set the pin high initially
    gpio_set_level(TEST_SINGLE_GPIO_PIN, 1);
    SysLog("TestModule: Configured as ECHO ZECU (0x1D0B) on GPIO %d.", TEST_SINGLE_GPIO_PIN);
#endif
    return STAT_OKE;
}

void Test_WakeUp() {
#if (ZECU_ID_VAL == 0x1D0B) /* Only the Echo ZECU generates a pulse */
    gpio_set_level(TEST_SINGLE_GPIO_PIN, 0);
    ets_delay_us(1000); // 1ms delay
    gpio_set_level(TEST_SINGLE_GPIO_PIN, 1);
#endif
}

ReturnCode_t Test_SetISRHandler(GenericPtr_t ISRHandler) {
    return STAT_ERR_UNSUPPORTED;
}

void Test_Runtime(void* args) {
    SysEntry("Test_Runtime");
    SysLog("Test_Runtime(...): Started!");
    
    TestTaskHandle = xTaskGetCurrentTaskHandle();
    
    // Initialize pins and ISR
    Test_Init();
    
    ZECUFrame_Generic_t tx_frame;
    memset(&tx_frame, 0, sizeof(ZECUFrame_Generic_t));
    uint8_t sync_num = 0;

    // Fill header information similar to HeartBeat
    tx_frame.Header.MagicByte0 = ZECU_FRAME_HEADER_MAGIC_BYTE;
    tx_frame.Header.Version    = ZECU_FRAME_VERSION;
    tx_frame.Header.MagicByte1 = ZECU_FRAME_HEADER_MAGIC_BYTE;
    tx_frame.Header.MagicByte2 = ZECU_FRAME_HEADER_MAGIC_BYTE;
    strcpy(tx_frame.Header.Info.Label, ECU_NAME_STR);

    // Wait for system Initialization to pass phase 3
    while(3 >= GlobalInit_GetLevel()){ vTaskDelay(pdMS_TO_TICKS(50)); }

    while(1) {
        // Sleep until notified by the ISR (only for Trigger ZECU)
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        
#if (ZECU_ID_VAL == 0x1D0A) /* Only the Trigger ZECU sends a packet when woken up */
        {
            SysLog("Test_Runtime(...): Woken up by trigger! Sending KeepAlive packet...");
            
            // Write KeepAlive body
            App_WriteFrame_KeepAlive(&tx_frame, sync_num++, 0xAA);
            
            // Broadcast KeepAlive via W5500
            extern ReturnCode_t Eth_SendUDPPacket(uint32_t IPv4Address, uint16_t IPv4Port, uint8_t* UDP_Payload, uint16_t PayloadSize);
            ReturnCode_t ret = Eth_SendUDPPacket(0xFFFFFFFF, DEFAULT_PORT_VAL, (uint8_t*)&tx_frame, sizeof(ZECUFrame_Generic_t));
            if(ret != STAT_OKE) {
                SysErr("Test_Runtime(...): Failed to send KeepAlive!");
            }
        }
#endif
    }
}
