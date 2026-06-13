#include "HBridge.h"
#include "driver/gpio.h"
#include <string.h>

/* --- INTERNAL HELPER FUNCTIONS --- */

/// @brief Configure a specific LEDC timer
/// @param bridge Pointer to HBridge instance
/// @param Index Timer index (0 or 1)
static void __IHF__SetTimer(HBridge_t * bridge, uint8_t Index) {
    /* this is a comment - setup specific timer configuration */
    ledc_timer_config_t timer_conf = {
        .speed_mode      = bridge->Config.Mode,
        .duty_resolution = bridge->Config.Resolution,
        .timer_num       = bridge->Config.Timer[Index],
        .freq_hz         = bridge->Config.Frequency[Index],
        .clk_cfg         = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer_conf);
}

/// @brief Register a GPIO pin and attach it to the correct motor timer
/// @param bridge Pointer to HBridge instance
/// @param Index Pin index (0-3)
static void __IHF__RegisterGPIO(HBridge_t * bridge, uint8_t Index) {
    /* this is a comment - determine which timer controls this pin */
    ledc_timer_t SelectedTimer = (Index < 2) ? bridge->Config.Timer[0] : bridge->Config.Timer[1];
    
    gpio_reset_pin(bridge->IN[Index]);
    
    /* this is a comment - map channel to the selected motor timer */
    ledc_channel_config_t ledc_ch = {
        .gpio_num   = bridge->IN[Index],
        .speed_mode = bridge->Config.Mode,
        .channel    = bridge->Config.Channels[Index],
        .timer_sel  = SelectedTimer,
        .duty       = 0,
        .hpoint     = 0
    };
    ledc_channel_config(&ledc_ch);
}

/* --- PUBLIC FUNCTIONS --- */

HBridge_t * HBridge_Create(Pin_t IN0, Pin_t IN1, Pin_t IN2, Pin_t IN3) {
    HBridge_t * Ptr = (HBridge_t *)malloc(sizeof(HBridge_t));
    if (Ptr == NULL) {
        return NULL;
    }

    Ptr->IN0 = IN0; Ptr->IN1 = IN1;
    Ptr->IN2 = IN2; Ptr->IN3 = IN3;
    
    /* this is a comment - default config: Motor A (1kHz), Motor B (2kHz) */
    Ptr->Config.Mode = LEDC_LOW_SPEED_MODE;
    Ptr->Config.Resolution = LEDC_TIMER_10_BIT;
    
    Ptr->Config.Timer[0] = LEDC_TIMER_0;
    Ptr->Config.Frequency[0] = 1000;
    
    Ptr->Config.Timer[1] = LEDC_TIMER_1;
    Ptr->Config.Frequency[1] = 1000;

    for (int32_t i = 0; i < 4; i++) {
        Ptr->Config.Channels[i] = (ledc_channel_t)i;
        Ptr->PrevIN[i] = 0xFF;
    }

    SafeFlagSet(&Ptr->Flag, eHBridge_ModifiedPin);
    SafeFlagSet(&Ptr->Flag, eHBridge_ModifiedSpeed);
    SafeFlagSet(&Ptr->Flag, eHBridge_PWM_10bit);

    (void)HBridge_Apply(Ptr);
    return Ptr;
}

