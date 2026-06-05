#include "Module.h"

HBridge_t *         Motor = NULL;               /// Global Motor struct
SemaphoreHandle_t   MotorLock = NULL;           /// Mutex for thread-safe Motor access

static volatile uint8_t IsEmergencyStopped = 0; /// Flag to lock motor runtime when emergency occurs

/// @brief Set speed for Motor 0 (Channel A)
/// @param Speed Target speed value (-1023 to 1023)
/// @return ReturnCode_t STAT_OKE if successful
ReturnCode_t MotorSetSpeed0(int32_t Speed) {
    if (IsEmergencyStopped) return STAT_ERR;
    if (MotorLock == NULL || Motor == NULL) return STAT_ERR_NULL;

    SysLog("MotorSetSpeed0(...): Requested Speed = %d", Speed);

    /* lock resource and update speed0 */
    if (xSemaphoreTake(MotorLock, portMAX_DELAY) == pdTRUE) {
        HBridge_SetSpeed(Motor, (int16_t)Speed, Motor->Speed1);
        xSemaphoreGive(MotorLock);
        return STAT_OKE;
    }
    return STAT_ERR;
}

/// @brief Set speed for Motor 1 (Channel B)
/// @param Speed Target speed value (-1023 to 1023)
/// @return ReturnCode_t STAT_OKE if successful
ReturnCode_t MotorSetSpeed1(int32_t Speed) {
    if (IsEmergencyStopped) return STAT_ERR;
    if (MotorLock == NULL || Motor == NULL) return STAT_ERR_NULL;

    SysLog("MotorSetSpeed1(...): Requested Speed = %d", Speed);

    /* lock resource and update speed1 */
    if (xSemaphoreTake(MotorLock, portMAX_DELAY) == pdTRUE) {
        HBridge_SetSpeed(Motor, Motor->Speed0, (int16_t)Speed);
        xSemaphoreGive(MotorLock);
        return STAT_OKE;
    }
    return STAT_ERR;
}

/// @brief Get current speed of Motor 0
/// @return int32_t Current speed value
int32_t MotorGetSpeed0(void) {
    int32_t val = 0;
    /* safe read motor0 speed */
    if (MotorLock && xSemaphoreTake(MotorLock, portMAX_DELAY) == pdTRUE) {
        val = Motor->Speed0;
        xSemaphoreGive(MotorLock);
    }
    return val;
}

/// @brief Get current speed of Motor 1
/// @return int32_t Current speed value
int32_t MotorGetSpeed1(void) {
    int32_t val = 0;
    /* safe read motor1 speed */
    if (MotorLock && xSemaphoreTake(MotorLock, portMAX_DELAY) == pdTRUE) {
        val = Motor->Speed1;
        xSemaphoreGive(MotorLock);
    }
    return val;
}

/// @brief Stop all motor for Emergency case
/// @return ReturnCode_t Status of the action
ReturnCode_t EmergencyStop(void) {
    if (MotorLock == NULL || Motor == NULL) return STAT_ERR_NULL;
    if (IsEmergencyStopped) {
        SysWarn("EmergencyStop(...): Motor is already in Emergency Stop state!");
        return STAT_OKE; // Already stopped
    }

    // /* lock resource and stop both motors immediately */
    // if (xSemaphoreTake(MotorLock, portMAX_DELAY) == pdTRUE) {
    //     IsEmergencyStopped = 1;
    //     // HBridge_SetSpeed(Motor, 0, 0);
    //     // HBridge_Apply(Motor); // Force apply to hardware immediately
    //     xSemaphoreGive(MotorLock);
    //     SysLog("EmergencyStop(...): Motor stopped! (Emergency case)");
    //     return STAT_OKE;
    // }
    return STAT_ERR;
}

/* --- SERVICE RUNTIME --- */

/// @brief Main Motor Control Task
/// @param arg Task arguments
void MotorRuntime(void* arg) {
    SysEntry("AppService_HBridge");
    
    /* initialize mutex before creating motor object */
    MotorLock = xSemaphoreCreateMutex();
    
    SysLog("[AppService_HBridge] New motor object...");
    Motor = HBridge_Create(PIN_B_DC_IN0, PIN_B_DC_IN1, PIN_B_DC_IN2, PIN_B_DC_IN3);
    
    if (IsNull(Motor)) {
        SysErr("[AppService_HBridge] Cannot initialize `Motor`!");
        vTaskDelete(NULL);
        return;
    }

    SysLog("[AppService_HBridge] Join forever loop...");
    while (1) {
        /* If ECU is in emergency state, block task from applying any further settings */
        if (IsEmergencyStopped) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        /* check and apply flags if any changes detected */
        if (xSemaphoreTake(MotorLock, pdMS_TO_TICKS(10)) == pdTRUE) {
            if (Motor->Flag != 0) {
                HBridge_Apply(Motor);
            }
            xSemaphoreGive(MotorLock);
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
