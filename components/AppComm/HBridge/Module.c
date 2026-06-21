/**************************************************************************************************
 * @file    Module.c
 * @author  Nguyen Thanh Phu (ngxxfus)
 * @date    Sun Jun 21 2026 1:49:07 PM
 * @brief   Hardware specific driver module
 **************************************************************************************************/

/**************************************************************************************************
 * INCLUDES
 **************************************************************************************************/
#include "Module.h"

/**************************************************************************************************
 * MACROS
 **************************************************************************************************/

#if (MORTOR_LOG_EN == 1)
#define MortorLog(...) SysLog(__VA_ARGS__)
#define MortorErr(...) SysErr(__VA_ARGS__)
#else
#define MortorLog(...)
#define MortorErr(...) SysErr(__VA_ARGS__)
#endif

#define ENABLE_EMERGENCY_ACTION 0

/**************************************************************************************************
 * PUBLIC VARIABLES
 **************************************************************************************************/

HBridge_t *Motor = NULL;			/* Global MFileHeaderCommenttor struct */
SemaphoreHandle_t MotorLock = NULL; /* Mutex for thread-safe Motor access */

/**************************************************************************************************
 * PRIVATE VARIABLES
 **************************************************************************************************/

/* Handle to wake up the motor runtime task immediately */
static TaskHandle_t MotorTaskHandle = NULL;
static volatile uint8_t IsEmergencyStopped =
	0; /* Flag to lock motor runtime when emergency occurs */

/**************************************************************************************************
 * INTERNAL HELPER FUNCTIONS
 **************************************************************************************************/

/*
 * @brief Wait and verify if target speed is applied to hardware
 * @param SelectMotor Speed selection mask (1: M0, 2: M1, 3: M0 & M1)
 * @param TargetSpeed0 Target speed for Motor 0
 * @param TargetSpeed1 Target speed for Motor 1
 * @param TimeoutMs Time to wait in milliseconds before checking
 * @return ReturnCode_t STAT_OKE if matched, STAT_ERR_TIMEOUT on failure
 */
static ReturnCode_t __IHF__WaitForSpeedAt(uint8_t SelectMotor,
										  int16_t TargetSpeed0,
										  int16_t TargetSpeed1,
										  uint32_t TimeoutMs) {
	/* Control flow: delay to allow runtime task to process hardware update */
	if (TimeoutMs > 0) {
		vTaskDelay(pdMS_TO_TICKS(TimeoutMs));
	}

	uint8_t IsMatched = 0;

	/* Control flow: attempt to acquire lock to read flags and speeds safely */
	if (xSemaphoreTake(MotorLock, MOTOR_ACQUIRE_RESOURCE_TIMEOUT_MS) ==
		pdTRUE) {
		/* Control flow: verify if speed modification flag has been cleared by
		 * runtime task */
		if (SafeFlagHas(&Motor->Flag, eHBridge_ModifiedSpeed) == 0) {
			/* Control flow: evaluate match condition based on motor selection
			 * mask */
			if (SelectMotor == 1) {
				/* Control flow: verify Motor 0 speed */
				if (Motor->Speed0 == TargetSpeed0) {
					IsMatched = 1;
				}
			} else if (SelectMotor == 2) {
				/* Control flow: verify Motor 1 speed */
				if (Motor->Speed1 == TargetSpeed1) {
					IsMatched = 1;
				}
			} else if (SelectMotor == 3) {
				/* Control flow: verify both motor speeds */
				if ((Motor->Speed0 == TargetSpeed0) &&
					(Motor->Speed1 == TargetSpeed1)) {
					IsMatched = 1;
				}
			}
		}
		xSemaphoreGive(MotorLock);
	}

	/* Control flow: evaluate final match result */
	if (IsMatched) {
		/* Jump statement: return success code */
		return STAT_OKE;
	}

	/* Jump statement: return timeout error code */
	return STAT_ERR_TIMEOUT;
}

/**************************************************************************************************
 * PUBLIC FUNCTIONS
 **************************************************************************************************/

/*
 * @brief Set speed for both motors with blocking mechanism
 * @param Speed0 Target speed value for Motor 0 (-1023 to 1023)
 * @param Speed1 Target speed value for Motor 1 (-1023 to 1023)
 * @return ReturnCode_t STAT_OKE if successful, STAT_ERR_TIMEOUT on timeout
 */
