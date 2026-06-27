#include "__CommonHeaders.h"
#include "TestModule.h"
// #include "../../AppComm/SharedAPIs.h"
// #include "../HeartBeat/Module.h"
// #include "driver/gpio.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "rom/ets_sys.h"
// #include "string.h"

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
    if (!IsValidPin(PIN_TEST_TRIG) || !IsValidPin(PIN_TEST_ECHO)) {
        return STAT_ERR_INVALID_ARG;
    }

    // Configure PIN_TEST_TRIG as input with falling edge interrupt
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIN_TEST_TRIG),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE
    };
    gpio_config(&io_conf);
    
    gpio_install_isr_service(0);
    gpio_isr_handler_add(PIN_TEST_TRIG, Test_ISR_Handler, NULL);

    // Configure PIN_TEST_ECHO as output pull-up
    io_conf.pin_bit_mask = (1ULL << PIN_TEST_ECHO);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);
    
    // Set to high initially
    gpio_set_level(PIN_TEST_ECHO, 1);

    return STAT_OKE;
}

void Test_WakeUp() {
    // if (TestTaskHandle != NULL) {
    //     xTaskNotifyGive(TestTaskHandle);
    // }
    
    // Wake the runtime up, then make ECHO low for 1ms, then back to high
    gpio_set_level(PIN_TEST_ECHO, 0);
    ets_delay_us(1000); // 1ms delay
    gpio_set_level(PIN_TEST_ECHO, 1);
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
        // Sleep until notified (e.g. by trigger interrupt or KeepAlive receive)
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        
        SysLog("Test_Runtime(...): Woken up! Sending KeepAlive packet...");
        
        // Write KeepAlive body
        App_WriteFrame_KeepAlive(&tx_frame, sync_num++, 0xAA);
        
        // Broadcast KeepAlive via W5500
        extern ReturnCode_t Eth_SendUDPPacket(uint32_t IPv4Address, uint16_t IPv4Port, uint8_t* UDP_Payload, uint16_t PayloadSize);
        ReturnCode_t ret = Eth_SendUDPPacket(0xFFFFFFFF, DEFAULT_PORT_VAL, (uint8_t*)&tx_frame, sizeof(ZECUFrame_Generic_t));
        if(ret != STAT_OKE) {
            SysErr("Test_Runtime(...): Failed to send KeepAlive!");
        }
    }
}
