#include "Module.h"
#include "driver/uart.h"
#include <string.h>

#define LOC_UART_NUM  UART_NUM_1
#define LOC_UART_BAUD 9600
#define LOC_BUF_SIZE  1024

volatile uint32_t Loc_Long = 106660172; // Default to HCMC Longitude
volatile uint32_t Loc_Lat  = 10762622;  // Default to HCMC Latitude

#ifndef LOC_LOG_EN
    #define LOC_LOG_EN 0
#endif

#if (LOC_LOG_EN == 1)
    #define LocLog(...)         SysLog(__VA_ARGS__)
#else 
    #define LocLog(...)
#endif

static uint32_t NmeaToMicroDegrees(const char* str) {
    uint32_t degrees = 0, minutes_int = 0, minutes_frac = 0, frac_divisor = 1;
    const char* dot = strchr(str, '.');
    if (dot == NULL) return 0;
    
    int int_len = dot - str;
    if (int_len < 3) return 0; 
    
    for (int i = 0; i < int_len - 2; i++) degrees = degrees * 10 + (str[i] - '0');
    minutes_int = (str[int_len - 2] - '0') * 10 + (str[int_len - 1] - '0');
    
    const char* p = dot + 1;
    while (*p >= '0' && *p <= '9') {
        minutes_frac = minutes_frac * 10 + (*p - '0');
        frac_divisor *= 10;
        p++;
    }
    
    uint64_t min_total_micro = ((uint64_t)minutes_int * 1000000ULL) + 
                               (((uint64_t)minutes_frac * 1000000ULL) / frac_divisor);
    return (degrees * 1000000U) + (uint32_t)(min_total_micro / 60U);
}

ReturnCode_t ParseLocation(void* Buff, void* LocationLong, void* LocationLat) {
    if (Buff == NULL || LocationLong == NULL || LocationLat == NULL) return STAT_ERR_NULL;
    const char* str = (const char*)Buff;
    
    /* Control flow: Hỗ trợ RMC (trường 2,3,5) hoặc GGA (trường 2,4,6) */
    const char* type = NULL;
    uint8_t status_idx = 0, lat_idx = 0, lon_idx = 0;
    
    if (strstr(str, "RMC,")) {
        type = "RMC,"; status_idx = 2; lat_idx = 3; lon_idx = 5;
    } else if (strstr(str, "GGA,")) {
        type = "GGA,"; status_idx = 6; lat_idx = 2; lon_idx = 4;
    } else return STAT_ERR;

    const char* p = strstr(str, type);
    uint8_t comma_count = 0;
    const char *status_ptr = NULL, *lat_ptr = NULL, *lon_ptr = NULL;

    while (*p && *p != '*') {
        if (*p == ',') {
            comma_count++;
            if (comma_count == status_idx) status_ptr = p + 1;
            else if (comma_count == lat_idx) lat_ptr = p + 1;
            else if (comma_count == lon_idx) lon_ptr = p + 1;
        }
        p++;
    }

    if (!status_ptr || !lat_ptr || !lon_ptr) return STAT_ERR;
    if (type[0] == 'R' && *status_ptr != 'A') return STAT_ERR; // RMC active check
    if (type[0] == 'G' && *status_ptr == '0') return STAT_ERR; // GGA fix check

    uint32_t parsed_lat = NmeaToMicroDegrees(lat_ptr);
    uint32_t parsed_lon = NmeaToMicroDegrees(lon_ptr);

    if (parsed_lat > 0 && parsed_lon > 0) {
        *((uint32_t*)LocationLat) = parsed_lat;
        *((uint32_t*)LocationLong) = parsed_lon;
        return STAT_OKE;
    }
    return STAT_ERR;
}

ReturnCode_t Loc_Init() {
    SysEntry("Loc_Init");
    uart_config_t uart_config = {
        .baud_rate = LOC_UART_BAUD, .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE, .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE, .source_clk = UART_SCLK_APB
    };
    if (uart_driver_install(LOC_UART_NUM, LOC_BUF_SIZE * 2, 0, 0, NULL, 0) != ESP_OK) return STAT_ERR;
    uart_param_config(LOC_UART_NUM, &uart_config);
    uart_set_pin(LOC_UART_NUM, PIN_LOC_TXD, PIN_LOC_RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    LocLog("Loc_Init: ATGM336H UART%d initialized on TX:%d RX:%d", LOC_UART_NUM, PIN_LOC_TXD, PIN_LOC_RXD);
    SysExit("Loc_Init");
    return STAT_OKE;
}

ReturnCode_t Loc_ReadLocation() {
    uint8_t data[256];
    bool found = false;
    
    /* Control flow: vét sạch buffer để lấy bản tin mới nhất */
    while (true) {
        int len = uart_read_bytes(LOC_UART_NUM, data, sizeof(data) - 1, pdMS_TO_TICKS(10));
        if (len <= 0) break;
        data[len] = '\0';
        
        char* start = strstr((char*)data, "$G");
        while (start != NULL) {
            char* end = strpbrk(start, "\r\n");
            if (end) {
                *end = '\0';
                LocLog("Loc_ReadLocation(...): GPS NMEA: %s", start);
                
                uint32_t temp_long = 0, temp_lat = 0;
                if (ParseLocation(start, &temp_long, &temp_lat) == STAT_OKE) {
                    Loc_Long = temp_long;
                    Loc_Lat  = temp_lat;
                    LocLog("Loc_ReadLocation(...): GPS Parsed Successfully: Lat=%u, Lon=%u", Loc_Lat, Loc_Long);
                    found = true;
                }
                start = strstr(end + 1, "$G");
            } else break;
        }
    }
    return found ? STAT_OKE : STAT_ERR_TIMEOUT;
}

void Loc_Runtime(void* arg) {
    SysEntry("Loc_Runtime");
    while(1 >= GlobalInit_GetLevel()) vTaskDelay(pdMS_TO_TICKS(50));
    if (Loc_Init() != STAT_OKE) { vTaskDelete(NULL); return; }
    GlobalInit_MoveNextLevel();
    while (1) {
        Loc_ReadLocation();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}