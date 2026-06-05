#include "TempControlFrame.h"
#include "../../AppComm/HBridge/Module.h"

#define SCALE_IN_POSITIVE_ABS_MIN_VAL    (300)
#define SCALE_IN_POSITIVE_ABS_MAX_VAL    (1024)
#define SCALE_IN_NEGATIVE_ABS_MIN_VAL    (300)
#define SCALE_IN_NEGATIVE_ABS_MAX_VAL    (1024)

#define SCALE_OUT_POSITIVE_ABS_MIN_VAL   (0)
#define SCALE_OUT_POSITIVE_ABS_MAX_VAL   (100)
#define SCALE_OUT_NEGATIVE_ABS_MIN_VAL   (0)
#define SCALE_OUT_NEGATIVE_ABS_MAX_VAL   (100)

/*
 * @brief Converts raw PWM/ADC input value to scaled percentage speed.
 * @param rawValue The raw input value (e.g., -1024 to 1024).
 * @return Scaled speed percentage (-100 to 100).
 */
int32_t ConvertRawToPercent(int32_t rawValue)
{
    int32_t speedPercent = 0;

    /* Route execution based on positive input range */
    if (rawValue >= SCALE_IN_POSITIVE_ABS_MIN_VAL)
    {
        /* Restrict input to defined positive maximum */
        if (rawValue > SCALE_IN_POSITIVE_ABS_MAX_VAL)
        {
            rawValue = SCALE_IN_POSITIVE_ABS_MAX_VAL;
        }
        
        speedPercent = SCALE_OUT_POSITIVE_ABS_MIN_VAL + ((rawValue - SCALE_IN_POSITIVE_ABS_MIN_VAL) * (SCALE_OUT_POSITIVE_ABS_MAX_VAL - SCALE_OUT_POSITIVE_ABS_MIN_VAL)) / (SCALE_IN_POSITIVE_ABS_MAX_VAL - SCALE_IN_POSITIVE_ABS_MIN_VAL);
    }
    /* Route execution based on negative input range */
    else if (rawValue <= -SCALE_IN_NEGATIVE_ABS_MIN_VAL)
    {
        /* Restrict input to defined negative maximum */
        if (rawValue < -SCALE_IN_NEGATIVE_ABS_MAX_VAL)
        {
            rawValue = -SCALE_IN_NEGATIVE_ABS_MAX_VAL;
        }
        
        speedPercent = -SCALE_OUT_NEGATIVE_ABS_MIN_VAL - (((-rawValue - SCALE_IN_NEGATIVE_ABS_MIN_VAL) * (SCALE_OUT_NEGATIVE_ABS_MAX_VAL - SCALE_OUT_NEGATIVE_ABS_MIN_VAL)) / (SCALE_IN_NEGATIVE_ABS_MAX_VAL - SCALE_IN_NEGATIVE_ABS_MIN_VAL));
    }

    /* Terminate function and yield scaled percentage */
    return speedPercent;
}

/*
 * @brief Converts percentage speed back to raw PWM/ADC value.
 * @param speedPercent The percentage speed output (-100 to 100).
 * @return Scaled raw value with deadband applied.
 */
