#include "Module.h"

#include "../__CommonHeaders.h"
#include "../../AppComm/SharedAPIs.h"
#include "../TestModule/TestModule.h"

#include "nvs_flash.h"
#include "nvs.h"
#include "driver/gpio.h"

#ifndef CCU_CONNECT_AUTH_TIMEOUT_MS
    #define CCU_CONNECT_AUTH_TIMEOUT_MS (10000U)
#endif

/* Macro to disable Keep-Alive tracking for testing purposes */
#define DISABLE_KEEPALIVE_TRACK

/* Control flow: Define Heartbeat State Machine Steps */
enum App_HeartBeat_State_e {
    eSTATE_BROADCAST        = 0x00, ///< Node is broadcasting its presence
    eSTATE_AUTHENTICATION   = 0x01, ///< Node is performing Challenge-Response Auth
    eSTATE_CONNECTED        = 0x02, ///< Node is fully authenticated and streaming data
    eSTATE_EMERGENCY_STOP   = 0x03  ///< Node is in fault/emergency halt state
};

/* * Typedef to force the state machine variable to 1-byte (uint8_t).
* Prevents compiler from implicitly using 4-byte integers for the enum.
*/
typedef uint8_t App_HeartBeat_State_t;

/* Configuration Constants */
#define CCU_KEEPALIVE_TIMEOUT_MS  (1000U)
#define BOOT_BUTTON_PIN           (0)

/* Internal variable tracking the current connection state */
volatile static App_HeartBeat_State_t App_HeartBeat_State = (App_HeartBeat_State_t)eSTATE_BROADCAST;
volatile static IPv4EndPoint_t        CCU_EP;

/* ZECU_Frame_Header*/
ZECUFrame_Header_t ZECU_Frame_Header = {
    .MagicByte0 = ZECU_FRAME_HEADER_MAGIC_BYTE,
    .Version    = ZECU_FRAME_VERSION,
    .MagicByte1 = ZECU_FRAME_HEADER_MAGIC_BYTE,
    .MagicByte2 = ZECU_FRAME_HEADER_MAGIC_BYTE,
    .Info       = {
        .Label          = ECU_NAME_STR,
        .MAC            = SRC_MAC_ADDR_INITIALIZER,
        .MagicByte3     = ZECU_FRAME_HEADER_MAGIC_BYTE,
        .IPv4Address    = SRC_IP_ADDR_INITIALIZER,
        .IPv4Port       = DEFAULT_PORT_HEX,
        .MagicByte4     = ZECU_FRAME_HEADER_MAGIC_BYTE,
        .MagicByte5     = ZECU_FRAME_HEADER_MAGIC_BYTE,
        .MagicByte6     = ZECU_FRAME_HEADER_MAGIC_BYTE
    }
};

ZECUFrame_Body_Broadcast_t ZECU_Frame_Body_Broadcast = {
    .MagicDWord0    = ZECU_FRAME_HEADER_MAGIC_DWORD,
    .ServiceCount   = ZECU_FRAME_SERVICE_NUM
};

/* Internal state variable to keep track of the Challenge generated for Auth */
static uint64_t last_challenge = 0;

/* Timestamp of the last valid Keep-Alive received from CCU */
static volatile TickType_t last_keepalive_time = 0;

/* --- ARITHMETIC  UTILITIES --- */

int16_t ConvertRaw2Percent(int16_t raw_val) {
    /* Control flow: route to the first segment if the raw value is below or equal to the center breakpoint */
    if (raw_val <= 300 && raw_val >= (-300)){
        return 0;
    }
    if (raw_val <= 300) {
        /* Control flow: return scaled value mapped from negative raw range to negative percentage */
        return LinearScale(-1024, -300, -100, 0, 0, 0, raw_val);
    } 
    else {
        /* Control flow: return scaled value mapped from positive raw range to positive percentage */
        return LinearScale(300, 1024, 0, 100, 0, 0, raw_val);
    }
}