ReturnCode_t MotorSetSpeed(int16_t Speed0, int16_t Speed1) {
	uint32_t MaxRetries =
		(MOTOR_SET_SPEED_TIMEOUT_MS / MOTOR_RETRIGGER_CHANGE_SPEED);
	ReturnCode_t RetVal = STAT_ERR_TIMEOUT;

/* Conditional compilation: check emergency action configuration */
#if (ENABLE_EMERGENCY_ACTION == 1)
	/* Control flow: prevent operation if emergency stop is active */
	if (IsEmergencyStopped) {
		/* Jump statement: return error immediately due to emergency state */
		return STAT_ERR;
	}
#endif

	/* Control flow: prevent null pointer access */
	if (MotorLock == NULL || Motor == NULL) {
		/* Jump statement: return initialization error */
		return STAT_ERR_NULL;
	}

	/* Control flow: poll loop to set and verify target speed */
	for (uint32_t i = 0; i < MaxRetries; i++) {
		/* Control flow: acquire lock to conditionally trigger update */
		if (xSemaphoreTake(MotorLock, MOTOR_ACQUIRE_RESOURCE_TIMEOUT_MS) ==
			pdTRUE) {
			/* Control flow: retrigger HBridge update if current state does not
			 * match target */
			if (!((Motor->Speed0 == Speed0) && (Motor->Speed1 == Speed1) &&
				  (SafeFlagHas(&Motor->Flag, eHBridge_ModifiedSpeed) == 0))) {
				HBridge_SetSpeed(Motor, Speed0, Speed1);
			}
			xSemaphoreGive(MotorLock);
		}

		/* Control flow: trigger immediate wake-up for the motor task */
		if (MotorTaskHandle != NULL) {
			xTaskNotifyGive(MotorTaskHandle);
		}

		/* Control flow: wait and evaluate speed status using helper */
		if (__IHF__WaitForSpeedAt(3, Speed0, Speed1,
								  MOTOR_RETRIGGER_CHANGE_SPEED) == STAT_OKE) {
			RetVal = STAT_OKE;
			MortorLog("MotorSetSpeed(...): Updated Speed0 = %d, Speed1 = %d",
					  Speed0, Speed1);
			/* Jump statement: exit loop on success */
			break;
		}
	}

	/* Jump statement: return execution status */
	return RetVal;
}

/*
 * @brief Set speed for Motor 0 (Channel A) with blocking mechanism
 * @param Speed Target speed value (-1023 to 1023)
 * @return ReturnCode_t STAT_OKE if successful, STAT_ERR_TIMEOUT on timeout
 */
ReturnCode_t MotorSetSpeed0(int32_t Speed) {
	uint32_t MaxRetries =
		(MOTOR_SET_SPEED_TIMEOUT_MS / MOTOR_RETRIGGER_CHANGE_SPEED);
	ReturnCode_t RetVal = STAT_ERR_TIMEOUT;

/* Conditional compilation: check emergency action configuration */
#if (ENABLE_EMERGENCY_ACTION == 1)
	/* Control flow: prevent operation if emergency stop is active */
	if (IsEmergencyStopped) {
		/* Jump statement: return error immediately due to emergency state */
		return STAT_ERR;
	}
#endif

	/* Control flow: prevent null pointer access */
	if (MotorLock == NULL || Motor == NULL) {
		/* Jump statement: return initialization error */
		return STAT_ERR_NULL;
	}

	/* Control flow: poll loop to set and verify target speed */
	for (uint32_t i = 0; i < MaxRetries; i++) {
		/* Control flow: acquire lock to conditionally trigger update */
		if (xSemaphoreTake(MotorLock, MOTOR_ACQUIRE_RESOURCE_TIMEOUT_MS) ==
			pdTRUE) {
			/* Control flow: retrigger HBridge update if current state does not
			 * match target */
			if (!((Motor->Speed0 == (int16_t)Speed) &&
				  (SafeFlagHas(&Motor->Flag, eHBridge_ModifiedSpeed) == 0))) {
				HBridge_SetSpeed0(Motor, (int16_t)Speed);
			}
			xSemaphoreGive(MotorLock);
		}

		/* Control flow: trigger immediate wake-up for the motor task */
		if (MotorTaskHandle != NULL) {
			xTaskNotifyGive(MotorTaskHandle);
		}

		/* Control flow: wait and evaluate speed status using helper */
		if (__IHF__WaitForSpeedAt(1, (int16_t)Speed, 0,
								  MOTOR_RETRIGGER_CHANGE_SPEED) == STAT_OKE) {
			RetVal = STAT_OKE;
			MortorLog("MotorSetSpeed0(...): Updated Speed0 = %d", Speed);
			/* Jump statement: exit loop on success */
			break;
		}
	}

	/* Jump statement: return execution status */
	return RetVal;
}

