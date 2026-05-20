#include "Module.h"

#include "../__CommonHeaders.h"
#include "../../AppComm/SharedAPIs.h"

/**
 * @brief Mock function to retrieve H-Bridge motor speed.
 * @note Replace this with the actual function from the HBridge module.
 * * @return uint16_t The current motor speed in RPM.
 */
__attribute__((weak)) uint16_t HBridge_GetSpeed(void) { 
    /* Return mock data to avoid compilation errors */
    return 3000;      
}

/**
 * @brief High-level control task to collect and transmit ECU state periodically.
 * * @param arg Pointer to task arguments (unused).
 */
void HeartBeatRuntime(void* arg) {
    SysEntry("HeartBeatRuntime");
    SysLog("HeartBeatRuntime(...): Started! Target CCU: 10.0.0.100:%d", CCU_UDP_PORT);

    SF_ECUState_t ecuState;
    memset(&ecuState, 0, sizeof(SF_ECUState_t));
    static uint8_t sync_num = 0;

    /* Infinite loop to periodically process and dispatch heartbeat frames */
    while (1) {
        /* 1. Retrieve actual data from sensors and actuators */
        uint16_t dist0_mm = HCSR04_GetLatestDistanceMm(0);
        uint16_t dist1_mm = HCSR04_GetLatestDistanceMm(1);
        uint16_t dist2_mm = HCSR04_GetLatestDistanceMm(2);
        
        /* Convert millimeters to centimeters and mask to fit into the 10-bit fields (Max: 1023) */
        uint16_t dist0_cm = (dist0_mm != 0xFFFFU) ? (dist0_mm / 10U) : 0x3FF;
        uint16_t dist1_cm = (dist1_mm != 0xFFFFU) ? (dist1_mm / 10U) : 0x3FF;
        uint16_t dist2_cm = (dist2_mm != 0xFFFFU) ? (dist2_mm / 10U) : 0x3FF;

        /* --- FAIL-SAFE CHECK --- */
        /* Check if any sensor distance is below the emergency threshold */
        if (dist0_cm < SF_ETT_Distance[0]) {
            SysErr("HeartBeatRuntime: EMERGENCY! Sensor 0 distance (%u cm) is below threshold (%u cm).", dist0_cm, SF_ETT_Distance[0]);
            EmergencyStop();
        }
        if (dist1_cm < SF_ETT_Distance[1]) {
            SysErr("HeartBeatRuntime: EMERGENCY! Sensor 1 distance (%u cm) is below threshold (%u cm).", dist1_cm, SF_ETT_Distance[1]);
            EmergencyStop();
        }
        if (dist2_cm < SF_ETT_Distance[2]) {
            SysErr("HeartBeatRuntime: EMERGENCY! Sensor 2 distance (%u cm) is below threshold (%u cm).", dist2_cm, SF_ETT_Distance[2]);
            EmergencyStop();
        }

        /* 2. Pack the retrieved information into the ECU state structure */
        ecuState.Distance.Fields.D0 = dist0_cm & 0x3FF;
        ecuState.Distance.Fields.D1 = dist1_cm & 0x3FF;
        ecuState.Distance.Fields.D2 = dist2_cm & 0x3FF;
        ecuState.Distance.Fields.SyncNum = sync_num & 0x03;

        /* Get actual motor speeds (absolute value to fit 10-bit unsigned field) */
        ecuState.Motor.Fields.M0 = (abs(MotorGetSpeed0())) & 0x3FF;
        ecuState.Motor.Fields.M1 = (abs(MotorGetSpeed1())) & 0x3FF;
        ecuState.Motor.Fields.SyncNum = sync_num & 0x03;
        ecuState.Motor.Fields.Reserved = 0;
        
        sync_num = (sync_num + 1) % 4;

        /* 3. Dispatch the packed UDP payload via the W5500 network layer */
        ReturnCode_t ret = Eth_SendUDPPacket(CCU_IP_ADDR, CCU_UDP_PORT, (uint8_t*)&ecuState, sizeof(SF_ECUState_t));
        
        /* Steering logic to evaluate the transmission status and log the outcome */
        if (ret == STAT_OKE) {
            SysLog("HeartBeatRuntime: Sent ECU State -> D0: %u, D1: %u, D2: %u, M0: %u, M1: %u, SyncNum: %u", 
                   ecuState.Distance.Fields.D0,
                   ecuState.Distance.Fields.D1,
                   ecuState.Distance.Fields.D2,
                   ecuState.Motor.Fields.M0,
                   ecuState.Motor.Fields.M1,
                   ecuState.Distance.Fields.SyncNum);
        } else {
            SysErr("HeartBeatRuntime: Failed to send ECU State! Error: %d", ret);
        }

        /* 4. Suspend task execution for the defined 300ms cycle duration */
        vTaskDelay(pdMS_TO_TICKS(HEART_TASK_CYCLE_MS));
    }

    SysExit("HeartBeatRuntime");
    
    /* Control flow termination: Delete task to clean up resources if loop exits */
    vTaskDelete(NULL);
}
