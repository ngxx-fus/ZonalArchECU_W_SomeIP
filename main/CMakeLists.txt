idf_component_register(
    SRCS 
        "MainApp.c"
    INCLUDE_DIRS 
        "."
    REQUIRES 
        AppRT
)

target_include_directories(${COMPONENT_TARGET}
    PUBLIC
    ../AppRT
)
