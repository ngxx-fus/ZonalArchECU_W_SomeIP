#ifndef __HBRIDGE_MODULE_H__
#define __HBRIDGE_MODULE_H__

#include "../../AppESPWrap/All.h"
#include "../../AppUtils/All.h"
#include "./HBridge.h"

/* --- GLOBAL DATA EXPOSURE --- */

extern HBridge_t *          Motor;        /* Global Motor instance pointer */
extern SemaphoreHandle_t    MotorLock;    /* Mutex for synchronized motor access */

/* --- INTER-COMMUNICATION FUNCTIONS --- */

/// @brief Set speed for Motor 0 (Channel A)
/// @param Speed Target speed value (-1023 to 1023)
/// @return ReturnCode_t STAT_OKE if successful
ReturnCode_t MotorSetSpeed0(int Speed);

/// @brief Set speed for Motor 1 (Channel B)
/// @param Speed Target speed value (-1023 to 1023)
/// @return ReturnCode_t STAT_OKE if successful
ReturnCode_t MotorSetSpeed1(int Speed);

/// @brief Get current speed of Motor 0
/// @return int Current speed value
int MotorGetSpeed0(void);

/// @brief Get current speed of Motor 1
/// @return int Current speed value
int MotorGetSpeed1(void);

/* --- SERVICE RUNTIME --- */

/// @brief Main Motor Control Task
/// @param arg Task arguments
void MotorCtl(void* arg);

#endif