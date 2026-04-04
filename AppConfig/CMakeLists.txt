idf_component_register(
    INCLUDE_DIRS 
        "."
)

set(PINOUT_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/Pinout.h")
set(PINOUT_DEF "${CMAKE_CURRENT_SOURCE_DIR}/Pinout/Pinout.h.def")
set(PINOUT_CNF "${CMAKE_CURRENT_SOURCE_DIR}/Pinout/Pinout.cnf")

# Run Makefile in Pinout directory if .def or .cnf changes
add_custom_command(
    OUTPUT "${PINOUT_HEADER}"
    COMMAND make -C "${CMAKE_CURRENT_SOURCE_DIR}/Pinout" all
    DEPENDS "${PINOUT_DEF}" "${PINOUT_CNF}"
    COMMENT "Generating Pinout.h from Pinout.cnf..."
    VERBATIM
)

# Force CMake to run generation step before building the component
add_custom_target(generate_pinout_header DEPENDS "${PINOUT_HEADER}")
add_dependencies(${COMPONENT_LIB} generate_pinout_header)