/*
 * @brief Set speed for Motor 1 (Channel B) with blocking mechanism
 * @param Speed Target speed value (-1023 to 1023)
 * @return ReturnCode_t STAT_OKE if successful, STAT_ERR_TIMEOUT on timeout
 */
ReturnCode_t MotorSetSpeed1(int32_t Speed) {
	uint32_t MaxRetries =
		(MOTOR_SET_SPEED_TIMEOUT_MS / MOTOR_RETRIGGER_CHANGE_SPEED);
	ReturnCode_t RetVal = STAT_ERR_TIMEOUT;

/* Conditional compilation: check emergency action configuration */
#if (ENABLE_EMERGENCY_ACTION == 1)
	/* Control flow: prevent operation if emergency stop is active */
	if (IsEmergencyStopped) {
		/* Jump statement: return error immediately due to emergency state */
		return STAT_ERR;
	}
#endif

	/* Control flow: prevent null pointer access */
	if (MotorLock == NULL || Motor == NULL) {
		/* Jump statement: return initialization error */
		return STAT_ERR_NULL;
	}

	/* Control flow: poll loop to set and verify target speed */
	for (uint32_t i = 0; i < MaxRetries; i++) {
		/* Control flow: acquire lock to conditionally trigger update */
		if (xSemaphoreTake(MotorLock, MOTOR_ACQUIRE_RESOURCE_TIMEOUT_MS) ==
			pdTRUE) {
			/* Control flow: retrigger HBridge update if current state does not
			 * match target */
			if (!((Motor->Speed1 == (int16_t)Speed) &&
				  (SafeFlagHas(&Motor->Flag, eHBridge_ModifiedSpeed) == 0))) {
				HBridge_SetSpeed1(Motor, (int16_t)Speed);
			}
			xSemaphoreGive(MotorLock);
		}

		/* Control flow: trigger immediate wake-up for the motor task */
		if (MotorTaskHandle != NULL) {
			xTaskNotifyGive(MotorTaskHandle);
		}

		/* Control flow: wait and evaluate speed status using helper */
		if (__IHF__WaitForSpeedAt(2, 0, (int16_t)Speed,
								  MOTOR_RETRIGGER_CHANGE_SPEED) == STAT_OKE) {
			RetVal = STAT_OKE;
			MortorLog("MotorSetSpeed1(...): Updated Speed1 = %d", Speed);
			/* Jump statement: exit loop on success */
			break;
		}
	}

	/* Jump statement: return execution status */
	return RetVal;
}

/*
 * @brief Get current speed of Motor 0
 * @return int32_t Current speed value
 */
int32_t MotorGetSpeed0(void) {
	int32_t val = 0;

	/* Control flow: attempt to acquire resource lock for safe reading */
	if (MotorLock &&
		xSemaphoreTake(MotorLock, MOTOR_ACQUIRE_RESOURCE_TIMEOUT_MS) ==
			pdTRUE) {
		val = Motor->Speed0;
		xSemaphoreGive(MotorLock);
	}

	MortorLog("MotorGetSpeed0(...): Speed: %d", val);

	/* Jump statement: return the safely retrieved speed value */
	return val;
}

/*
 * @brief Get current speed of Motor 1
 * @return int32_t Current speed value
 */
int32_t MotorGetSpeed1(void) {
	int32_t val = 0;

	/* Control flow: attempt to acquire resource lock for safe reading */
	if (MotorLock &&
		xSemaphoreTake(MotorLock, MOTOR_ACQUIRE_RESOURCE_TIMEOUT_MS) ==
			pdTRUE) {
		val = Motor->Speed1;
		xSemaphoreGive(MotorLock);
	}

	MortorLog("MotorGetSpeed1(...): Speed: %d", val);

	/* Jump statement: return the safely retrieved speed value */
	return val;
}