int32_t ConvertPercentToRaw(int32_t speedPercent)
{
    int32_t rawValue = 0;

    /* Route execution based on positive speed range */
    if (speedPercent > SCALE_OUT_POSITIVE_ABS_MIN_VAL)
    {
        /* Restrict speed to defined positive maximum */
        if (speedPercent > SCALE_OUT_POSITIVE_ABS_MAX_VAL)
        {
            speedPercent = SCALE_OUT_POSITIVE_ABS_MAX_VAL;
        }

        rawValue = SCALE_IN_POSITIVE_ABS_MIN_VAL + ((speedPercent - SCALE_OUT_POSITIVE_ABS_MIN_VAL) * (SCALE_IN_POSITIVE_ABS_MAX_VAL - SCALE_IN_POSITIVE_ABS_MIN_VAL)) / (SCALE_OUT_POSITIVE_ABS_MAX_VAL - SCALE_OUT_POSITIVE_ABS_MIN_VAL);
    }
    /* Route execution based on negative speed range */
    else if (speedPercent < -SCALE_OUT_NEGATIVE_ABS_MIN_VAL)
    {
        /* Restrict speed to defined negative maximum */
        if (speedPercent < -SCALE_OUT_NEGATIVE_ABS_MAX_VAL)
        {
            speedPercent = -SCALE_OUT_NEGATIVE_ABS_MAX_VAL;
        }

        rawValue = -SCALE_IN_NEGATIVE_ABS_MIN_VAL - (((-speedPercent - SCALE_OUT_NEGATIVE_ABS_MIN_VAL) * (SCALE_IN_NEGATIVE_ABS_MAX_VAL - SCALE_IN_NEGATIVE_ABS_MIN_VAL)) / (SCALE_OUT_NEGATIVE_ABS_MAX_VAL - SCALE_OUT_NEGATIVE_ABS_MIN_VAL));
    }

    /* Terminate function and yield mapped raw value */
    return rawValue;
}

/*
 * @brief   Parses the incoming UDP packet and extracts telemetry commands.
 * @param   raw_data Pointer to the received byte array.
 * @param   len Total length of the received byte array.
 * @return  None.
 */
void ParseCCUPacket(const uint8_t *raw_data, size_t len) {
    /* Verify if the packet meets the minimum 64-byte size requirement */
    if (len < sizeof(ccu_packet_t)) {
        /* Abort parsing for malformed or incomplete frames */
        return;
    }

    const ccu_packet_t *packet = (const ccu_packet_t *)raw_data;
    uint32_t cmd = packet->command;

    /* Steer execution based on the extracted command identifier */
    switch (cmd) {
        case CMD_SYS_CTRL:
            /* Execute branch if system control command is detected */
            if (packet->info.sys_ctrl.fl_state == ARG_SYS_START) {
                SysLog("ParseCCUPacket(...): [ACTION] System Start Command Detected.\n");
            } 
            /* Execute branch if alternate condition is met */
            else if (packet->info.sys_ctrl.fl_state == ARG_SYS_STOP) {
                SysLog("ParseCCUPacket(...): System Stop Command Detected.\n");
                MotorSetSpeed0(0);
                MotorSetSpeed1(0);
                HBridge_Apply(Motor);
            }
            /* Terminate switch case */
            break;

        case CMD_ENG_CTRL:
        {
            int32_t left_percent = 0;
            if (packet->info.eng_ctrl.left_dir == ARG_ENG_FWD) {
                left_percent = (int32_t)(packet->info.eng_ctrl.left_speed - 0xFE00A100);
            } else if (packet->info.eng_ctrl.left_dir == ARG_ENG_BWD) {
                left_percent = -(int32_t)(packet->info.eng_ctrl.left_speed - 0xFE00A100);
            }

            int32_t right_percent = 0;
            if (packet->info.eng_ctrl.right_dir == ARG_ENG_FWD) {
                right_percent = (int32_t)(packet->info.eng_ctrl.right_speed - 0xFE00A100);
            } else if (packet->info.eng_ctrl.right_dir == ARG_ENG_BWD) {
                right_percent = -(int32_t)(packet->info.eng_ctrl.right_speed - 0xFE00A100);
            }

            SysLog("ParseCCUPacket(...): CMD_ENG_CTRL -> Left: %d%%, Right: %d%%\n", (int)left_percent, (int)right_percent);

            MotorSetSpeed0(ConvertPercentToRaw(left_percent));
            MotorSetSpeed1(ConvertPercentToRaw(right_percent));
            HBridge_Apply(Motor);
            /* Terminate switch case */
            break;
        }

        default:
            SysLog("ParseCCUPacket(...): [WARNING] Unknown Command Type: 0x%08X\n", cmd);
            /* Terminate switch case */
            break;
    }

    /* Exit the parsing function */
    return;
}