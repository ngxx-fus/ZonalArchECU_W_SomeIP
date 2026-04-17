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

# --- Component Registration ---

idf_component_register(
    SRCS 
        "Comm/Comm.c"  # List the generated .c file here
    INCLUDE_DIRS 
        "."
        "Comm/"
        "Pinout/"
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