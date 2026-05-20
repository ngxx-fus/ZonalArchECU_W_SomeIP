#include "../AppRT/StartUp/Service.h"

/// @brief Main entry point of the application
void app_main(void) {
    SysEntry("app_main");
    #ifndef __ZECU_DISABLED__
        do{/*Try start-up system*/}
        while(AppInit() != STAT_OKE);
    #else
        SysLog("[app_main] This ECU is disabled! Please clear `__ZECU_DISABLED_` macro the enable!");
    #endif /*iodefine"?*/
    SysLog("[app_main] System initialized, entering idle state...");
    while (1) {
        vTaskDelay(portMAX_DELAY);
    }
}
