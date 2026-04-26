#include "Module.h"

void HCSR04Ctl(void *arg) {
    UltraSonic_t *ultra;

    (void)arg;
    SysEntry("HCSR04Ctl");

    ultra = UltraSonic_CreateDefault();
    if (ultra == NULL) {
        SysErr("[HCSR04Ctl] Ultrasonic object allocation failed");
        vTaskDelete(NULL);
        return;
    }

    if (UltraSonic_Init(ultra) != STAT_OKE) {
        SysErr("[HCSR04Ctl] Ultrasonic initialization failed");
        (void)UltraSonic_Delete(&ultra);
        vTaskDelete(NULL);
        return;
    }

    SysLog("[HCSR04Ctl] Start collecting ultrasonic distance");
    while (1) {
        unsigned count;

        (void)UltraSonic_ReadAll(ultra);
        count = (unsigned)UltraSonic_GetCount(ultra);

        for (unsigned i = 0; i < count; ++i) {
            unsigned distance = 0xFFFFU;
            uint16_t rawDistance = 0xFFFFU;
            (void)UltraSonic_GetDistance(ultra, (uint8_t)i, &rawDistance);
            distance = (unsigned)rawDistance;
            SysLog("[HCSR04Ctl] S%u=%u mm", (unsigned)i, (unsigned)distance);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
