add_library(statsplugin MODULE
    statsplugin.cpp
    metadata.json
)

target_link_libraries(statsplugin PRIVATE AppleCat::interfaces)

# Build into <build>/bin/plugins so the host finds it automatically.
set_target_properties(statsplugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/plugins
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/plugins
)
