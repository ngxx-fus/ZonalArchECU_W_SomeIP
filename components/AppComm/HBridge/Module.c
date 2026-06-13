#include "Module.h"

HBridge_t *         Motor = NULL;               /* Global Motor struct */
SemaphoreHandle_t   MotorLock = NULL;           /* Mutex for thread-safe Motor access */

/* Handle to wake up the motor runtime task immediately */
static TaskHandle_t MotorTaskHandle = NULL;     

#define ENABLE_EMERGENCY_ACTION    0
static volatile uint8_t IsEmergencyStopped = 0; /* Flag to lock motor runtime when emergency occurs */

ReturnCode_t MotorSetSpeed(int16_t Speed0, int16_t Speed1){
    /* Control flow: prevent operation if emergency stop is active */
    #if (ENABLE_EMERGENCY_ACTION==1)
        if (IsEmergencyStopped) {
            /* Return error immediately due to emergency state */
            return STAT_ERR;
        }
    #endif
    
    /* Control flow: prevent null pointer access */
    if (MotorLock == NULL || Motor == NULL) {
        /* Return initialization error */
        return STAT_ERR_NULL;
    }

    /// SysLog("MotorSetSpeed(...): Requested Speed0 = %d, Speed1 = %d", Speed0, Speed1);

    /* Control flow: attempt to acquire resource lock to update speed safely */
    if (xSemaphoreTake(MotorLock, portMAX_DELAY) == pdTRUE) {
        HBridge_SetSpeed(Motor, Speed0, Speed1);
        SysLog("MotorSetSpeed(...): Updated Speed0 = %d, Speed1 = %d", Speed0, Speed1);
        xSemaphoreGive(MotorLock);
        
        /* Control flow: trigger immediate wake-up for the motor task */
        if (MotorTaskHandle != NULL) {
            xTaskNotifyGive(MotorTaskHandle);
        }
        
        /* Return execution success */
        return STAT_OKE;
    }
    
    /* Return failure if lock was not acquired */
    return STAT_ERR;
}

/*
 * @brief Set speed for Motor 0 (Channel A)
 * @param Speed Target speed value (-1023 to 1023)
 * @return ReturnCode_t STAT_OKE if successful
 */
ReturnCode_t MotorSetSpeed0(int32_t Speed) {
    /* Control flow: prevent operation if emergency stop is active */
    #if (ENABLE_EMERGENCY_ACTION==1)
        if (IsEmergencyStopped) {
            /* Return error immediately due to emergency state */
            return STAT_ERR;
        }
    #endif
    
    /* Control flow: prevent null pointer access */
    if (MotorLock == NULL || Motor == NULL) {
        /* Return initialization error */
        return STAT_ERR_NULL;
    }

    /// SysLog("MotorSetSpeed0(...): Requested Speed0 = %d", Speed);

    /* Control flow: attempt to acquire resource lock to update speed safely */
    if (xSemaphoreTake(MotorLock, portMAX_DELAY) == pdTRUE) {
        HBridge_SetSpeed0(Motor, (int16_t)Speed);
        SysLog("MotorSetSpeed0(...): Updated Speed0 = %d", Speed);
        xSemaphoreGive(MotorLock);
        
        /* Control flow: trigger immediate wake-up for the motor task */
        if (MotorTaskHandle != NULL) {
            xTaskNotifyGive(MotorTaskHandle);
        }
        
        /* Return execution success */
        return STAT_OKE;
    }
    
    /* Return failure if lock was not acquired */
    return STAT_ERR;
}

/*
 * @brief Set speed for Motor 1 (Channel B)
 * @param Speed Target speed value (-1023 to 1023)
 * @return ReturnCode_t STAT_OKE if successful
 */
ReturnCode_t MotorSetSpeed1(int32_t Speed) {
    /* Control flow: prevent operation if emergency stop is active */
    #if (ENABLE_EMERGENCY_ACTION==1)
        if (IsEmergencyStopped) {
            /* Return error immediately due to emergency state */
            return STAT_ERR;
        }
    #endif
    
    /* Control flow: prevent null pointer access */
    if (MotorLock == NULL || Motor == NULL) {
        /* Return initialization error */
        return STAT_ERR_NULL;
    }

    /// SysLog("MotorSetSpeed1(...): Requested Speed1 = %d", Speed);

    /* Control flow: attempt to acquire resource lock to update speed safely */
    if (xSemaphoreTake(MotorLock, portMAX_DELAY) == pdTRUE) {
        HBridge_SetSpeed1(Motor, (int16_t)Speed);
        SysLog("MotorSetSpeed1(...): Updated Speed1 = %d", Speed);
        xSemaphoreGive(MotorLock);
        
        /* Control flow: trigger immediate wake-up for the motor task */
        if (MotorTaskHandle != NULL) {
            xTaskNotifyGive(MotorTaskHandle);
        }
        
        /* Return execution success */
        return STAT_OKE;
    }
    
    /* Return failure if lock was not acquired */
    return STAT_ERR;
}

