# @brief Collect all source files from subdirectories
set(srcs
    "EthernetW5500/RxedpacketQueue.c"
    "EthernetW5500/EthernetW5500.c"
    "EthernetW5500/Network.c"
    "EthernetW5500/Module.c"
    "EthernetW5500/E2E.c"
    
    "UltraSonic/UltraSonic.c"
    
    "HBridge/Module.c"
    "HBridge/HBridge.c"
    
    "Dummy/Dummy.c"

    "__Shared/RoutingMatrix.c"
)

# @brief Export include directories so other components can find the headers
set(include_dirs
    "."
    "Dummy"
    "HBridge"
    "__Shared"
    "EthernetW5500"
    "UltraSonic"
)

# @brief Register the component with ESP-IDF build system
idf_component_register(
    SRCS "${srcs}"
    INCLUDE_DIRS "${include_dirs}"
    REQUIRES 
        "AppConfig"
        "AppUtils" 
        "AppESPWrap" 
        esp_driver_gpio 
        esp_driver_spi 
        esp_timer 
        driver 
        soc 
        esp_hw_support 
        esp_driver_ledc
        esp_hal_gpspi
)

