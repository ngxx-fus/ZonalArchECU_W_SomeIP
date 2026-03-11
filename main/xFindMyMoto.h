#include <stdio.h>
#include "../AppConfig/All.h"
#include "../AppESPWrap/All.h"
#include "../AppUtils/All.h"
#include "ESPFreeRTOSWrapper.h"
#include "ReturnType.h"


void xFindMyMoto_ErrorIndicate(){
    static int8_t Initialized = 0;

    if(Initialized == 0){
        IOConfigAsOutput(
            Mask64(PIN_LED_INDICATOR0),
            GPIO_PULLUP_DISABLE,
            GPIO_PULLDOWN_DISABLE
        );
        Initialized = 1;
    }

    GPIOSetHigh(Mask64(PIN_LED_INDICATOR0)); DelayMs(90);
    GPIOSetLow(Mask64(PIN_LED_INDICATOR0)); DelayMs(50);
    GPIOSetHigh(Mask64(PIN_LED_INDICATOR0)); DelayMs(70);
    GPIOSetLow(Mask64(PIN_LED_INDICATOR0)); DelayMs(50);
    GPIOSetHigh(Mask64(PIN_LED_INDICATOR0)); DelayMs(90);
    GPIOSetLow(Mask64(PIN_LED_INDICATOR0)); DelayMs(50);
    GPIOSetHigh(Mask64(PIN_LED_INDICATOR0)); DelayMs(70);
    GPIOSetLow(Mask64(PIN_LED_INDICATOR0));
}


// BLE /////////////////////////////////////////////////////
#include <string.h>
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"


#define TEST_SERVICE_UUID      0x00FF
#define TEST_CHAR_UUID         0xFF01
#define TEST_DEVICE_NAME       "xFindMyMoto"
#define TEST_APP_ID            0

static uint16_t G_GattsIf = ESP_GATT_IF_NONE;
static uint16_t G_ServiceHandle;
static uint16_t G_CharHandle;

/// @brief Configure BLE security parameters for Just Works mode
void BLE_SetMode_JustWorks(void) {
    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_NO_BOND;
    esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;
    uint8_t key_size = 16;
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;

    /*Before function call to set authentication request mode*/
    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
    /*Before function call to set IO capability to none*/
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
    /*Before function call to set maximum encryption key size*/
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
    /*Before function call to set initiation key mask*/
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
    /*Before function call to set response key mask*/
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));
}

/// @brief Configure advertising data and device name
void BLE_ConfigAdvertising(const char *dev_name) {
    esp_ble_adv_data_t adv_data = {
        .set_scan_rsp = false,
        .include_name = true,
        .include_txpower = true,
        .min_interval = 0x0006,
        .max_interval = 0x0010,
        .appearance = 0x00,
        .manufacturer_len = 0,
        .p_manufacturer_data = NULL,
        .service_data_len = 0,
        .p_service_data = NULL,
        .service_uuid_len = 0,
        .p_service_uuid = NULL,
        .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
    };

    /*Before function call to set local device name*/
    esp_ble_gap_set_device_name(dev_name);
    /*Before function call to configure advertising raw data*/
    esp_ble_gap_config_adv_data(&adv_data);
}

/// @brief Handle GAP related events such as advertising and security
void BLE_GapHandler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    /*Before control flow to handle various GAP events*/
    switch (event) {
        /*Before control flow to handle advertising data set completion*/
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            esp_ble_adv_params_t adv_params = {
                .adv_int_min = 0x20,
                .adv_int_max = 0x40,
                .adv_type = ADV_TYPE_IND,
                .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
                .channel_map = ADV_CHNL_ALL,
                .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
            };
            /*Before function call to start advertising*/
            esp_ble_gap_start_advertising(&adv_params);
            /*Before termination*/
            break;

        /*Before control flow to handle security request from peer*/
        case ESP_GAP_BLE_SEC_REQ_EVT:
            /*Before function call to accept security request*/
            esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
            /*Before termination*/
            break;

        /*Before control flow to handle authentication completion*/
        case ESP_GAP_BLE_AUTH_CMPL_EVT:
            /*Before control flow to check if authentication succeeded*/
            if (param->ble_security.auth_cmpl.success) {
                /*Before function call to log success*/
                SysLog("BLE: Pairing success");
            } else {
                /*Before function call to log failure*/
                SysErr("BLE: Pairing failed, reason: 0x%x", param->ble_security.auth_cmpl.fail_reason);
            }
            /*Before termination*/
            break;

        /*Before termination*/
        default:
            /*Before termination*/
            break;
    }
}