int16_t ConvertPercent2Raw(int16_t percent_val) {
    if(percent_val == 0) return 0;
    /* Control flow: route to the first segment if the percentage is below or equal to zero */
    if (percent_val <= 0) {
        /* Control flow: return scaled value mapped from negative percentage to negative raw range */
        return LinearScale(-100, 0, -1024, -300, 0, 0, percent_val);
    } 
    else {
        /* Control flow: return scaled value mapped from positive percentage to positive raw range */
        return LinearScale(0, 100, 300, 1024, 0, 0, percent_val);
    }
}

/* --- NVS STORAGE UTILITIES --- */

static void NVS_SaveCCUEndpoint(void) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err == ESP_OK) {
        nvs_set_blob(my_handle, "ccu_ep", (void*)&CCU_EP, sizeof(IPv4EndPoint_t));
        nvs_commit(my_handle);
        nvs_close(my_handle);
        SysLog("NVS: CCU Endpoint saved successfully.");
    }
}

static bool NVS_LoadCCUEndpoint(void) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &my_handle);
    if (err == ESP_OK) {
        size_t required_size = sizeof(IPv4EndPoint_t);
        err = nvs_get_blob(my_handle, "ccu_ep", (void*)&CCU_EP, &required_size);
        nvs_close(my_handle);
        if (err == ESP_OK && required_size == sizeof(IPv4EndPoint_t)) {
            SysLog("NVS: Loaded CCU Endpoint -> IP: %u.%u.%u.%u Port: %u", 
                CCU_EP.Member.Addr.Byte[3], CCU_EP.Member.Addr.Byte[2], 
                CCU_EP.Member.Addr.Byte[1], CCU_EP.Member.Addr.Byte[0], 
                CCU_EP.Member.Port.Word);
            return true;
        }
    }
    return false;
}

static void NVS_EraseCCUEndpoint(void) {
    nvs_handle_t my_handle;
    if (nvs_open("storage", NVS_READWRITE, &my_handle) == ESP_OK) {
        nvs_erase_key(my_handle, "ccu_ep");
        nvs_commit(my_handle);
        nvs_close(my_handle);
        SysLog("NVS: CCU Endpoint erased.");
    }
}

