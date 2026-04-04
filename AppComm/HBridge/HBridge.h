#ifndef __H_BRIDGE_H__
#define __H_BRIDGE_H__

/*Standard Lib-C*/
#include "stdint.h"
#include <stdlib.h>

/*Project def*/
#include "../../AppUtils/Bitwise.h"
#include "../../AppUtils/ReturnType.h"
#include "../../AppUtils/Arithmetic.h"
#include "../../AppUtils/FlagControl.h"
#include "../../AppConfig/Pinout/Pinout.h"

/* ----------------------------------------------------------------------------
 * L298N Mini Truth Table (Direct PWM Mapping)
 * ----------------------------------------------------------------------------
 * Motor |  INx  |  INy  |   VOUT Status   | Description
 * ----------------------------------------------------------------------------
 * A     |  IN0  |  IN1  |   0     |   0   | Coast (Motor stops slowly)
 * A     |  PWM  |   0   |   V+    |   0   | Forward (Speed = PWM Duty)
 * A     |   0   |  PWM  |   0     |   V+  | Reverse (Speed = PWM Duty)
 * A     |   1   |   1   |   V+    |   V+  | Brake (Motor stops immediately)
 * ----------------------------------------------------------------------------
 * B     |  IN2  |  IN3  |   0     |   0   | Coast
 * B     |  PWM  |   0   |   V+    |   0   | Forward
 * B     |   0   |  PWM  |   0     |   V+  | Reverse
 * B     |   1   |   1   |   V+    |   V+  | Brake
 * ----------------------------------------------------------------------------
 */

/// @brief Status flags for H-Bridge operations
enum HBridge_e {
    eHBridge_ModifiedPin    = 0,
    eHBridge_ModifiedSpeed  = 1,
    eHBridge_SwapOutput     = 28,
    eHBridge_NoPWM          = 29,
    eHBridge_PWM_8bit       = 30,
    eHBridge_PWM_12bit      = 31
};

/// @brief H-Bridge instance structure
__attribute__((packed)) struct HBridge_t {
    union {
        struct {
            uint8_t IN0; uint8_t IN1;
            uint8_t IN2; uint8_t IN3;
        };
        uint8_t IN[4];
    };
    uint8_t PrevIN[4];
    union {
        struct {
            int16_t Speed0;
            int16_t Speed1;
        };
        int16_t Speed[2];
    };
    SafeFlag_t Flag; 
};
typedef struct HBridge_t HBridge_t;

/// @brief Create H-Bridge instance and initialize pins
/// @param IN0 Pin for Motor A Forward
/// @param IN1 Pin for Motor A Reverse
/// @param IN2 Pin for Motor B Forward
/// @param IN3 Pin for Motor B Reverse
/// @return Pointer to HBridge_t instance or NULL if failed
HBridge_t * HBridge_Create(Pin_t IN0, Pin_t IN1, Pin_t IN2, Pin_t IN3);

/// @brief Destroy H-Bridge instance and release hardware pins
/// @param Ptr Double pointer to instance for dangling pointer prevention
/// @return STAT_OKE on success, error code otherwise
ReturnCode_t HBridge_Free(HBridge_t ** Ptr);

/// @brief Clear modified flags from the instance
/// @param Ptr Pointer to H-Bridge instance
/// @return STAT_OKE on success
ReturnCode_t HBridge_ClearFlag(HBridge_t * Ptr);

/// @brief Update motor control pins
/// @param Ptr Pointer to H-Bridge instance
/// @param IN0-IN3 New pin assignments
/// @return STAT_OKE on success
ReturnCode_t HBridge_SetPin(HBridge_t * Ptr, Pin_t IN0, Pin_t IN1, Pin_t IN2, Pin_t IN3);

/// @brief Set speeds for both motors
/// @param Ptr Pointer to H-Bridge instance
/// @param Speed0 Speed for Motor A
/// @param Speed1 Speed for Motor B
/// @return STAT_OKE on success
ReturnCode_t HBridge_SetSpeed(HBridge_t * Ptr, int16_t Speed0, int16_t Speed1);

/// @brief Set speed for Motor A
/// @param Ptr Pointer to H-Bridge instance
/// @param Speed Target speed
/// @return STAT_OKE on success
ReturnCode_t HBridge_SetSpeed0(HBridge_t * Ptr, int16_t Speed);

/// @brief Set speed for Motor B
/// @param Ptr Pointer to H-Bridge instance
/// @param Speed Target speed
/// @return STAT_OKE on success
ReturnCode_t HBridge_SetSpeed1(HBridge_t * Ptr, int16_t Speed);

/// @brief Apply settings to hardware registers
/// @param Ptr Pointer to H-Bridge instance
/// @return STAT_OKE on success
ReturnCode_t HBridge_Apply(HBridge_t * Ptr);

#endif /// __H_BRIDGE_H__