# Updated: 14:40 - May 15, 2026

## @brief Check if CMake is running in script mode (ESP-IDF requirement extraction phase)
if(CMAKE_SCRIPT_MODE_FILE)
    # @brief Script mode does not support CONFIGURE_DEPENDS flag
    file(GLOB_RECURSE srcs "*.c")
    file(GLOB_RECURSE hdrs "*.h")
else()
    # @brief Normal configure mode, use CONFIGURE_DEPENDS to track file additions/removals
    file(GLOB_RECURSE srcs CONFIGURE_DEPENDS "*.c")
    file(GLOB_RECURSE hdrs CONFIGURE_DEPENDS "*.h")
endif()

set(include_dirs ".")

# @brief Extract directory path from each header file
foreach(hdr_file ${hdrs})
    get_filename_component(dir ${hdr_file} DIRECTORY)
    file(RELATIVE_PATH rel_dir ${CMAKE_CURRENT_LIST_DIR} ${dir})
    list(APPEND include_dirs ${rel_dir})
endforeach()

# @brief Remove duplicated include directories
if(include_dirs)
    list(REMOVE_DUPLICATES include_dirs)
endif()

# @brief Register the component with ESP-IDF build system
idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS ${include_dirs}
    REQUIRES 
        "AppConfig"
        "AppBase" 
        "AppESPWrap" 
        esp_driver_gpio 
        esp_driver_spi 
        esp_timer 
        driver 
        soc 
        esp_hw_support 
        esp_driver_ledc
        esp_hal_gpspi
        esp_rom
        esp_driver_uart
)






#DEPRECATED## # @brief Collect all source files from subdirectories
##DEPRECATED## set(srcs
##DEPRECATED##     "EthernetW5500/EthernetW5500.c"
##DEPRECATED##     "EthernetW5500/PacketQueue.c"
##DEPRECATED##     "EthernetW5500/Network.c"
##DEPRECATED##     "EthernetW5500/Module.c"
##DEPRECATED##     "EthernetW5500/E2E.c"
##DEPRECATED##     
##DEPRECATED##     "UltraSonic/Module.c"
##DEPRECATED##     "UltraSonic/UltraSonic.c"
##DEPRECATED##     
##DEPRECATED##     "HBridge/Module.c"
##DEPRECATED##     "HBridge/HBridge.c"
##DEPRECATED##     
##DEPRECATED##     "Dummy/Dummy.c"
##DEPRECATED## 
##DEPRECATED##     "Routing/RoutingMatrix.c"
##DEPRECATED##     "Routing/E2EComputeHelper.c"
##DEPRECATED## )
##DEPRECATED## 
##DEPRECATED## # @brief Export include directories so other components can find the headers
##DEPRECATED## set(include_dirs
##DEPRECATED##     "."
##DEPRECATED##     "Dummy"
##DEPRECATED##     "HBridge"
##DEPRECATED##     "Routing"
##DEPRECATED##     "EthernetW5500"
##DEPRECATED##     "UltraSonic"
##DEPRECATED## )
##DEPRECATED## 
##DEPRECATED## # @brief Register the component with ESP-IDF build system
##DEPRECATED## idf_component_register(
##DEPRECATED##     SRCS "${srcs}"
##DEPRECATED##     INCLUDE_DIRS "${include_dirs}"
##DEPRECATED##     REQUIRES 
##DEPRECATED##         "AppConfig"
##DEPRECATED##         "AppBase" 
##DEPRECATED##         "AppESPWrap" 
##DEPRECATED##         esp_driver_gpio 
##DEPRECATED##         esp_driver_spi 
##DEPRECATED##         esp_timer 
##DEPRECATED##         driver 
##DEPRECATED##         soc 
##DEPRECATED##         esp_hw_support 
##DEPRECATED##         esp_driver_ledc
##DEPRECATED##         esp_hal_gpspi
##DEPRECATED## )
##DEPRECATED##