void ParsePacket(PacketSlot_t* pkt){
    if(IsNull(pkt)) return;
    if(pkt->Size < sizeof(ZECUFrame_Generic_t)) {
        SysLog("ParsePacket(...): Frame is not recognized!");
        return;
    }
    ZECUFrame_Generic_t * Fr = (ZECUFrame_Generic_t*)pkt->Data;

    /* 1. Ensure the Frame has valid Checksum/CRC before acting on commands */
    if(App_VerifyPacket(Fr) != STAT_OKE) {
        SysErr("ParsePacket(...): Received frame failed CRC/Checksum validation!");
        return;
    }
    
    SysLog("ParsePacket(...): Name: %s", Fr->Header.Info.Label);
    SysLog("ParsePacket(...): IP: %u.%u.%u.%u", 
           Fr->Header.Info.IPv4Address[0], Fr->Header.Info.IPv4Address[1], 
           Fr->Header.Info.IPv4Address[2], Fr->Header.Info.IPv4Address[3]);
    SysLog("ParsePacket(...): Port: %d", Fr->Header.Info.IPv4Port);
    SysLog("ParsePacket(...): FrameType: %X", Fr->Header.FrameType);
    
    /* 2. Process Pairing Requests (Monitor sent ePairingRequest_New) */
    if (Fr->Header.FrameType == eFrameType_PairingRequest) {
        if (Fr->Body.PairingRequest.Request == ePairingRequest_New) {
            SysLog("ParsePacket(...): AUTH: Received PairingRequest from CCU.");
            
            /* Save CCU IPv4(Address/Port) to dynamic Endpoint */
            CCU_EP.Member.Addr.Dword = IPv4ToUint32(
                Fr->Header.Info.IPv4Address[0],
                Fr->Header.Info.IPv4Address[1],
                Fr->Header.Info.IPv4Address[2],
                Fr->Header.Info.IPv4Address[3]
            );
            CCU_EP.Member.Port.Word = Fr->Header.Info.IPv4Port;

            SysLog("ParsePacket(...): Saved: CCU's IPv4_Addr=%d:%d:%d:%d | IPv4_Port=%d",
                Fr->Header.Info.IPv4Address[0],
                Fr->Header.Info.IPv4Address[1],
                Fr->Header.Info.IPv4Address[2],
                Fr->Header.Info.IPv4Address[3],
                CCU_EP.Member.Port.Word
            );

            /* Change State Machine */
            App_HeartBeat_State = eSTATE_AUTHENTICATION;
        }
    }
    /* 3. Process Keep-Alive frames to feed the watchdog */
    else if (eFrameType_KeepAlive == Fr->Header.FrameType) {
        #ifndef DISABLE_KEEPALIVE_TRACK
            /* Feed the CCU Keep-Alive Watchdog */
            last_keepalive_time = xTaskGetTickCount();
        #endif
        
        extern void Test_WakeUp(void);
        Test_WakeUp();
        
        return; /* Skip further logging for Keep-Alive to prevent UART spam */
    }
    /* 3. Process Authentication Response (Monitor signed the challenge) */
    else if (Fr->Header.FrameType == eFrameType_AuthRX) {
        SysLog("AUTH: Received AuthRX (Signature) from CCU.");
        Auth_SetCoreKey(0xDEADBEEF);
        
        if (App_VerifyFrame_AuthRX(Fr, last_challenge) == STAT_OKE) {
            SysLog("AUTH: Authentication SUCCESS! Handshake Complete.");
            
            /* Save the validated Endpoint to NVS for future reboots */
            NVS_SaveCCUEndpoint();
            
            /* Transition into CONNECTED state, freeing the Heartbeat broadcast loop */
            App_HeartBeat_State = eSTATE_CONNECTED;
        } else {
            SysErr("AUTH: Authentication FAILED! Signature mismatch.");
            
            /* Revert back to broadcasting */
            App_HeartBeat_State = eSTATE_BROADCAST;
        }
    }
    else if(Fr->Header.FrameType == eFrameType_EngineControl1){
        SysLog("ParsePacket(...): Speed0=%d", Fr->Body.EngineControl1.M0);
        MotorSetSpeed0(ConvertPercent2Raw(Fr->Body.EngineControl1.M0));
        Eth_RequestResponseNow();
    }
    else if(Fr->Header.FrameType == eFrameType_EngineControl2){
        SysLog("ParsePacket(...): Speed1=%d", Fr->Body.EngineControl2.M1);
        MotorSetSpeed1(ConvertPercent2Raw(Fr->Body.EngineControl2.M1));
        Eth_RequestResponseNow();
    }
    else if(Fr->Header.FrameType == eFrameType_EngineControl3){
        SysLog("ParsePacket(...): Speed0=%d, Speed1=%d", Fr->Body.EngineControl3.M0, Fr->Body.EngineControl3.M1);
        MotorSetSpeed(
            ConvertPercent2Raw(Fr->Body.EngineControl3.M0),
            ConvertPercent2Raw(Fr->Body.EngineControl3.M1)
        );
        Eth_RequestResponseNow();
    }
    else if(Fr->Header.FrameType == eFrameType_EmergencyStop){
        EmergencyStop();
    }
}

void App_EthernetRxCallback(PacketSlot_t* pkt) {
    /* 1. Ignore if packet pointer is invalid */
    if (pkt == NULL) {
        return;
    }
    
    /* 2. Log Network Info using public W5500 module APIs */
    SysLog("App_EthernetRxCallback(...): Received packet");
    Eth_LogUDPInfo(pkt);

    /* 3. Log raw frame data (Hex and ASCII dump) */
    Eth_LogFrame((GenericPtr_t)pkt->Data, pkt->Size, GenericNullPtr, GenericNullPtr);

    /* 4. Parse the received packet using the updated ZECU Frame protocol */
    ParsePacket(pkt);
}

