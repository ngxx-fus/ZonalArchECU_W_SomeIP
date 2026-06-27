#include "UltraSonic.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h" /* Added for explicit Task Watchdog kicking */

/* HC-SR04 Specifications & OS Constraints */
#define HCSR04_TRIGGER_PULSE_US         (10U)
#define HCSR04_ECHO_TIMEOUT_US          (24000U) /* ~4 meters max range. Prevents OS lockup */
#define HCSR04_FILTER_DELAY_MS          (40U)    /* Minimum 40ms to prevent ghost echoes */
#define HCSR04_INTER_SENSOR_DELAY_MS    (40U)

static const Pin_t s_trigPin = PIN_US_TRIG;

/* Spinlock required by ESP-IDF SMP FreeRTOS implementation for critical sections */
static portMUX_TYPE s_hcsr04_mux = portMUX_INITIALIZER_UNLOCKED;

static const UltraSonicSensor_t s_defaultSensors[] = {
    { PIN_US_TRIG, PIN_US_L_ECHO },
    { PIN_US_TRIG, PIN_US_C_ECHO },
    { PIN_US_TRIG, PIN_US_R_ECHO },
};

/*
 * @brief Initializes GPIO configurations for the shared Trigger and individual Echo pins.
 * @param ptr Pointer to the UltraSonic object instance.
 * @return ReturnCode_t STAT_OKE on success.
 */
