#include "Module.h"

#include "../__CommonHeaders.h"
#include "../../AppComm/SharedAPIs.h"

/*
 * @brief Mock function to retrieve H-Bridge motor speed.
 * @note Replace this with the actual function from the HBridge module.
 * @return uint16_t The current motor speed in RPM.
 */
__attribute__((weak)) uint16_t HBridge_GetSpeed(void) { 
    /* Return mock data to avoid compilation errors */
    return 3000;      
}

/*
 * @brief Application-level Ethernet Rx Callback.
 * @param pkt Pointer to the received packet slot.
 */
void App_EthernetRxCallback(PacketSlot_t* pkt) {
    /* 1. Ignore if packet pointer is invalid */
    if (pkt == NULL) {
        return;
    }
    
    /* 2. Log Network Info using public W5500 module APIs */
    SysLog("App_EthernetRxCallback(...): Received packet");
    W5500_LogUDPInfo(pkt);

    /* 3. Log raw frame data (Hex and ASCII dump) */
    W5500_LogFrame((GenericPtr_t)pkt->Data, pkt->Size, GenericNullPtr, GenericNullPtr);

    /* 4. Parse the received packet to extract CCU telemetry commands */
    ParseCCUPacket((const uint8_t*)pkt->Data, pkt->Size);
    
    /* 5. Optional: Echo the packet back (remove if not needed) */
    TxPacket_Push(pkt->Size, (GenericPtr_t)pkt->Data, pkt->SrcIP.Byte, pkt->SrcPort.Word, pkt->SrcMAC.Byte);
    
    /* Notify communication task to send out the Echo packet */
    if (W5500_TaskComm_TaskHandler != NULL) {
        xTaskNotifyGive(W5500_TaskComm_TaskHandler);
    }
}

void Eth_CallBackSetUp(){
    /* Register the application-level Rx callback to the Ethernet module */
    Eth_SetRxCallback(App_EthernetRxCallback);
    SysLog("Eth_CallBackSetUp(...): Application Ethernet Rx Callback registered successfully.");
}

/*
 * @brief High-level control task to collect and transmit ECU state periodically.
 * @param arg Pointer to task arguments (unused).
 */
void HeartBeatRuntime(void* arg) {
    SysEntry("HeartBeatRuntime");
    SysLog("HeartBeatRuntime(...): Started! Target CCU: 10.0.0.100:%d", CCU_UDP_PORT);

    /* Register the Ethernet data reception callback before entering the loop */
    Eth_CallBackSetUp();

    SF_ECUState_t ecuState;
    memset(&ecuState, 0, sizeof(SF_ECUState_t));
    memcpy(&ecuState.ECUInfo, &ECUInfo, sizeof(ECUInfo));

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
        
        /* Check if sensor 1 is below threshold */
        if (dist1_cm < SF_ETT_Distance[1]) {
            SysErr("HeartBeatRuntime: EMERGENCY! Sensor 1 distance (%u cm) is below threshold (%u cm).", dist1_cm, SF_ETT_Distance[1]);
            EmergencyStop();
        }
        
        /* Check if sensor 2 is below threshold */
        if (dist2_cm < SF_ETT_Distance[2]) {
            SysErr("HeartBeatRuntime: EMERGENCY! Sensor 2 distance (%u cm) is below threshold (%u cm).", dist2_cm, SF_ETT_Distance[2]);
            EmergencyStop();
        }

        /* 2. Pack the retrieved information into the ECU state structure */
        ecuState.Distance.Fields.D0 = dist0_cm & 0x3FF;
        ecuState.Distance.Fields.D1 = dist1_cm & 0x3FF;
        ecuState.Distance.Fields.D2 = dist2_cm & 0x3FF;
        ecuState.Distance.Fields.SyncNum = sync_num & 0x03;

        // /* Get actual motor speeds (absolute value to fit 10-bit unsigned field) */
        // ecuState.Motor.Fields.M0 = (abs(MotorGetSpeed0())) & 0x3FF;
        // ecuState.Motor.Fields.M1 = (abs(MotorGetSpeed1())) & 0x3FF;
        // ecuState.Motor.Fields.SyncNum = sync_num & 0x03;
        // ecuState.Motor.Fields.Reserved = 0;
        
        /* Scale actual motor speeds from 0~1024 to -100~100 */
        int32_t m0 = ((MotorGetSpeed0() * 200) / 1024);
        int32_t m1 = ((MotorGetSpeed1() * 200) / 1024);

        /* Clamp to valid range */
        if (m0 > 100)  m0 = 100;
        if (m0 < -100) m0 = -100;

        if (m1 > 100)  m1 = 100;
        if (m1 < -100) m1 = -100;

        ecuState.Motor.Fields.M0 = (m0 + 100) & 0xFF;
        ecuState.Motor.Fields.M1 = (m1 + 100) & 0xFF;
        ecuState.Motor.Fields.SyncNum = sync_num & 0x03;
        ecuState.Motor.Fields.Reserved = 0;

        sync_num = (sync_num + 1) % 4;

        /* 3. Dispatch the packed UDP payload via the W5500 network layer */
        ReturnCode_t ret = Eth_SendUDPPacket(CCU_IP_ADDR, CCU_UDP_PORT, (uint8_t*)&ecuState, sizeof(SF_ECUState_t));
        
        /* Evaluate the transmission status and log the outcome */
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

        /* 4. Suspend task execution for the defined cycle duration */
        vTaskDelay(pdMS_TO_TICKS(HEART_TASK_CYCLE_MS));
    }

    SysExit("HeartBeatRuntime");
    
    /* Delete task to clean up resources if loop exits */
    vTaskDelete(NULL);
}