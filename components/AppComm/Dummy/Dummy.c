#include "Dummy.h"

/* Module to test */
#include "../HBridge/Module.h"

/// @brief Task that incrementally increases speed for both motors
/// @param arg Task arguments
void Dummy0(void * arg) {
    SysEntry("Dummy_Incremental");
    int16_t current_speed = -1023;

    SysLog("[Incremental] Starting incremental test loop...");
    while (1) {
        /* Update both motor speeds and log the increment */
        MotorSetSpeed0(current_speed);
        MotorSetSpeed1(current_speed);
        SysLog("[Incremental] Step - Speed: %d", current_speed);

        /* Increment speed and wrap around if limit reached */
        current_speed += 100;
        if (current_speed > 1023) {
            current_speed = -1023;
        }

        vTaskDelay(pdMS_TO_TICKS(580));
    }
}

/// @brief Task that incrementally decreases speed for both motors
/// @param arg Task arguments
void Dummy1(void * arg) {
    SysEntry("Dummy_Decremental");
    int16_t current_speed = 1023;

    SysLog("[Decremental] Starting decremental test loop...");
    while (1) {
        /* Update both motor speeds and log the decrement */
        MotorSetSpeed0(current_speed);
        MotorSetSpeed1(current_speed);
        SysLog("[Decremental] Step - Speed: %d", current_speed);

        /* Decrement speed and wrap around if limit reached */
        current_speed -= 100;
        if (current_speed < -1023) {
            current_speed = 1023;
        }

        vTaskDelay(pdMS_TO_TICKS(907));
    }
}

/// @brief Task that injects random speeds at a long interval
/// @param arg Task arguments
void Dummy_Jitter(void * arg) {
    SysEntry("Dummy_Random");
    
    SysLog("[Random] Starting slow random jitter loop...");
    while (1) {
        /* Generate random speeds for both channels within 10-bit range */
        int32_t s0 = (rand() % 2047) - 1023;
        int32_t s1 = (rand() % 2047) - 1023;

        /* Apply chaos to both motors and log values */
        MotorSetSpeed0(s0);
        MotorSetSpeed1(s1);
        SysLog("[Random] Applied - S0: %d, S1: %d", s0, s1);

        vTaskDelay(pdMS_TO_TICKS(1732));
    }
}


void Dummy3(void * arg){
    SysEntry("Dummy3");
    
    SysLog("[Random] Starting slow random jitter loop...");
    while (1) {
        /* Generate random speeds for both channels within 10-bit range */
        int32_t s0 = (rand() % 2047) - 1023;
        int32_t s1 = (rand() % 2047) - 1023;

        /* Apply chaos to both motors and log values */
        MotorSetSpeed0(s0);
        MotorSetSpeed1(s1);
        SysLog("[Random] Applied - S0: %d, S1: %d", s0, s1);

        vTaskDelay(pdMS_TO_TICKS(1732));
    }
}
