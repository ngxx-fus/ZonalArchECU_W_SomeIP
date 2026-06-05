#include "UltraSonic.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#define HCSR04_TRIGGER_PULSE_US         10
#define HCSR04_ECHO_TIMEOUT_US          30000
#define HCSR04_INTER_SENSOR_DELAY_MS    15

static const Pin_t s_trigPin = PIN_US_TRIG;

static const UltraSonicSensor_t s_defaultSensors[] = {
	{ PIN_US_TRIG, PIN_US_L_ECHO },
	{ PIN_US_TRIG, PIN_US_C_ECHO },
	{ PIN_US_TRIG, PIN_US_R_ECHO },
};

static ReturnCode_t HCSR04_InitPins(UltraSonic_t *ptr) {
	uint64_t echoMask = 0ULL;
	uint8_t i;

	if (ptr == NULL || ptr->Sensor == NULL || ptr->SensorCount == 0) {
		return STAT_ERR_INVALID_ARG;
	}

	if (!IsValidPin(s_trigPin)) {
		return STAT_ERR_INVALID_ARG;
	}

	for (i = 0; i < ptr->SensorCount; ++i) {
		if (!IsValidPin(ptr->Sensor[i].Echo)) {
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

	if (gpio_config(&trig_cfg) != ESP_OK || gpio_config(&echo_cfg) != ESP_OK) {
		return STAT_ERR_IO;
	}

	gpio_set_level((gpio_num_t)s_trigPin, 0);
	return STAT_OKE;
}

static ReturnCode_t HCSR04_ReadOneRaw(UltraSonic_t *ptr, uint8_t index, uint32_t *distance_mm) {
	int64_t t0;
	int64_t timeout_at;
	gpio_num_t echoPin;

	if (ptr == NULL || ptr->Sensor == NULL || distance_mm == NULL || index >= ptr->SensorCount) {
		return STAT_ERR_INVALID_ARG;
	}

	echoPin = (gpio_num_t)ptr->Sensor[index].Echo;

	gpio_set_level((gpio_num_t)s_trigPin, 0);
	esp_rom_delay_us(2);
	gpio_set_level((gpio_num_t)s_trigPin, 1);
	esp_rom_delay_us(HCSR04_TRIGGER_PULSE_US);
	gpio_set_level((gpio_num_t)s_trigPin, 0);

	timeout_at = esp_timer_get_time() + HCSR04_ECHO_TIMEOUT_US;
	while (gpio_get_level(echoPin) == 0) {
		if (esp_timer_get_time() > timeout_at) {
			SysLog("[HCSR04] sensor %u timeout waiting ECHO HIGH (ECHO=%d)",
				   (unsigned)index, (int32_t)echoPin);
			return STAT_ERR_TIMEOUT;
		}
	}

	t0 = esp_timer_get_time();
	timeout_at = t0 + HCSR04_ECHO_TIMEOUT_US;
	while (gpio_get_level(echoPin) == 1) {
		if (esp_timer_get_time() > timeout_at) {
			SysLog("[HCSR04] sensor %u timeout waiting ECHO LOW (ECHO=%d)",
				   (unsigned)index, (int32_t)echoPin);
			return STAT_ERR_TIMEOUT;
		}
	}

	{
		int64_t pulse_us = esp_timer_get_time() - t0;
		if (pulse_us < 0) {
			pulse_us = 0;
		}
		*distance_mm = (uint32_t)((pulse_us * 1000LL) / 5820LL);
	}

	return STAT_OKE;
}

static uint16_t HCSR04_ReadFiltered(UltraSonic_t *ptr, uint8_t index) {
	uint32_t d0 = 0, d1 = 0, d2 = 0;
	uint32_t samples[3];
	uint8_t valid = 0;

	if (HCSR04_ReadOneRaw(ptr, index, &d0) == STAT_OKE) samples[valid++] = d0;
	vTaskDelay(pdMS_TO_TICKS(5));
	if (HCSR04_ReadOneRaw(ptr, index, &d1) == STAT_OKE) samples[valid++] = d1;
	vTaskDelay(pdMS_TO_TICKS(5));
	if (HCSR04_ReadOneRaw(ptr, index, &d2) == STAT_OKE) samples[valid++] = d2;

	if (valid == 0) return 0xFFFF;
	if (valid == 1) return (samples[0] > 65535U) ? 65535U : (uint16_t)samples[0];
	if (valid == 2) {
		uint32_t avg = (samples[0] + samples[1]) / 2U;
		return (avg > 65535U) ? 65535U : (uint16_t)avg;
	}

	if ((samples[0] > samples[1]) != (samples[0] > samples[2])) {
		return (samples[0] > 65535U) ? 65535U : (uint16_t)samples[0];
	}
	if ((samples[1] > samples[0]) != (samples[1] > samples[2])) {
		return (samples[1] > 65535U) ? 65535U : (uint16_t)samples[1];
	}
	return (samples[2] > 65535U) ? 65535U : (uint16_t)samples[2];
}


UltraSonic_t *UltraSonic_Create(const UltraSonicSensor_t *sensorCfg, uint8_t sensorCount) {
	UltraSonic_t *obj;

	if (sensorCfg == NULL || sensorCount == 0) {
		return NULL;
	}

	obj = (UltraSonic_t *)calloc(1, sizeof(UltraSonic_t));
	if (obj == NULL) {
		return NULL;
	}

	obj->DistanceMm = (uint16_t *)calloc(sensorCount, sizeof(uint16_t));
	if (obj->DistanceMm == NULL) {
		free(obj);
		return NULL;
	}

	obj->Sensor = sensorCfg;
	obj->SensorCount = sensorCount;
	obj->Lock = xSemaphoreCreateMutex();
	if (obj->Lock == NULL) {
		free(obj->DistanceMm);
		free(obj);
		return NULL;
	}

	return obj;
}

UltraSonic_t *UltraSonic_CreateDefault(void) {
	return UltraSonic_Create(s_defaultSensors, (uint8_t)(sizeof(s_defaultSensors) / sizeof(s_defaultSensors[0])));
}

ReturnCode_t UltraSonic_Delete(UltraSonic_t **ptr) {
	if (ptr == NULL || *ptr == NULL) {
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
	return STAT_OKE;
}

ReturnCode_t UltraSonic_Init(UltraSonic_t *ptr) {
	if (ptr == NULL || ptr->Sensor == NULL || ptr->SensorCount == 0 || ptr->DistanceMm == NULL || ptr->Lock == NULL) {
		return STAT_ERR_INVALID_ARG;
	}

	if (xSemaphoreTake(ptr->Lock, pdMS_TO_TICKS(20)) != pdTRUE) {
		return STAT_ERR_TIMEOUT;
	}

	for (uint8_t i = 0; i < ptr->SensorCount; ++i) {
		ptr->DistanceMm[i] = 0xFFFF;
	}

	xSemaphoreGive(ptr->Lock);
	return HCSR04_InitPins(ptr);
}

ReturnCode_t UltraSonic_ReadOne(UltraSonic_t *ptr, uint8_t index, uint16_t *distanceMm) {
	uint16_t filtered;

	if (ptr == NULL || distanceMm == NULL || index >= ptr->SensorCount) {
		return STAT_ERR_INVALID_ARG;
	}

	filtered = HCSR04_ReadFiltered(ptr, index);

	if (xSemaphoreTake(ptr->Lock, pdMS_TO_TICKS(20)) != pdTRUE) {
		return STAT_ERR_TIMEOUT;
	}
	ptr->DistanceMm[index] = filtered;
	xSemaphoreGive(ptr->Lock);

	*distanceMm = filtered;
	if (filtered == 0xFFFF) {
		return STAT_ERR_TIMEOUT;
	}

	return STAT_OKE;
}

ReturnCode_t UltraSonic_ReadAll(UltraSonic_t *ptr) {
	if (ptr == NULL || ptr->Sensor == NULL || ptr->DistanceMm == NULL || ptr->SensorCount == 0) {
		return STAT_ERR_INVALID_STATE;
	}

	for (uint8_t i = 0; i < ptr->SensorCount; ++i) {
		uint16_t value = HCSR04_ReadFiltered(ptr, i);
		if (xSemaphoreTake(ptr->Lock, pdMS_TO_TICKS(20)) == pdTRUE) {
			ptr->DistanceMm[i] = value;
			xSemaphoreGive(ptr->Lock);
		}

		if ((i + 1U) < ptr->SensorCount) {
			vTaskDelay(pdMS_TO_TICKS(HCSR04_INTER_SENSOR_DELAY_MS));
		}
	}

	return STAT_OKE;
}

ReturnCode_t UltraSonic_GetDistance(UltraSonic_t *ptr, uint8_t index, uint16_t *distanceMm) {
	if (ptr == NULL || distanceMm == NULL || index >= ptr->SensorCount) {
		return STAT_ERR_INVALID_ARG;
	}

	if (xSemaphoreTake(ptr->Lock, pdMS_TO_TICKS(10)) != pdTRUE) {
		return STAT_ERR_TIMEOUT;
	}

	*distanceMm = ptr->DistanceMm[index];
	xSemaphoreGive(ptr->Lock);
	return STAT_OKE;
}

uint8_t UltraSonic_GetCount(const UltraSonic_t *ptr) {
	if (ptr == NULL) {
		return 0;
	}

	return ptr->SensorCount;
}