void App_SendImmediateHeartbeat(void) {
    ZECUFrame_Generic_t tx_frame;
    memset(&tx_frame, 0, sizeof(ZECUFrame_Generic_t));
    memcpy(&tx_frame.Header, &ZECU_Frame_Header, sizeof(ZECUFrame_Header_t));

    static uint8_t sync_num_immediate = 0;

    /* Use latest cached ultrasonic readings (do not trigger new measurements) */
    uint16_t dist0_mm = HCSR04_GetLatestDistanceMm(0);
    uint16_t dist1_mm = HCSR04_GetLatestDistanceMm(1);
    uint16_t dist2_mm = HCSR04_GetLatestDistanceMm(2);

    uint16_t dist0_cm = (dist0_mm != 0xFFFFU) ? (dist0_mm / 10U) : 0x3FF;
    uint16_t dist1_cm = (dist1_mm != 0xFFFFU) ? (dist1_mm / 10U) : 0x3FF;
    uint16_t dist2_cm = (dist2_mm != 0xFFFFU) ? (dist2_mm / 10U) : 0x3FF;

    /* Read latest motor speeds and convert */
    int32_t m0 = ConvertRaw2Percent(MotorGetSpeed0());
    int32_t m1 = ConvertRaw2Percent(MotorGetSpeed1());

    App_WriteFrame_HeartBeat(&tx_frame, sync_num_immediate, dist0_cm, dist1_cm, dist2_cm, (int8_t)m0, (int8_t)m1, 0, 0);

    sync_num_immediate = (sync_num_immediate + 1) % 4;

    ReturnCode_t ret = Eth_SendUDPPacket(CCU_EP.Member.Addr.Dword, CCU_EP.Member.Port.Word, (uint8_t*)&tx_frame, sizeof(ZECUFrame_Generic_t));
    if (ret == STAT_OKE) {
        SysLog("App_SendImmediateHeartbeat: Sent ECU State -> D0: %u, D1: %u, D2: %u, M0: %u, M1: %u, SyncNum: %u",
            dist0_cm, dist1_cm, dist2_cm, m0, m1, (sync_num_immediate - 1) & 0x03);
    } else {
        SysErr("App_SendImmediateHeartbeat: Failed to send ECU State! Error: %d", ret);
    }
}

void App_EthernetResponse(PacketSlot_t* pkt){
    (void) pkt;
    /* Send immediate heartbeat using cached sensor values and current motor speeds */
    App_SendImmediateHeartbeat();
}

void Eth_CallBackSetUp(){
    /* Register the application-level Rx callback to the Ethernet module */
    Eth_SetRxCallback(App_EthernetRxCallback);
    /* Route execution to register the Ethernet response callback */
    Eth_SetResponseFunction(App_EthernetResponse);
    SysLog("Eth_CallBackSetUp(...): Application Ethernet Rx Callback registered successfully.");
}

/**
 * @brief Connects the ZECU to the CCU by handling the full handshake (Broadcast -> Auth -> Connected).
 * @return ReturnCode_t STAT_OKE upon successful execution.
 */
