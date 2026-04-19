#ifndef __H_BRIDGE_H__
#define __H_BRIDGE_H__

#include "stdint.h"
#include <stdlib.h>
#include "driver/ledc.h"

/* Project dependency headers */
#include "../../AppUtils/FlagControl.h"
#include "../../AppUtils/ReturnType.h"
#include "../../AppConfig/Pinout/Pinout.h"

/// @brief Status flags for H-Bridge operations
enum HBridge_e {
    eHBridge_ModifiedPin    = 0,
    eHBridge_ModifiedSpeed  = 1,
    eHBridge_SwapOutput     = 28,
    eHBridge_NoPWM          = 29,
    eHBridge_PWM_8bit       = 30,
    eHBridge_PWM_10bit      = 31   /* 10-bit (0-1023) resolution */
};

/// @brief H-Bridge instance structure with independent dual-timer configuration
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
    struct {
        ledc_mode_t      Mode;
        ledc_timer_bit_t Resolution;
        ledc_timer_t     Timer[2];      /* Timer 0 for Motor A, Timer 1 for Motor B */
        uint32_t         Frequency[2];  /* Independent frequencies */
        ledc_channel_t   Channels[4];   /* 4 channels for IN0-IN3 */
    } Config;
    SafeFlag_t Flag; 
};
typedef struct HBridge_t HBridge_t;

/* Public API Functions */
HBridge_t *     HBridge_Create(Pin_t IN0, Pin_t IN1, Pin_t IN2, Pin_t IN3);
ReturnCode_t    HBridge_Apply(HBridge_t * Ptr);
ReturnCode_t    HBridge_SetSpeed(HBridge_t * Ptr, int16_t Speed0, int16_t Speed1);
ReturnCode_t    HBridge_SetSpeed0(HBridge_t * Ptr, int16_t Speed);
ReturnCode_t    HBridge_SetSpeed1(HBridge_t * Ptr, int16_t Speed);
#endif