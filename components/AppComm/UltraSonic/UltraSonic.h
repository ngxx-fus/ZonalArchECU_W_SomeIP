#ifndef __ULTRASONIC_H__
#define __ULTRASONIC_H__

#include <stdint.h>

#include "../__CommonHeaders.h"

typedef struct UltraSonicSensor_t {
	Pin_t Trig;
	Pin_t Echo;
} UltraSonicSensor_t;

typedef struct UltraSonic_t {
	const UltraSonicSensor_t *Sensor;
	uint8_t SensorCount;
	uint16_t *DistanceMm;
	SemaphoreHandle_t Lock;
} UltraSonic_t;

UltraSonic_t *UltraSonic_Create(const UltraSonicSensor_t *sensorCfg, uint8_t sensorCount);
UltraSonic_t *UltraSonic_CreateDefault(void);
ReturnCode_t UltraSonic_Delete(UltraSonic_t **ptr);
ReturnCode_t UltraSonic_Init(UltraSonic_t *ptr);
ReturnCode_t UltraSonic_ReadOne(UltraSonic_t *ptr, uint8_t index, uint16_t *distanceMm);
ReturnCode_t UltraSonic_ReadAll(UltraSonic_t *ptr);
ReturnCode_t UltraSonic_GetDistance(UltraSonic_t *ptr, uint8_t index, uint16_t *distanceMm);
uint8_t UltraSonic_GetCount(const UltraSonic_t *ptr);


#endif