/// @brief Handle GATT Server events including connections and data writes
void BLE_GattsHandler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    /*Before control flow to handle GATT events*/
    switch (event) {
        /*Before control flow to handle application registration*/
        case ESP_GATTS_REG_EVT:
            /*Before function call to configure advertising*/
            BLE_ConfigAdvertising(TEST_DEVICE_NAME);
            esp_gatt_srvc_id_t service_id = {
                .is_primary = true,
                .id.inst_id = 0,
                .id.uuid.len = ESP_UUID_LEN_16,
                .id.uuid.uuid.uuid16 = TEST_SERVICE_UUID,
            };
            /*Before function call to create service*/
            esp_ble_gatts_create_service(gatts_if, &service_id, 4);
            /*Before termination*/
            break;

        /*Before control flow to handle service creation completion*/
        case ESP_GATTS_CREATE_EVT:
            G_ServiceHandle = param->create.service_handle;
            /*Before function call to start the service*/
            esp_ble_gatts_start_service(G_ServiceHandle);
            esp_bt_uuid_t char_uuid = {
                .len = ESP_UUID_LEN_16,
                .uuid.uuid16 = TEST_CHAR_UUID,
            };
            /*Before function call to add characteristic with write property*/
            esp_ble_gatts_add_char(G_ServiceHandle, &char_uuid, ESP_GATT_PERM_WRITE, 
                                   ESP_GATT_CHAR_PROP_BIT_WRITE, NULL, NULL);
            /*Before termination*/
            break;

        /*Before control flow to handle characteristic addition completion*/
        case ESP_GATTS_ADD_CHAR_EVT:
            G_CharHandle = param->add_char.attr_handle;
            /*Before termination*/
            break;

        /*Before control flow to handle peer connection*/
        case ESP_GATTS_CONNECT_EVT:
            /*Before function call to log connection*/
            SysLog("BLE: Device connected, conn_id: %d", param->connect.conn_id);
            /*Before termination*/
            break;

        /*Before control flow to handle peer disconnection*/
        case ESP_GATTS_DISCONNECT_EVT:
            /*Before function call to log disconnection*/
            SysLog("BLE: Device disconnected");
            /*Before function call to restart advertising*/
            esp_ble_gap_start_advertising(&(esp_ble_adv_params_t){
                .adv_int_min = 0x20, .adv_int_max = 0x40, .adv_type = ADV_TYPE_IND,
                .own_addr_type = BLE_ADDR_TYPE_PUBLIC, .channel_map = ADV_CHNL_ALL,
                .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY
            });
            /*Before termination*/
            break;

        /*Before control flow to handle data written by phone*/
        case ESP_GATTS_WRITE_EVT:
            /*Before function call to log incoming message*/
            SysLog("BLE: Received message, length %d: %.*s", param->write.len, param->write.len, param->write.value);
            /*Notice*/
            xFindMyMoto_ErrorIndicate();
            /*Before control flow to check if response is needed*/
            if (param->write.need_rsp) {
                /*Before function call to send response*/
                esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
            }
            /*Before termination*/
            break;

        /*Before termination*/
        default:
            /*Before termination*/
            break;
    }
}

/// @brief Initialize NVS, BT Controller, and Bluedroid Stack
ReturnCode_t BLE_Init(void) {
    esp_err_t ret;

    /*Before function call to initialize NVS flash*/
    ret = nvs_flash_init();
    /*Before control flow to verify NVS state*/
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        /*Before function call to erase corrupted NVS*/
        ESP_ERROR_CHECK(nvs_flash_erase());
        /*Before function call to re-init NVS*/
        ret = nvs_flash_init();
    }
    /*Before function call to check NVS result*/
    ESP_ERROR_CHECK(ret);

    /*Before function call to release classic BT memory*/
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    /*Before function call to init controller*/
    ret = esp_bt_controller_init(&bt_cfg);
    /*Before control flow to verify controller init*/
    if (ret != ESP_OK) {
        /*Before termination*/
        return STAT_ERR;
    }

    /*Before function call to enable controller in BLE mode*/
    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    /*Before control flow to verify controller enablement*/
    if (ret != ESP_OK) {
        /*Before termination*/
        return STAT_ERR;
    }

    /*Before function call to init host stack*/
    ret = esp_bluedroid_init();
    /*Before control flow to verify host stack init*/
    if (ret != ESP_OK) {
        /*Before termination*/
        return STAT_ERR;
    }

    /*Before function call to enable host stack*/
    ret = esp_bluedroid_enable();
    /*Before control flow to verify host stack enablement*/
    if (ret != ESP_OK) {
        /*Before termination*/
        return STAT_ERR;
    }

    /*Before function call to register GATT callback*/
    esp_ble_gatts_register_callback(BLE_GattsHandler);
    /*Before function call to register GAP callback*/
    esp_ble_gap_register_callback(BLE_GapHandler);
    /*Before function call to register application ID*/
    esp_ble_gatts_app_register(TEST_APP_ID);
    /*Before function call to apply security settings*/
    BLE_SetMode_JustWorks();

    /*Before termination*/
    return STAT_OKE;
}
////////////////////////////////////////////////////////////


ReturnCode_t xFindMyMoto_Init() {
    SysEntry("xFindMyMoto_Init");
    return BLE_Init();
}

void xFindMyMoto_Main() {
    ReturnCode_t RetVal;
    
    SysEntry("xFindMyMoto_Main");
    
    while((RetVal = xFindMyMoto_Init()) != STAT_OKE){
        SysErr("xFindMyMoto_Main(...): xFindMyMoto_Init has returned non-zero CODE=%d", RetVal);
        xFindMyMoto_ErrorIndicate();
        DelayMs(500);
    };

    while(1){
        DelayMs(5000);
    }
}

