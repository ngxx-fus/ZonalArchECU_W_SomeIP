#include "HBridge.h"

/* ESP-IDF Standard Drivers */
#include "driver/ledc.h"
#include "driver/gpio.h"

/* Motor Control Constants */
#define LEDC_HS_MODE      LEDC_HIGH_SPEED_MODE
#define LEDC_HS_TIMER     LEDC_TIMER_0
#define LEDC_FREQUENCY    10 

/* --- INTERNAL HELPER FUNCTIONS --- */

static inline void __IHF__RegisterGPIO(uint8_t Pin, uint8_t Channel) {

    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,     
        .mode = GPIO_MODE_INPUT_OUTPUT,    
        .pin_bit_mask = (1ULL << Pin),      
        .pull_down_en = GPIO_PULLDOWN_ENABLE, 
        .pull_up_en = GPIO_PULLUP_DISABLE
    };
    gpio_config(&io_conf);

    /* Driver internally handles GPIO Matrix and IO MUX */
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_HS_MODE,
        .channel        = (ledc_channel_t)Channel,
        .timer_sel      = LEDC_HS_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = Pin,
        .duty           = 0,
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel);
}

static inline void __IHF__ReleaseGPIO(uint8_t Pin) {
    if (Pin >= 40) return;
    /* Cleanly reset pin and disconnect from LEDC */
    gpio_reset_pin(Pin);
}

static void __IHF__SetPWMResolution(uint32_t Bits) {
    /* ledc_timer_config internally handles peripheral clock enabling */
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_HS_MODE,
        .duty_resolution  = (ledc_timer_bit_t)Bits,
        .timer_num        = LEDC_HS_TIMER,
        .freq_hz          = LEDC_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);
}

static void __IHF__SetPWM(uint8_t Channel, uint32_t Duty, uint8_t NoPWM, uint32_t MaxDuty) {
    uint32_t FinalDuty = (NoPWM) ? ((Duty > 0) ? MaxDuty : 0) : ((Duty > MaxDuty) ? MaxDuty : Duty);

    /* Update hardware registers via driver */
    ledc_set_duty(LEDC_HS_MODE, (ledc_channel_t)Channel, FinalDuty);
    ledc_update_duty(LEDC_HS_MODE, (ledc_channel_t)Channel);
}

/* --- PUBLIC FUNCTIONS --- */

HBridge_t * HBridge_Create(Pin_t IN0, Pin_t IN1, Pin_t IN2, Pin_t IN3) {
    HBridge_t * Ptr = (HBridge_t *)malloc(sizeof(HBridge_t));
    if (Ptr == NULL) return NULL;

    Ptr->IN0 = IN0; Ptr->IN1 = IN1;
    Ptr->IN2 = IN2; Ptr->IN3 = IN3;
    for (int i = 0; i < 4; i++) Ptr->PrevIN[i] = 0xFF;

    SafeFlagSet(&Ptr->Flag, eHBridge_ModifiedPin);
    SafeFlagSet(&Ptr->Flag, eHBridge_ModifiedSpeed);
    SafeFlagSet(&Ptr->Flag, eHBridge_PWM_8bit);

    /* Apply calls IHF functions which use ledc_timer_config to enable clock */
    (void)HBridge_Apply(Ptr);
    return Ptr;
}

ReturnCode_t HBridge_Free(HBridge_t ** Ptr) {
    /* Text */
    if (IsNull(Ptr) || IsNull(*Ptr)) return STAT_ERR_NULL;

    /* Stop everything */
    (*Ptr)->Speed0 = 0; (*Ptr)->Speed1 = 0;
    SafeFlagSet(&(*Ptr)->Flag, eHBridge_ModifiedSpeed);
    (void)HBridge_Apply(*Ptr);

    for (int i = 0; i < 4; i++) __IHF__ReleaseGPIO((*Ptr)->IN[i]);

    free(*Ptr);
    *Ptr = (HBridge_t *)NULL;
    return STAT_OKE;
}

