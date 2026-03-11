idf_component_register(
    SRCS 
        "xFindMyMoto.c"
    INCLUDE_DIRS 
        "."
    REQUIRES 
        bt              # /*Required for Bluetooth APIs*/
        nvs_flash       # /*Required for storing pairing keys*/
        esp_driver_gpio 
        esp_driver_spi 
        esp_timer 
        AppConfig 
        AppESPWrap 
        AppUtils
)

target_include_directories(${COMPONENT_TARGET}
    PUBLIC
    ../AppConfig
    ../AppESPWrap
    ../AppUtils
)
