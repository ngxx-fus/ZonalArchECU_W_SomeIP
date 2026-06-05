#include "TempControlFrame.h"
#include "../../AppComm/HBridge/Module.h"

static uint16_t ScaleSpeed(int32_t input)
{
    // Clamp để tránh giá trị vượt giới hạn
    if (input < -100)
        input = -100;
    if (input > 100)
        input = 100;

    return (uint16_t)(((input + 100) * 1024) / 200);
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
            }
            /* Terminate switch case */
            break;

        case CMD_ENG_CTRL:
            /* Validate if the left engine is moving forward */
            if (packet->info.eng_ctrl.left_dir == ARG_ENG_FWD) {
                SysLog("ParseCCUPacket(...): Left Engine Moving Forward at speed offset: %u\n", 
                       packet->info.eng_ctrl.left_speed - 0xFE00A100);
                int32_t RawValue = packet->info.eng_ctrl.left_speed - 0xFE00A100;
                uint16_t ScaledSpeed = ScaleSpeed(RawValue);
                MotorSetSpeed0((int32_t)ScaledSpeed);
                MotorSetSpeed1((int32_t)ScaledSpeed);
                HBridge_Apply(Motor);
            }
            /* Terminate switch case */
            break;

        default:
            SysLog("ParseCCUPacket(...): [WARNING] Unknown Command Type: 0x%08X\n", cmd);
            /* Terminate switch case */
            break;
    }

    /* Exit the parsing function */
    return;
}