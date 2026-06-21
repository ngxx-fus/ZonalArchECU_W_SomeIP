/**************************************************************************************************
 * @file    Module.h
 * @author  Nguyen Thanh Phu (ngxxfus)
 * @date    Sun Jun 21 2026 1:50:40 PM
 * @brief   Hardware specific driver module
 **************************************************************************************************/

#ifndef __HBRIDGE_MODULE_H__
#define __HBRIDGE_MODULE_H__

#include "../../AppBase/All.h"
#include "../../AppESPWrap/All.h"
#include "./HBridge.h"

#ifndef MORTOR_LOG_EN
#define MORTOR_LOG_EN 1
#endif

/* Control parameters for blocking speed setting */
#define MOTOR_RETRIGGER_CHANGE_SPEED (1U /*1ms*/)
#define MOTOR_SET_SPEED_TIMEOUT_MS (3000U /*3sec*/)
#define MOTOR_ACQUIRE_RESOURCE_TIMEOUT_MS (pdMS_TO_TICKS(30000U /*30sec*/))

/* --- GLOBAL DATA EXPOSURE --- */

extern HBridge_t *Motor;			/* Global Motor instance pointer */
extern SemaphoreHandle_t MotorLock; /* Mutex for synchronized motor access */

/* --- INTER-COMMUNICATION FUNCTIONS --- */

/*
 * @brief Set speed for both motors with blocking mechanism
 * @param Speed0 Target speed value for Motor 0 (-1023 to 1023)
 * @param Speed1 Target speed value for Motor 1 (-1023 to 1023)
 * @return ReturnCode_t STAT_OKE if successful, STAT_ERR_TIMEOUT on timeout
 */
ReturnCode_t MotorSetSpeed(int16_t Speed0, int16_t Speed1);

/*
 * @brief Set speed for Motor 0 (Channel A) with blocking mechanism
 * @param Speed Target speed value (-1023 to 1023)
 * @return ReturnCode_t STAT_OKE if successful, STAT_ERR_TIMEOUT on timeout
 */
ReturnCode_t MotorSetSpeed0(int32_t Speed);

/*
 * @brief Set speed for Motor 1 (Channel B) with blocking mechanism
 * @param Speed Target speed value (-1023 to 1023)
 * @return ReturnCode_t STAT_OKE if successful, STAT_ERR_TIMEOUT on timeout
 */
ReturnCode_t MotorSetSpeed1(int32_t Speed);

/*
 * @brief Get current speed of Motor 0
 * @return int32_t Current speed value
 */
int32_t MotorGetSpeed0(void);

/*
 * @brief Get current speed of Motor 1
 * @return int32_t Current speed value
 */
int32_t MotorGetSpeed1(void);

/*
 * @brief Stop all motors for Emergency case
 * @return ReturnCode_t Status of the action
 */
ReturnCode_t EmergencyStop(void);

/* --- SERVICE RUNTIME --- */

/*
 * @brief Main Motor Control Task
 * @param arg Task arguments
 */
void MotorRuntime(void *arg);

#endif