/*
 * @brief Stop all motors for Emergency case
 * @return ReturnCode_t Status of the action
 */
ReturnCode_t EmergencyStop(void) {
/* Conditional compilation: check emergency action configuration */
#if (ENABLE_EMERGENCY_ACTION == 1)
	/* Control flow: prevent null pointer access */
	if (MotorLock == NULL || Motor == NULL) {
		/* Jump statement: return initialization error */
		return STAT_ERR_NULL;
	}

	/* Control flow: verify if already stopped to avoid redundant operations */
	if (IsEmergencyStopped) {
		MortorErr(
			"EmergencyStop(...): Motor is already in Emergency Stop state!");
		/* Jump statement: return success as it is already stopped */
		return STAT_OKE;
	}

	/* Control flow: lock resource and stop both motors immediately */
	if (xSemaphoreTake(MotorLock, MOTOR_ACQUIRE_RESOURCE_TIMEOUT_MS) ==
		pdTRUE) {
		IsEmergencyStopped = 1;
		HBridge_SetSpeed(Motor, 0, 0);
		HBridge_Apply(Motor); /* Force apply to hardware immediately */
		xSemaphoreGive(MotorLock);

		MortorLog("EmergencyStop(...): Motor stopped! (Emergency case)");

		/* Control flow: wake up task to acknowledge emergency */
		if (MotorTaskHandle != NULL) {
			xTaskNotifyGive(MotorTaskHandle);
		}

		/* Jump statement: return execution success */
		return STAT_OKE;
	}

	/* Jump statement: return failure if lock was not acquired */
	return STAT_ERR;
#else
	/* Jump statement: return success when emergency action is disabled */
	return STAT_OKE;
#endif
}

/**************************************************************************************************
 * SERVICE RUNTIME
 **************************************************************************************************/

/*
 * @brief Main Motor Control Task
 * @param arg Task arguments
 */
void MotorRuntime(void *arg) {
	SysEntry("MotorRuntime");

	/* Save current task handle for notification wake-ups */
	MotorTaskHandle = xTaskGetCurrentTaskHandle();

	/* Control flow: wait until global initialization allows proceeding */
	while (1 >= GlobalInit_GetLevel()) {
		vTaskDelay(pdMS_TO_TICKS(50));
	}

	/* Initialize mutex before creating motor object */
	MotorLock = xSemaphoreCreateMutex();

	MortorLog("[MotorRuntime] New motor object...");
	Motor =
		HBridge_Create(PIN_B_DC_IN0, PIN_B_DC_IN1, PIN_B_DC_IN2, PIN_B_DC_IN3);

	/* Control flow: verify if motor creation was successful */
	if (IsNull(Motor)) {
		MortorErr("[MotorRuntime] Cannot initialize `Motor`!");
		vTaskDelete(NULL);

		/* Jump statement: return to terminate task execution */
		return;
	}

	MotorSetSpeed0(0);
	MotorSetSpeed1(0);

	/* Control flow: safely apply initial states */
	if (xSemaphoreTake(MotorLock, MOTOR_ACQUIRE_RESOURCE_TIMEOUT_MS) ==
		pdTRUE) {
		HBridge_Apply(Motor);
		xSemaphoreGive(MotorLock);
	}

	GlobalInit_MoveNextLevel();

	MortorLog("[MotorRuntime] Join forever loop...");

	/* Control flow: enter infinite task loop */
	while (1) {
		/* Wait for notification to wake up immediately, or timeout after 20ms
		 */
		ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(20));

/* Conditional compilation: check if ECU is in emergency state */
#if (ENABLE_EMERGENCY_ACTION == 1)
		/* Control flow: verify emergency status */
		if (IsEmergencyStopped) {
			/* Jump statement: continue loop immediately to block execution */
			continue;
		}
#endif

		/* Control flow: attempt to lock mutex to check and apply flags */
		if (xSemaphoreTake(MotorLock, pdMS_TO_TICKS(10)) == pdTRUE) {
			/* Control flow: apply changes to hardware if flag is raised */
			if (Motor->Flag != 0) {
				HBridge_Apply(Motor);
				Motor->Flag = 0; /* Clear all flags after applying */
			}
			xSemaphoreGive(MotorLock);
		}
	}
}