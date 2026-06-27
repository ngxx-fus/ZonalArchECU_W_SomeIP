#ifndef __ULTRASONIC_MODULE_H__
#define __ULTRASONIC_MODULE_H__

#include "./UltraSonic.h"


/// @brief Get the latest read distance for a specific sensor.
/// @param index Sensor index (0, 1, 2, ...)
/// @return Distance in millimeters, or 0xFFFF if not available.
uint16_t HCSR04_GetLatestDistanceMm(uint8_t index);

/// @brief Main Ultrasonic Control Task - continuously reads 3 HCSR04 sensors
/// @param arg Task arguments (unused)
void HCSR04Runtime(void *arg);


#endif
