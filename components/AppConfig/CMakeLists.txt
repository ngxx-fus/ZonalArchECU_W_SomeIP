cmake_minimum_required(VERSION 3.31)

# --- Path Definitions ---
set(PINOUT_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/Pinout/Pinout.h")
set(PINOUT_DEF    "${CMAKE_CURRENT_SOURCE_DIR}/Pinout/Pinout.h.def")

set(COMM_HEADER   "${CMAKE_CURRENT_SOURCE_DIR}/Comm/Comm.h")
set(COMM_SOURCE   "${CMAKE_CURRENT_SOURCE_DIR}/Comm/Comm.c")
set(COMM_DEF_H    "${CMAKE_CURRENT_SOURCE_DIR}/Comm/Comm.h.def")
set(COMM_DEF_C    "${CMAKE_CURRENT_SOURCE_DIR}/Comm/Comm.c.def")
set(SYSTEM_CONFIG_YAML "${CMAKE_CURRENT_SOURCE_DIR}/SystemConfig.yml")
set(CONFIG_GENERATOR   "${CMAKE_CURRENT_SOURCE_DIR}/generate_config.py")

# --- Auto-Scanning Sources and Includes ---

# Check if CMake is running in script mode
if(CMAKE_SCRIPT_MODE_FILE)
    file(GLOB_RECURSE srcs "*.c")
    file(GLOB_RECURSE hdrs "*.h")
else()
    file(GLOB_RECURSE srcs CONFIGURE_DEPENDS "*.c")
    file(GLOB_RECURSE hdrs CONFIGURE_DEPENDS "*.h")
endif()

# Extract directory path from each header file
foreach(hdr_file ${hdrs})
    get_filename_component(dir ${hdr_file} DIRECTORY)
    file(RELATIVE_PATH rel_dir ${CMAKE_CURRENT_LIST_DIR} ${dir})
    list(APPEND auto_include_dirs ${rel_dir})
endforeach()

# Clean up duplicated include items
if(auto_include_dirs)
    list(REMOVE_DUPLICATES auto_include_dirs)
endif()

# Explicitly append the generated C file
list(APPEND srcs "${COMM_SOURCE}")
if(srcs)
    list(REMOVE_DUPLICATES srcs)
endif()

# --- Component Registration ---

# MUST be called before any custom commands/targets
idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS ${auto_include_dirs}
)

# --- Custom Commands & Dependencies ---

# Command for generating configurations via Python
add_custom_command(
    OUTPUT  "${PINOUT_HEADER}" "${COMM_HEADER}" "${COMM_SOURCE}"
    COMMAND python ${CONFIG_GENERATOR} ${SYSTEM_CONFIG_YAML}
    DEPENDS "${PINOUT_DEF}" "${COMM_DEF_H}" "${COMM_DEF_C}" "${SYSTEM_CONFIG_YAML}" "${CONFIG_GENERATOR}"
    COMMENT "Generating Config files from SystemConfig.yml using Python..."
    VERBATIM
)

# Create a target for the generation process
add_custom_target(generate_config_files 
    DEPENDS "${PINOUT_HEADER}" "${COMM_HEADER}" "${COMM_SOURCE}"
)

# Force the component to wait for code generation before compiling 
add_dependencies(${COMPONENT_LIB} generate_config_files)

# Mark the .c file as GENERATED so CMake ignores missing files during config
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