ReturnCode_t HBridge_Apply(HBridge_t * Ptr) {
    /* Control flow: check for null pointer before proceeding */
    if (Ptr == NULL) {
        SysLog("HBridge_Apply(...): Null pointer provided");
        /* Control flow: return error due to null pointer */
        return STAT_ERR_NULL;
    }

    /* Control flow: update both timers and re-register all pins if pin flag is set */
    if (SafeFlagHas(&Ptr->Flag, eHBridge_ModifiedPin)) {
        SysLog("HBridge_Apply(...): Pin configuration modified, applying changes");
        __IHF__SetTimer(Ptr, 0);
        __IHF__SetTimer(Ptr, 1);
        
        /* Control flow: iterate through all 4 GPIO channels to register them */
        for (int32_t i = 0; i < 4; i++) {
            __IHF__RegisterGPIO(Ptr, i);
        }
        SafeFlagClear(&Ptr->Flag, eHBridge_ModifiedPin);
    }

    /* Control flow: apply duty cycles based on motor speeds if speed flag is set */
    if (SafeFlagHas(&Ptr->Flag, eHBridge_ModifiedSpeed)) {
        SysLog("HBridge_Apply(...): Speed modified, applying duty cycles");
        uint32_t MaxDuty = (1UL << Ptr->Config.Resolution) - 1;

        /* Control flow: update forward/reverse channels for both motors */
        for (int32_t i = 0; i < 2; i++) {
            uint8_t Base = i * 2;
            
            /* Control flow: determine forward and reverse duty values */
            uint32_t Fwd = (Ptr->Speed[i] > 0) ? (uint32_t)Ptr->Speed[i] : 0;
            uint32_t Rev = (Ptr->Speed[i] < 0) ? (uint32_t)(-Ptr->Speed[i]) : 0;

            /* Control flow: set and update duty for forward channel */
            ledc_set_duty(Ptr->Config.Mode, Ptr->Config.Channels[Base], (Fwd > MaxDuty) ? MaxDuty : Fwd);
            ledc_update_duty(Ptr->Config.Mode, Ptr->Config.Channels[Base]);
            
            /* Control flow: set and update duty for reverse channel */
            ledc_set_duty(Ptr->Config.Mode, Ptr->Config.Channels[Base + 1], (Rev > MaxDuty) ? MaxDuty : Rev);
            ledc_update_duty(Ptr->Config.Mode, Ptr->Config.Channels[Base + 1]);
        }
        SafeFlagClear(&Ptr->Flag, eHBridge_ModifiedSpeed);
    }
    
    /* Control flow: return success after processing all flags */
    return STAT_OKE;
}

/// @brief Update speed for Motor 0
/// @param Ptr Pointer to HBridge instance
/// @param Speed Speed for Motor 0 (-1023 to 1023)
/// @return ReturnCode_t Result status
ReturnCode_t HBridge_SetSpeed0(HBridge_t * Ptr, int16_t Speed) {
    if (Ptr == NULL) {
        return STAT_ERR_NULL;
    }

    /* Update speed0 and mark as modified */
    Ptr->Speed0 = Speed;
    SafeFlagSet(&Ptr->Flag, eHBridge_ModifiedSpeed);

    return STAT_OKE;
}

/// @brief Update speed for Motor 1
/// @param Ptr Pointer to HBridge instance
/// @param Speed Speed for Motor 1 (-1023 to 1023)
/// @return ReturnCode_t Result status
ReturnCode_t HBridge_SetSpeed1(HBridge_t * Ptr, int16_t Speed) {
    if (Ptr == NULL) {
        return STAT_ERR_NULL;
    }

    /* Update speed1 and mark as modified */
    Ptr->Speed1 = Speed;
    SafeFlagSet(&Ptr->Flag, eHBridge_ModifiedSpeed);

    return STAT_OKE;
}

/// @brief Update both motor speeds and mark for update
/// @param Ptr Pointer to HBridge instance
/// @param Speed0 Speed for Motor 0 (-1023 to 1023)
/// @param Speed1 Speed for Motor 1 (-1023 to 1023)
/// @return ReturnCode_t Result status
ReturnCode_t HBridge_SetSpeed(HBridge_t * Ptr, int16_t Speed0, int16_t Speed1) {
    if (Ptr == NULL) {
        return STAT_ERR_NULL;
    }

    /* Update both internal speed values and set modify flag */
    Ptr->Speed0 = Speed0;
    Ptr->Speed1 = Speed1;
    SafeFlagSet(&Ptr->Flag, eHBridge_ModifiedSpeed);

    return STAT_OKE;
}