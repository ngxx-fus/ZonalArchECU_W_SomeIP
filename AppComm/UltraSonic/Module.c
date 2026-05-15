#include "Module.h"

#define HCSR04_LOG_PERIOD_MS 500

static UltraSonic_t *g_ultra = NULL;

uint16_t HCSR04_GetLatestDistanceMm(uint8_t index) {
    uint16_t dist = 0xFFFF;
    if (g_ultra != NULL) {
        (void)UltraSonic_GetDistance(g_ultra, index, &dist);
    }
    return dist;
}

void HCSR04Ctl(void *arg) {
    (void)arg;
    SysEntry("HCSR04Ctl");

    g_ultra = UltraSonic_CreateDefault();
    if (g_ultra == NULL) {
        SysErr("[HCSR04Ctl] Ultrasonic object allocation failed");
        vTaskDelete(NULL);
        return;
    }

    if (UltraSonic_Init(g_ultra) != STAT_OKE) {
        SysErr("[HCSR04Ctl] Ultrasonic initialization failed");
        (void)UltraSonic_Delete(&g_ultra);
        vTaskDelete(NULL);
        return;
    }

    SysLog("[HCSR04Ctl] Start collecting ultrasonic distance");
    while (1) {
        unsigned count;

        (void)UltraSonic_ReadAll(g_ultra);
        count = (unsigned)UltraSonic_GetCount(g_ultra);

        for (unsigned i = 0; i < count; ++i) {
            unsigned distanceMm = 0xFFFFU;
            uint16_t rawDistance = 0xFFFFU;
            (void)UltraSonic_GetDistance(g_ultra, (uint8_t)i, &rawDistance);
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