ReturnCode_t HBridge_ClearFlag(HBridge_t * Ptr) {
    /* Text */
    if (IsNull(Ptr)) return STAT_ERR_NULL;
    SafeFlagClear(&Ptr->Flag, eHBridge_ModifiedPin);
    SafeFlagClear(&Ptr->Flag, eHBridge_ModifiedSpeed);
    return STAT_OKE;
}

ReturnCode_t HBridge_SetPin(HBridge_t * Ptr, Pin_t IN0, Pin_t IN1, Pin_t IN2, Pin_t IN3) {
    /* Text */
    if (IsNull(Ptr)) return STAT_ERR_NULL;
    for (int i = 0; i < 4; i++) Ptr->PrevIN[i] = Ptr->IN[i];
    Ptr->IN0 = IN0; Ptr->IN1 = IN1;
    Ptr->IN2 = IN2; Ptr->IN3 = IN3;
    SafeFlagSet(&Ptr->Flag, eHBridge_ModifiedPin);
    return STAT_OKE;
}

ReturnCode_t HBridge_SetSpeed(HBridge_t * Ptr, int16_t Speed0, int16_t Speed1) {
    /* Text */
    if (IsNull(Ptr)) return STAT_ERR_NULL;
    Ptr->Speed0 = Speed0; Ptr->Speed1 = Speed1;
    SafeFlagSet(&Ptr->Flag, eHBridge_ModifiedSpeed);
    return STAT_OKE;
}

ReturnCode_t HBridge_SetSpeed0(HBridge_t * Ptr, int16_t Speed) {
    /* Text */
    if (IsNull(Ptr)) return STAT_ERR_NULL;
    Ptr->Speed0 = Speed;
    SafeFlagSet(&Ptr->Flag, eHBridge_ModifiedSpeed);
    return STAT_OKE;
}

ReturnCode_t HBridge_SetSpeed1(HBridge_t * Ptr, int16_t Speed) {
    /* Text */
    if (IsNull(Ptr)) return STAT_ERR_NULL;
    Ptr->Speed1 = Speed;
    SafeFlagSet(&Ptr->Flag, eHBridge_ModifiedSpeed);
    return STAT_OKE;
}

ReturnCode_t HBridge_Apply(HBridge_t * Ptr) {
    if (Ptr == NULL) return STAT_ERR_NULL;

    if (SafeFlagHas(&Ptr->Flag, eHBridge_ModifiedPin)) {
        for (int i = 0; i < 4; i++) {
            __IHF__ReleaseGPIO(Ptr->PrevIN[i]);
            __IHF__RegisterGPIO(Ptr->IN[i], i);
        }
        SafeFlagClear(&Ptr->Flag, eHBridge_ModifiedPin);
    }

    if (SafeFlagHas(&Ptr->Flag, eHBridge_ModifiedSpeed)) {
        uint8_t IsSwap = SafeFlagHas(&Ptr->Flag, eHBridge_SwapOutput);
        uint8_t NoPWM  = SafeFlagHas(&Ptr->Flag, eHBridge_NoPWM);
        uint32_t Res   = SafeFlagHas(&Ptr->Flag, eHBridge_PWM_12bit) ? 12 : 8;
        uint32_t MaxDuty = (1UL << Res) - 1;

        __IHF__SetPWMResolution(Res);

        for (int i = 0; i < 2; i++) {
            uint8_t MotorIdx = (i == 0) ? (IsSwap ? 1 : 0) : (IsSwap ? 0 : 1);
            uint8_t BaseCh = MotorIdx * 2;
            uint32_t Fwd = (Ptr->Speed[i] > 0) ? (uint32_t)Ptr->Speed[i] : 0;
            uint32_t Rev = (Ptr->Speed[i] < 0) ? (uint32_t)(-Ptr->Speed[i]) : 0;

            __IHF__SetPWM(BaseCh, Fwd, NoPWM, MaxDuty);
            __IHF__SetPWM(BaseCh + 1, Rev, NoPWM, MaxDuty);
        }
        SafeFlagClear(&Ptr->Flag, eHBridge_ModifiedSpeed);
    }
    return STAT_OKE;
}