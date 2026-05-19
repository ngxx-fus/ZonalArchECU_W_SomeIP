# Updated: 14:40 - May 15, 2026

# --- Path Definitions ---

# Pinout Generation
set(PINOUT_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/Pinout.h")
set(PINOUT_DEF    "${CMAKE_CURRENT_SOURCE_DIR}/Pinout/Pinout.h.def")
set(PINOUT_CNF    "${CMAKE_CURRENT_SOURCE_DIR}/Pinout/Pinout.cnf")

# Comm Generation
set(COMM_HEADER   "${CMAKE_CURRENT_SOURCE_DIR}/Comm.h")
set(COMM_SOURCE   "${CMAKE_CURRENT_SOURCE_DIR}/Comm/Comm.c")
set(COMM_DEF_H    "${CMAKE_CURRENT_SOURCE_DIR}/Comm/Comm.h.def")
set(COMM_DEF_C    "${CMAKE_CURRENT_SOURCE_DIR}/Comm/Comm.c.def")
set(COMM_CNF      "${CMAKE_CURRENT_SOURCE_DIR}/Comm/Comm.cnf")

# --- Auto-Scanning Sources and Includes ---

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

# @brief Extract directory path from each header file
foreach(hdr_file ${hdrs})
    get_filename_component(dir ${hdr_file} DIRECTORY)
    file(RELATIVE_PATH rel_dir ${CMAKE_CURRENT_LIST_DIR} ${dir})
    list(APPEND auto_include_dirs ${rel_dir})
endforeach()

# @brief Explicitly append generated files/dirs to prevent missing them on fresh builds
list(APPEND auto_srcs "Comm/Comm.c")
list(APPEND auto_include_dirs "Comm/" "Pinout/")

# @brief Clean up duplicated items in lists
if(auto_srcs)
    list(REMOVE_DUPLICATES auto_srcs)
endif()
if(auto_include_dirs)
    list(REMOVE_DUPLICATES auto_include_dirs)
endif()

# --- Component Registration ---

idf_component_register(
    SRCS ${auto_srcs}
    INCLUDE_DIRS ${auto_include_dirs}
)

# --- Custom Commands (Code Generation) ---

# Command for Pinout.h
add_custom_command(
    OUTPUT  "${PINOUT_HEADER}"
    COMMAND make -C "${CMAKE_CURRENT_SOURCE_DIR}/Pinout" all
    DEPENDS "${PINOUT_DEF}" "${PINOUT_CNF}"
    COMMENT "Generating Pinout.h from Pinout.cnf..."
    VERBATIM
)

# Command for Comm.h and Comm.c 
add_custom_command(
    OUTPUT  "${COMM_HEADER}" "${COMM_SOURCE}"
    COMMAND make -C "${CMAKE_CURRENT_SOURCE_DIR}/Comm" all
    DEPENDS "${COMM_DEF_H}" "${COMM_DEF_C}" "${COMM_CNF}"
    COMMENT "Generating Comm files from Comm.cnf..."
    VERBATIM
)

# --- Dependency Linking ---

# Create a target for the generation process
add_custom_target(generate_config_files 
    DEPENDS "${PINOUT_HEADER}" "${COMM_HEADER}" "${COMM_SOURCE}"
)

# Force the component to wait for generation before compiling 
add_dependencies(${COMPONENT_LIB} generate_config_files)

# Mark the .c file as GENERATED so CMake doesn't complain if it is missing at startup
set_source_files_properties("${COMM_SOURCE}" PROPERTIES GENERATED TRUE)

##DEPRECATED## # --- Path Definitions ---
##DEPRECATED## 
##DEPRECATED## # Pinout Generation
##DEPRECATED## set(PINOUT_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/Pinout.h")
##DEPRECATED## set(PINOUT_DEF    "${CMAKE_CURRENT_SOURCE_DIR}/Pinout/Pinout.h.def")
##DEPRECATED## set(PINOUT_CNF    "${CMAKE_CURRENT_SOURCE_DIR}/Pinout/Pinout.cnf")
##DEPRECATED## 
##DEPRECATED## # Comm Generation
##DEPRECATED## set(COMM_HEADER   "${CMAKE_CURRENT_SOURCE_DIR}/Comm.h")
##DEPRECATED## set(COMM_SOURCE   "${CMAKE_CURRENT_SOURCE_DIR}/Comm/Comm.c")
##DEPRECATED## set(COMM_DEF_H    "${CMAKE_CURRENT_SOURCE_DIR}/Comm/Comm.h.def")
##DEPRECATED## set(COMM_DEF_C    "${CMAKE_CURRENT_SOURCE_DIR}/Comm/Comm.c.def")
##DEPRECATED## set(COMM_CNF      "${CMAKE_CURRENT_SOURCE_DIR}/Comm/Comm.cnf")
##DEPRECATED## 
##DEPRECATED## # --- Component Registration ---
##DEPRECATED## 
##DEPRECATED## idf_component_register(
##DEPRECATED##     SRCS 
##DEPRECATED##         "Comm/Comm.c"  # List the generated .c file here
##DEPRECATED##     INCLUDE_DIRS 
##DEPRECATED##         "."
##DEPRECATED##         "Comm/"
##DEPRECATED##         "Pinout/"
##DEPRECATED## )
##DEPRECATED## 
##DEPRECATED## # --- Custom Commands (Code Generation) ---
##DEPRECATED## 
##DEPRECATED## # Command for Pinout.h
##DEPRECATED## add_custom_command(
##DEPRECATED##     OUTPUT  "${PINOUT_HEADER}"
##DEPRECATED##     COMMAND make -C "${CMAKE_CURRENT_SOURCE_DIR}/Pinout" all
##DEPRECATED##     DEPENDS "${PINOUT_DEF}" "${PINOUT_CNF}"
##DEPRECATED##     COMMENT "Generating Pinout.h from Pinout.cnf..."
##DEPRECATED##     VERBATIM
##DEPRECATED## )
##DEPRECATED## 
##DEPRECATED## # Command for Comm.h and Comm.c 
##DEPRECATED## add_custom_command(
##DEPRECATED##     OUTPUT  "${COMM_HEADER}" "${COMM_SOURCE}"
##DEPRECATED##     COMMAND make -C "${CMAKE_CURRENT_SOURCE_DIR}/Comm" all
##DEPRECATED##     DEPENDS "${COMM_DEF_H}" "${COMM_DEF_C}" "${COMM_CNF}"
##DEPRECATED##     COMMENT "Generating Comm files from Comm.cnf..."
##DEPRECATED##     VERBATIM
##DEPRECATED## )
##DEPRECATED## 
##DEPRECATED## # --- Dependency Linking ---
##DEPRECATED## 
##DEPRECATED## # Create a target for the generation process
##DEPRECATED## add_custom_target(generate_config_files 
##DEPRECATED##     DEPENDS "${PINOUT_HEADER}" "${COMM_HEADER}" "${COMM_SOURCE}"
##DEPRECATED## )
##DEPRECATED## 
##DEPRECATED## # Force the component to wait for generation before compiling 
##DEPRECATED## add_dependencies(${COMPONENT_LIB} generate_config_files)
##DEPRECATED## 
##DEPRECATED## # Mark the .c file as GENERATED so CMake doesn't complain if it is missing at startup
##DEPRECATED## set_source_files_properties("${COMM_SOURCE}" PROPERTIES GENERATED TRUE)

