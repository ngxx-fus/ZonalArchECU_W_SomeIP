#include "MainApp.h"

/// @brief Main entry point of the application
void app_main(void) {
    SysEntry("app_main");
    AppInit(NULL);
    SysLog("[app_main] System initialized, entering idle state...");
    while (1) {
        vTaskDelay(portMAX_DELAY);
    }
}