/*
 * @brief Get current speed of Motor 0
 * @return int32_t Current speed value
 */
int32_t MotorGetSpeed0(void) {
    int32_t val = 0;
    
    /* Control flow: attempt to acquire resource lock for safe reading */
    if (MotorLock && xSemaphoreTake(MotorLock, portMAX_DELAY) == pdTRUE) {
        val = Motor->Speed0;
        xSemaphoreGive(MotorLock);
    }
    
    SysLog("MotorGetSpeed0(...): Speed: %d", val);
    /* Return the safely retrieved speed value */
    return val;
}

/*
 * @brief Get current speed of Motor 1
 * @return int32_t Current speed value
 */
int32_t MotorGetSpeed1(void) {
    int32_t val = 0;
    
    /* Control flow: attempt to acquire resource lock for safe reading */
    if (MotorLock && xSemaphoreTake(MotorLock, portMAX_DELAY) == pdTRUE) {
        val = Motor->Speed1;
        xSemaphoreGive(MotorLock);
    }
    SysLog("MotorGetSpeed1(...): Speed: %d", val);
    /* Return the safely retrieved speed value */
    return val;
}

/*
 * @brief Stop all motor for Emergency case
 * @return ReturnCode_t Status of the action
 */
ReturnCode_t EmergencyStop(void) {
    #if (ENABLE_EMERGENCY_ACTION==1)
        /* Control flow: prevent null pointer access */
        if (MotorLock == NULL || Motor == NULL) {
            /* Return initialization error */
            return STAT_ERR_NULL;
        }
        
        /* Control flow: verify if already stopped to avoid redundant operations */
        if (IsEmergencyStopped) {
            SysWarn("EmergencyStop(...): Motor is already in Emergency Stop state!");
            /* Return success as it is already stopped */
            return STAT_OKE; 
        }

        /* Control flow: lock resource and stop both motors immediately */
        if (xSemaphoreTake(MotorLock, portMAX_DELAY) == pdTRUE) {
            IsEmergencyStopped = 1;
            HBridge_SetSpeed(Motor, 0, 0);
            HBridge_Apply(Motor); /* Force apply to hardware immediately */
            xSemaphoreGive(MotorLock);
            
            SysLog("EmergencyStop(...): Motor stopped! (Emergency case)");
            
            /* Control flow: wake up task to acknowledge emergency */
            if (MotorTaskHandle != NULL) {
                xTaskNotifyGive(MotorTaskHandle);
            }
            
            /* Return execution success */
            return STAT_OKE;
        }
        /* Return failure if lock was not acquired */
        return STAT_ERR;
    #else
        return STAT_OKE;
    #endif
}

/* --- SERVICE RUNTIME --- */

/*
 * @brief Main Motor Control Task
 * @param arg Task arguments
 */
void MotorRuntime(void* arg) {
    SysEntry("MotorRuntime");
    
    /* Save current task handle for notification wake-ups */
    MotorTaskHandle = xTaskGetCurrentTaskHandle();
    
    /* Control flow: wait until global initialization allows proceeding */
    while(1 >= GlobalInit_GetLevel()) {
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    /* Initialize mutex before creating motor object */
    MotorLock = xSemaphoreCreateMutex();
    
    SysLog("[MotorRuntime] New motor object...");
    Motor = HBridge_Create(PIN_B_DC_IN0, PIN_B_DC_IN1, PIN_B_DC_IN2, PIN_B_DC_IN3);
    
    /* Control flow: verify if motor creation was successful */
    if (IsNull(Motor)) {
        SysErr("[MotorRuntime] Cannot initialize `Motor`!");
        vTaskDelete(NULL);
        
        /* Return to terminate task execution */
        return;
    }

    MotorSetSpeed0(0);
    MotorSetSpeed1(0);
    HBridge_Apply(Motor);

    GlobalInit_MoveNextLevel();

    SysLog("[MotorRuntime] Join forever loop...");
    
    /* Control flow: enter infinite task loop */
    while (1) {
        /* Control flow: Wait for notification to wake up immediately, or timeout after 20ms */
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(20));
        
        /* Control flow: check if ECU is in emergency state */
        #if (ENABLE_EMERGENCY_ACTION==1)
            if (IsEmergencyStopped) {
                /* Return error immediately due to emergency state */
                continue;
            }
        #endif

        /* Control flow: attempt to lock mutex to check and apply flags */
        if (xSemaphoreTake(MotorLock, pdMS_TO_TICKS(10)) == pdTRUE) {
            
            /* Control flow: apply changes to hardware if flag is raised */
            if (Motor->Flag != 0) {
                HBridge_Apply(Motor);
                Motor->Flag  = 0;
            }
            xSemaphoreGive(MotorLock);
        }
    }
}