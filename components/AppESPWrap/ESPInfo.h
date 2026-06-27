#ifndef __APP_ESP_WRAP_ESP32_INFO_
#define __APP_ESP_WRAP_ESP32_INFO_

#include "esp_mac.h"

/**
 * @brief Retrieves the factory-burned base MAC address from eFuse.
 * 
 * @note In case of a failure, the first 5 bytes of the buffer will be 
 *       filled with 0x00, and the 6th byte will contain the error code.
 *
 * @param mac_buffer A 6-byte array to store the retrieved MAC address.
 */
void GetBaseEPSPhysAddress(uint8_t *mac_buffer);

#endif /*__APP_ESP_WRAP_ESP32_INFO_*/