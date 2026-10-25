# Recursively find all C source files in all subfolders of components/
file(GLOB_RECURSE component_srcs
    "${CMAKE_CURRENT_LIST_DIR}/*.c"
)

# Recursively find all directories containing header (.h) files
file(GLOB_RECURSE component_headers
    "${CMAKE_CURRENT_LIST_DIR}/*.h"
)

set(component_include_dirs "")
foreach(header ${component_headers})
    get_filename_component(header_dir ${header} DIRECTORY)
    list(APPEND component_include_dirs ${header_dir})
endforeach()

if(component_include_dirs)
    list(REMOVE_DUPLICATES component_include_dirs)
    # Sort so that parent directories come before subdirectories
    list(SORT component_include_dirs)
else()
    set(component_include_dirs ".")
endif()

# Register the 'components' component
idf_component_register(
    SRCS ${component_srcs}
    INCLUDE_DIRS ${component_include_dirs}
    REQUIRES esp_system log
)

