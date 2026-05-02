#include "Module.h"

#define HCSR04_LOG_PERIOD_MS 500

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
            unsigned distanceMm = 0xFFFFU;
            uint16_t rawDistance = 0xFFFFU;
            (void)UltraSonic_GetDistance(ultra, (uint8_t)i, &rawDistance);
            distanceMm = (unsigned)rawDistance;

            if (distanceMm == 0xFFFFU) {
                SysLog("[HCSR04Ctl] S%u=timeout", (unsigned)i);
            } else {
                unsigned distanceCmX10 = distanceMm;
                unsigned cmWhole = distanceCmX10 / 10U;
                unsigned cmFrac = distanceCmX10 % 10U;
                SysLog("[HCSR04Ctl] S%u=%u.%u cm", (unsigned)i, (unsigned)cmWhole, (unsigned)cmFrac);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(HCSR04_LOG_PERIOD_MS));
    }
}