static ReturnCode_t HCSR04_InitPins(UltraSonic_t *ptr) {
    uint64_t echoMask = 0ULL;
    uint8_t i;

    /* Control flow: Verify valid pointer and parameters */
    if (ptr == NULL || ptr->Sensor == NULL || ptr->SensorCount == 0) {
        /* Return invalid argument error due to null pointers */
        return STAT_ERR_INVALID_ARG;
    }

    if (!IsValidPin(s_trigPin)) {
        /* Return invalid argument error due to invalid trigger pin */
        return STAT_ERR_INVALID_ARG;
    }

    /* Compile mask for all active ECHO pins */
    for (i = 0; i < ptr->SensorCount; ++i) {
        if (!IsValidPin(ptr->Sensor[i].Echo)) {
            /* Return invalid argument error if any echo pin is invalid */
            return STAT_ERR_INVALID_ARG;
        }
        echoMask |= (1ULL << (uint8_t)ptr->Sensor[i].Echo);
    }

    gpio_config_t trig_cfg = {
        .pin_bit_mask = (1ULL << (uint8_t)s_trigPin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 0,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE,
    };
    
    gpio_config_t echo_cfg = {
        .pin_bit_mask = echoMask,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 0,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE,
    };

    /* Control flow: Apply hardware configurations */
    if (gpio_config(&trig_cfg) != ESP_OK || gpio_config(&echo_cfg) != ESP_OK) {
        /* Return IO error if GPIO configuration fails */
        return STAT_ERR_IO;
    }

    gpio_set_level((gpio_num_t)s_trigPin, 0);
    
    /* Return success after successful configuration */
    return STAT_OKE;
}

/*
 * @brief Reads a single raw distance measurement from the specified sensor.
 * @details Locks the entire read process inside a critical section to guarantee exact timing.
 * @param ptr Pointer to the UltraSonic object instance.
 * @param index Index of the sensor to query.
 * @param distance_mm Output parameter to store the calculated distance.
 * @return ReturnCode_t STAT_OKE on successful read.
 */
static ReturnCode_t HCSR04_ReadOneRaw(UltraSonic_t *ptr, uint8_t index, uint32_t *distance_mm) {
    int64_t t0;
    int64_t timeout_at;
    int64_t pulse_us;
    gpio_num_t echoPin;

    /* Control flow: Verify pointers and index bounds */
    if (ptr == NULL || ptr->Sensor == NULL || distance_mm == NULL || index >= ptr->SensorCount) {
        /* Return invalid argument error on bad parameters */
        return STAT_ERR_INVALID_ARG;
    }

    echoPin = (gpio_num_t)ptr->Sensor[index].Echo;

    /// SysLog("HCSR04_ReadOneRaw(...): Trigger TRIG: %d", s_trigPin);
    /// SysLog("HCSR04_ReadOneRaw(...): Wait for ECHO pin: %d", echoPin);

    /* * Control flow: Enter OS Critical Section for the ENTIRE read process.
     * The user explicitly accepts busy-waiting up to 24ms. This guarantees 100% 
     * timing accuracy because FreeRTOS cannot preempt this core.
     */
    taskENTER_CRITICAL(&s_hcsr04_mux);

    gpio_set_level((gpio_num_t)s_trigPin, 0);
    esp_rom_delay_us(HCSR04_TRIGGER_PULSE_US);
    gpio_set_level((gpio_num_t)s_trigPin, 1);
    esp_rom_delay_us(2*HCSR04_TRIGGER_PULSE_US);
    gpio_set_level((gpio_num_t)s_trigPin, 0);

    timeout_at = esp_timer_get_time() + HCSR04_ECHO_TIMEOUT_US;

    /* Control flow: Wait for ECHO pin to go HIGH (Pulse Start) */
    while (gpio_get_level(echoPin) == 0) {
        if (esp_timer_get_time() > timeout_at) {
            /* Control flow: release spinlock before returning to prevent deadlock */
            taskEXIT_CRITICAL(&s_hcsr04_mux);
            
            /// SysLog("HCSR04_ReadOneRaw(...): Timeout! -> Return");
            
            /* Control flow: kick watchdog before exiting with timeout */
            /// esp_task_wdt_reset();
            
            /* Return timeout error if pulse start is missed */
            return STAT_ERR_TIMEOUT;
        }
    }

    t0 = esp_timer_get_time();
    timeout_at = t0 + HCSR04_ECHO_TIMEOUT_US;
    
    /* Control flow: Wait for ECHO pin to return LOW (Pulse End) */
    while (gpio_get_level(echoPin) == 1) {
        if (esp_timer_get_time() > timeout_at) {
            /* Control flow: release spinlock before returning to prevent deadlock */
            taskEXIT_CRITICAL(&s_hcsr04_mux);
            
            /* Control flow: kick watchdog before exiting with timeout */
            /// esp_task_wdt_reset();
            
            /* Return timeout error if pulse does not end */
            return STAT_ERR_TIMEOUT;
        }
    }

    pulse_us = esp_timer_get_time() - t0;
    
    /* Control flow: Exit critical section immediately after time-sensitive operations */
    taskEXIT_CRITICAL(&s_hcsr04_mux);
    
    /* Control flow: manually reset Task Watchdog after a successful read cycle */
    /// esp_task_wdt_reset();
    
    /* * Control flow: Filter out impossible physics values. */
    if (pulse_us < 0 || pulse_us > HCSR04_ECHO_TIMEOUT_US) {
        /* Return timeout error for out of bounds pulse duration */
        return STAT_ERR_TIMEOUT;
    }

    /* Convert time payload to millimeters (Sound speed ~340m/s -> 58.2us/cm) */
    *distance_mm = (uint32_t)((pulse_us * 1000LL) / 5820LL);

    /* Return success after calculating distance */
    return STAT_OKE;
}

/*
 * @brief Takes multiple samples and applies a Median filter to reject noise.
 * @param ptr Pointer to the UltraSonic object instance.
 * @param index Index of the sensor to query.
 * @return uint16_t Filtered distance in millimeters, or 0xFFFF if reading failed.
 */
static uint16_t HCSR04_ReadFiltered(UltraSonic_t *ptr, uint8_t index) {
    uint32_t d0 = 0, d1 = 0, d2 = 0;
    uint32_t samples[3];
    uint8_t valid = 0;

    /* Control flow: Gather three samples with proper acoustic dissipation delays */
    if (HCSR04_ReadOneRaw(ptr, index, &d0) == STAT_OKE) {
        samples[valid++] = d0;
    }
    vTaskDelay(pdMS_TO_TICKS(HCSR04_FILTER_DELAY_MS));
    
    if (HCSR04_ReadOneRaw(ptr, index, &d1) == STAT_OKE) {
        samples[valid++] = d1;
    }
    vTaskDelay(pdMS_TO_TICKS(HCSR04_FILTER_DELAY_MS));
    
    if (HCSR04_ReadOneRaw(ptr, index, &d2) == STAT_OKE) {
        samples[valid++] = d2;
    }

    /* Control flow: Evaluate collected samples */
    if (valid == 0) {
        /* Return error code if no valid samples gathered */
        return 0xFFFF;
    }
    
    if (valid == 1) {
        /* Return single valid sample bounded to uint16_t */
        return (samples[0] > 65535U) ? 65535U : (uint16_t)samples[0];
    }
    
    if (valid == 2) {
        uint32_t avg = (samples[0] + samples[1]) / 2U;
        /* Return average of two valid samples bounded to uint16_t */
        return (avg > 65535U) ? 65535U : (uint16_t)avg;
    }

    /* Resolve Median for 3 valid samples */
    if ((samples[0] >= samples[1] && samples[0] <= samples[2]) || 
        (samples[0] >= samples[2] && samples[0] <= samples[1])) {
        /* Return median sample 0 */
        return (samples[0] > 65535U) ? 65535U : (uint16_t)samples[0];
    }
    
    if ((samples[1] >= samples[0] && samples[1] <= samples[2]) || 
        (samples[1] >= samples[2] && samples[1] <= samples[0])) {
        /* Return median sample 1 */
        return (samples[1] > 65535U) ? 65535U : (uint16_t)samples[1];
    }
    
    /* Return median sample 2 */
    return (samples[2] > 65535U) ? 65535U : (uint16_t)samples[2];
}

/*
 * @brief Allocates and initializes an UltraSonic tracking structure.
 * @param sensorCfg Array of Sensor pin configurations.
 * @param sensorCount Number of active sensors.
 * @return UltraSonic_t* Pointer to created instance, or NULL on failure.
 */
UltraSonic_t *UltraSonic_Create(const UltraSonicSensor_t *sensorCfg, uint8_t sensorCount) {
    UltraSonic_t *obj;

    /* Control flow: validate configuration parameters */
    if (sensorCfg == NULL || sensorCount == 0) {
        /* Return NULL on invalid arguments */
        return NULL;
    }

    obj = (UltraSonic_t *)calloc(1, sizeof(UltraSonic_t));
    if (obj == NULL) {
        /* Return NULL if main struct allocation fails */
        return NULL;
    }

    obj->DistanceMm = (uint16_t *)calloc(sensorCount, sizeof(uint16_t));
    if (obj->DistanceMm == NULL) {
        free(obj);
        /* Return NULL if array allocation fails */
        return NULL;
    }

    obj->Sensor = sensorCfg;
    obj->SensorCount = sensorCount;
    
    /* Control flow: initialize thread-safety primitive */
    obj->Lock = xSemaphoreCreateMutex();
    if (obj->Lock == NULL) {
        free(obj->DistanceMm);
        free(obj);
        /* Return NULL if mutex creation fails */
        return NULL;
    }

    /* Return successfully allocated object */
    return obj;
}

UltraSonic_t *UltraSonic_CreateDefault(void) {
    /* Return default instance */
    return UltraSonic_Create(s_defaultSensors, (uint8_t)(sizeof(s_defaultSensors) / sizeof(s_defaultSensors[0])));
}

/*
 * @brief Frees memory and resources associated with the UltraSonic instance.
 * @param ptr Double pointer to the instance.
 * @return ReturnCode_t STAT_OKE on success.
 */
ReturnCode_t UltraSonic_Delete(UltraSonic_t **ptr) {
    if (ptr == NULL || *ptr == NULL) {
        /* Return error on invalid pointers */
        return STAT_ERR_INVALID_ARG;
    }

    if ((*ptr)->Lock != NULL) {
        vSemaphoreDelete((*ptr)->Lock);
        (*ptr)->Lock = NULL;
    }
    
    if ((*ptr)->DistanceMm != NULL) {
        free((*ptr)->DistanceMm);
        (*ptr)->DistanceMm = NULL;
    }

    free(*ptr);
    *ptr = NULL;
    
    /* Return success after freeing resources */
    return STAT_OKE;
}

/*
 * @brief Initializes the hardware pins for all registered sensors.
 * @param ptr Pointer to the instance.
 * @return ReturnCode_t STAT_OKE on success.
 */
ReturnCode_t UltraSonic_Init(UltraSonic_t *ptr) {
    if (ptr == NULL || ptr->Sensor == NULL || ptr->SensorCount == 0 || ptr->DistanceMm == NULL || ptr->Lock == NULL) {
        /* Return invalid argument error */
        return STAT_ERR_INVALID_ARG;
    }

    /* Control flow: securely acquire resource lock */
    if (xSemaphoreTake(ptr->Lock, pdMS_TO_TICKS(20)) != pdTRUE) {
        /* Return timeout error if lock acquisition fails */
        return STAT_ERR_TIMEOUT;
    }

    for (uint8_t i = 0; i < ptr->SensorCount; ++i) {
        ptr->DistanceMm[i] = 0xFFFF;
    }

    xSemaphoreGive(ptr->Lock);
    
    /* Return the status of hardware pin initialization */
    return HCSR04_InitPins(ptr);
}

/*
 * @brief Performs a full filtered read sequence on a specific sensor.
 * @param ptr Pointer to the instance.
 * @param index Target sensor index.
 * @param distanceMm Output pointer for filtered distance.
 * @return ReturnCode_t STAT_OKE on success.
 */
ReturnCode_t UltraSonic_ReadOne(UltraSonic_t *ptr, uint8_t index, uint16_t *distanceMm) {
    uint16_t filtered;

    if (ptr == NULL || distanceMm == NULL || index >= ptr->SensorCount) {
        /* Return invalid argument error */
        return STAT_ERR_INVALID_ARG;
    }

    filtered = HCSR04_ReadFiltered(ptr, index);

    /* Control flow: securely commit the filtered result into memory */
    if (xSemaphoreTake(ptr->Lock, pdMS_TO_TICKS(20)) != pdTRUE) {
        /* Return timeout error if lock acquisition fails */
        return STAT_ERR_TIMEOUT;
    }
    
    ptr->DistanceMm[index] = filtered;
    xSemaphoreGive(ptr->Lock);

    *distanceMm = filtered;
    
    /* Control flow: evaluate read success */
    if (filtered == 0xFFFF) {
        /* Return error if filter returned invalid data */
        return STAT_ERR_TIMEOUT;
    }

    /* Return success */
    return STAT_OKE;
}

/*
 * @brief Sequentially polls all active sensors and updates their memory values.
 * @param ptr Pointer to the instance.
 * @return ReturnCode_t STAT_OKE on success.
 */
ReturnCode_t UltraSonic_ReadAll(UltraSonic_t *ptr) {
    if (ptr == NULL || ptr->Sensor == NULL || ptr->DistanceMm == NULL || ptr->SensorCount == 0) {
        /* Return invalid state error */
        return STAT_ERR_INVALID_STATE;
    }

    /* Control flow: sequence through all mapped sensors */
    for (uint8_t i = 0; i < ptr->SensorCount; ++i) {
        uint16_t value = HCSR04_ReadFiltered(ptr, i);
        
        if (xSemaphoreTake(ptr->Lock, pdMS_TO_TICKS(20)) == pdTRUE) {
            ptr->DistanceMm[i] = value;
            xSemaphoreGive(ptr->Lock);
        }

        /* Enforce acoustic delay between testing individual transducers */
        if ((i + 1U) < ptr->SensorCount) {
            vTaskDelay(pdMS_TO_TICKS(HCSR04_INTER_SENSOR_DELAY_MS));
        }
    }

    /* Return success */
    return STAT_OKE;
}

/*
 * @brief Safely retrieves the last known distance of a specific sensor from memory.
 * @param ptr Pointer to the instance.
 * @param index Target sensor index.
 * @param distanceMm Output pointer.
 * @return ReturnCode_t STAT_OKE on success.
 */
ReturnCode_t UltraSonic_GetDistance(UltraSonic_t *ptr, uint8_t index, uint16_t *distanceMm) {
    if (ptr == NULL || distanceMm == NULL || index >= ptr->SensorCount) {
        /* Return invalid argument error */
        return STAT_ERR_INVALID_ARG;
    }

    /* Control flow: securely extract cached value */
    if (xSemaphoreTake(ptr->Lock, pdMS_TO_TICKS(50)) != pdTRUE) {
        /* Return timeout error if lock acquisition fails */
        return STAT_ERR_TIMEOUT;
    }

    *distanceMm = ptr->DistanceMm[index];
    xSemaphoreGive(ptr->Lock);
    
    /* Return success */
    return STAT_OKE;
}

uint8_t UltraSonic_GetCount(const UltraSonic_t *ptr) {
    if (ptr == NULL) {
        /* Return zero on null pointer */
        return 0;
    }

    /* Return the total sensor count */
    return ptr->SensorCount;
}
