#include "esp_random.h"
#include "MainApp.h"

void app_main(void) {
    HBridge_t * DC_B;
    /* Initialize H-Bridge instance with defined project pins */
    DC_B = HBridge_Create(PIN_B_DC_IN0, PIN_B_DC_IN1, PIN_B_DC_IN2, PIN_B_DC_IN3);

    SysLog("Starting Ramp-up phase: 0 to 255");
    /* Text */
    /* Gradually increase speed from 0 to 255 with 500ms interval */
    for (int16_t speed = 0; speed <= 255; speed++) {
        HBridge_SetSpeed(DC_B, speed, speed);
        HBridge_Apply(DC_B);
        
        SysLog("Ramp-up - Speed: %d", speed);
        SleepMs(100);
    }

    SysLog("Transitioning to Chaotic movement loop");
    while (1) {
        /* Generate random speeds between -100 and 100 for chaotic behavior */
        int16_t rand_a = (int16_t)((esp_random() % 201) - 100);
        int16_t rand_b = (int16_t)((esp_random() % 201) - 100);

        /* Update motor speeds independently and commit to hardware */
        HBridge_SetSpeed0(DC_B, rand_a); 
        HBridge_SetSpeed1(DC_B, rand_b); 
        HBridge_Apply(DC_B);

        SysLog("Chaos - M0: %d, M1: %d", rand_a, rand_b);
        SleepMs(200);
    }
}