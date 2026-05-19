##DEPRECATED## idf_component_register(
##DEPRECATED##     SRCS
##DEPRECATED##         "SOMEIP/Service.c"
##DEPRECATED##         "StartUp/Service.c"
##DEPRECATED##         "HeartBeat/HeartBeat.c"
##DEPRECATED##     INCLUDE_DIRS
##DEPRECATED##         "."
##DEPRECATED##         "SOMEIP/"
##DEPRECATED##         "StartUp/"
##DEPRECATED##         "HeartBeat"
##DEPRECATED##     REQUIRES 
##DEPRECATED##         AppUtils
##DEPRECATED##         AppConfig
##DEPRECATED##         AppESPWrap
##DEPRECATED##         AppComm
##DEPRECATED## )

# @brief Check if CMake is running in script mode (ESP-IDF requirement extraction phase)
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
        AppUtils
        AppConfig
        AppESPWrap
        AppComm
)