ReturnCode_t App_ConnectCCU(void) {
    ZECUFrame_Generic_t tx_frame;
    uint16_t total_frame_size = sizeof(ZECUFrame_Generic_t); /* Exactly 612 bytes */
    TickType_t auth_start_time;

    /* Ensure state is reset to broadcast if not already authenticating */
    if (App_HeartBeat_State != eSTATE_AUTHENTICATION) {
        App_HeartBeat_State = eSTATE_BROADCAST;
    }

    /* State machine entry point */
    __STATE_BROADCAST__:
    while (eSTATE_BROADCAST == App_HeartBeat_State) {
        /* Assemble the generic frame from static components */
        memset(&tx_frame, 0, sizeof(ZECUFrame_Generic_t));
        memcpy(&tx_frame.Header, &ZECU_Frame_Header, sizeof(ZECUFrame_Header_t));
        tx_frame.Header.FrameType = eFrameType_Broadcast;
        memcpy(&tx_frame.Body.Broadcast, &ZECU_Frame_Body_Broadcast, sizeof(ZECUFrame_Body_Broadcast_t));

        /* Update the Trailer (Calculates Checksum and CRC over the full 604 byte payload) */
        App_UpdateTrailer(&tx_frame);

        /* Broadcast the frame via UDP (Broadcast IP = 255.255.255.255 = 0xFFFFFFFF) */
        Eth_SendUDPPacket(0xFFFFFFFF, CCU_UDP_PORT, (uint8_t*)&tx_frame, total_frame_size);
        
        SysLog("App_ConnectCCU(...): Broadcasted ZECU Services (%u bytes)", total_frame_size);

        /* Delay before the next broadcast transmission */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    // __STATE_AUTHENTICATION__:
    if (eSTATE_AUTHENTICATION == App_HeartBeat_State) {
        SysLog("App_ConnectCCU(...): eSTATE_AUTHENTICATION");
        auth_start_time = xTaskGetTickCount();
        
        memset(&tx_frame, 0, sizeof(ZECUFrame_Generic_t));
        memcpy(&tx_frame.Header, &ZECU_Frame_Header, sizeof(ZECUFrame_Header_t));
        
        Auth_SetCoreKey(0xDEADBEEF); /* Use fixed Master Key */
        
        /* Generate a single Challenge and pack it into the AuthTX frame */
        App_WriteFrame_AuthTX(&tx_frame, 1, xTaskGetTickCount());
        last_challenge = tx_frame.Body.AuthTX.Challenge;

        while (eSTATE_AUTHENTICATION == App_HeartBeat_State) {
            SysLog("App_ConnectCCU(...): eSTATE_AUTHENTICATION");
            TickType_t elapsed_ticks = xTaskGetTickCount() - auth_start_time;
            uint32_t elapsed_ms = elapsed_ticks * portTICK_PERIOD_MS;

            /* Calculate remaining time in milliseconds (clamp to 0 if timed out) */
            uint32_t remaining_ms = (elapsed_ms < CCU_CONNECT_AUTH_TIMEOUT_MS) ? (CCU_CONNECT_AUTH_TIMEOUT_MS - elapsed_ms) : 0;

            /* Optional: Log the exact time left before hitting the timeout */
            /* SysDebug("App_ConnectCCU: Time left for AuthRX: %lu ms", remaining_ms); */

            /* Check if the wait time has exceeded the CCU_CONNECT_AUTH_TIMEOUT_MSms timeout threshold */
            if (elapsed_ticks > pdMS_TO_TICKS(CCU_CONNECT_AUTH_TIMEOUT_MS)) {
                SysErr("App_ConnectCCU: Timeout waiting for AuthRX! Elapsed: %lu ms. Reverting to BROADCAST.", elapsed_ms);
                App_HeartBeat_State = eSTATE_BROADCAST;

                /* Revert state machine to BROADCAST mode due to authentication timeout */
                goto __STATE_BROADCAST__;
            }

            /* Dispatch the AuthTX Challenge to CCU using the saved dynamic endpoint */
            Eth_SendUDPPacket(CCU_EP.Member.Addr.Dword, CCU_EP.Member.Port.Word, (uint8_t*)&tx_frame, total_frame_size);
            SysLog("AUTH: Sent AuthTX (Challenge) to CCU.");

            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
    
    if (eSTATE_CONNECTED == App_HeartBeat_State) {
        /* Save CCU IPv4(Address/Port)*/
        /* Update CCU_EP */
        /* (Already captured during PairingRequest to enable AuthTX routing) */
        return STAT_OKE;
    } else {
        goto __STATE_BROADCAST__; /* Fallback for unexpected state transitions */
    }
}

/*
* @brief High-level control task to collect and transmit ECU state periodically.
* @param arg Pointer to task arguments (unused).
*/
void HeartBeatRuntime(void* arg) {
    SysEntry("HeartBeatRuntime");
    SysLog("HeartBeatRuntime(...): Started! Waiting for CCU Pairing...");

    MotorSetSpeed0(0);
    MotorSetSpeed1(0);

    /* Initialize NVS Flash for persistent storage */
    esp_err_t ret_nvs = nvs_flash_init();
    if (ret_nvs == ESP_ERR_NVS_NO_FREE_PAGES || ret_nvs == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret_nvs = nvs_flash_init();
    }
    
    /* Configure BOOT Button (GPIO0) as input with pull-up */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BOOT_BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    /* Register the Ethernet data reception callback before entering the loop */
    Eth_CallBackSetUp();

    /* Populate the static broadcast body with our service list */
    memcpy(ZECU_Frame_Body_Broadcast.Services, ZECUFrame_ServiceList, sizeof(ZECUFrame_Service_t) * ZECU_FRAME_SERVICE_NUM);

    /* Wait for system Initialization to pass phase 3 */
    while(3 >= GlobalInit_GetLevel()){vTaskDelay(pdMS_TO_TICKS(50));}
    GlobalInit_MoveNextLevel();

    ZECUFrame_Generic_t tx_frame;
    memset(&tx_frame, 0, sizeof(ZECUFrame_Generic_t));
    memcpy(&tx_frame.Header, &ZECU_Frame_Header, sizeof(ZECUFrame_Header_t));

    static uint8_t sync_num = 0;
    
    /* Attempt to restore connection from NVS */
    if (NVS_LoadCCUEndpoint()) {
        App_HeartBeat_State = eSTATE_CONNECTED;
        last_keepalive_time = xTaskGetTickCount();
        SysLog("HeartBeatRuntime(...): Restored previous CCU connection from NVS.");
    }

    /* Infinite loop to periodically process and dispatch heartbeat frames */
    while (1) {
        /* --- HARDWARE RESET TRIGGER --- */
        /* Check if BOOT button is pressed (Active LOW) */
        if (gpio_get_level(BOOT_BUTTON_PIN) == 0) {
            SysLog("HeartBeatRuntime(...): BOOT Button pressed! Forcing network reset...");
            NVS_EraseCCUEndpoint();
            App_HeartBeat_State = eSTATE_BROADCAST;
            vTaskDelay(pdMS_TO_TICKS(500)); /* Simple debounce delay */
            continue;
        }

        /* Handle connection and authentication handshake */
        if (App_HeartBeat_State != eSTATE_CONNECTED) {
            App_ConnectCCU();
            last_keepalive_time = xTaskGetTickCount();
            continue;
        }

        /* --- CCU KEEPALIVE TIMEOUT WATCHDOG --- */
        if (App_HeartBeat_State == eSTATE_CONNECTED) {
#ifndef DISABLE_KEEPALIVE_TRACK
            if ((xTaskGetTickCount() - last_keepalive_time) > pdMS_TO_TICKS(CCU_KEEPALIVE_TIMEOUT_MS)) {
                SysErr("HeartBeatRuntime(...): CCU Keep-Alive Timeout (> %d ms)! Reverting to Broadcast.", CCU_KEEPALIVE_TIMEOUT_MS);
                App_HeartBeat_State = eSTATE_BROADCAST;
                continue;
            }
#endif
        }
        
        /* Do not dispatch telemetry if not fully connected */
        if (App_HeartBeat_State != eSTATE_CONNECTED) {
            (void)ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(HEART_TASK_CYCLE_MS));
            continue;
        }

        uint16_t 
            dist0_mm = 0, dist1_mm = 0, dist2_mm = 0, 
            dist0_cm = 0, dist1_cm = 0, dist2_cm = 0;

        if(eSERVICE_DISABLED != ServiceList[eSERVICE_ULTRA_SONIC_RUNTIME].Status){
            /* 1. Retrieve actual data from sensors and actuators */
            dist0_mm = HCSR04_GetLatestDistanceMm(0);
            dist1_mm = HCSR04_GetLatestDistanceMm(1);
            dist2_mm = HCSR04_GetLatestDistanceMm(2);
            
            /* Convert millimeters to centimeters and mask to fit into the 10-bit fields (Max: 1023) */
            dist0_cm = (dist0_mm != 0xFFFFU) ? (dist0_mm / 10U) : 0x3FF;
            dist1_cm = (dist1_mm != 0xFFFFU) ? (dist1_mm / 10U) : 0x3FF;
            dist2_cm = (dist2_mm != 0xFFFFU) ? (dist2_mm / 10U) : 0x3FF;

            /* --- FAIL-SAFE CHECK --- */
            /* Check if any sensor distance is below the emergency threshold */
            if (dist0_cm < SF_ETT_Distance[0]) {
                SysErr("HeartBeatRuntime(...): EMERGENCY! Sensor 0 distance (%u cm) is below threshold (%u cm).", dist0_cm, SF_ETT_Distance[0]);
                EmergencyStop();
            }
            
            /* Check if sensor 1 is below threshold */
            if (dist1_cm < SF_ETT_Distance[1]) {
                SysErr("HeartBeatRuntime(...): EMERGENCY! Sensor 1 distance (%u cm) is below threshold (%u cm).", dist1_cm, SF_ETT_Distance[1]);
                EmergencyStop();
            }
            
            /* Check if sensor 2 is below threshold */
            if (dist2_cm < SF_ETT_Distance[2]) {
                SysErr("HeartBeatRuntime(...): EMERGENCY! Sensor 2 distance (%u cm) is below threshold (%u cm).", dist2_cm, SF_ETT_Distance[2]);
                EmergencyStop();
            }
        }

        /* Scale actual motor speeds from 0~1024 to -100~100 */
        int32_t m0 = MotorGetSpeed0();
        int32_t m1 = MotorGetSpeed1();
        /// SysLog("HeartBeatRuntime(...): m0=%d, m0=%d", m0, m1);
        m0 = ConvertRaw2Percent(m0);
        m1 = ConvertRaw2Percent(m1);
        /// SysLog("HeartBeatRuntime(...): Adjusted, m0=%d, m0=%d", m0, m1);

        /* 2. Pack the retrieved information into the generic frame structure */
        App_WriteFrame_HeartBeat(&tx_frame, sync_num, dist0_cm, dist1_cm, dist2_cm, (int8_t)m0, (int8_t)m1, Loc_Long, Loc_Lat);

        sync_num = (sync_num + 1) % 4;

        /* 3. Dispatch the packed UDP payload to the dynamically paired CCU */
        ReturnCode_t ret = Eth_SendUDPPacket(CCU_EP.Member.Addr.Dword, CCU_EP.Member.Port.Word, (uint8_t*)&tx_frame, sizeof(ZECUFrame_Generic_t));
        
        /* Evaluate the transmission status and log the outcome */
        if (ret == STAT_OKE) {
            SysLog("HeartBeatRuntime(...): Sent ECU State -> D0: %u, D1: %u, D2: %u, M0: %d, M1: %d, SyncNum: %u", 
                dist0_cm, dist1_cm, dist2_cm, m0, m1, (sync_num - 1) & 0x03);
        } else {
            SysErr("HeartBeatRuntime(...): Failed to send ECU State! Error: %d", ret);
        }

        /* 4. Suspend task execution for the defined cycle duration */
        /// vTaskDelay(pdMS_TO_TICKS(HEART_TASK_CYCLE_MS));
        (void)ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(HEART_TASK_CYCLE_MS));
    }

    SysExit("HeartBeatRuntime");
    
    /* Delete task to clean up resources if loop exits */
    vTaskDelete(NULL);
}