#include "../AppRT/StartUp/Service.h"

/// @brief Main entry point of the application
void app_main(void) {
    SysEntry("app_main");

    do{/*Try start-up system*/}
    while(AppInit() != STAT_OKE);

    SysLog("[app_main] System initialized, entering idle state...");
    while (1) {
        vTaskDelay(portMAX_DELAY);
    }
}