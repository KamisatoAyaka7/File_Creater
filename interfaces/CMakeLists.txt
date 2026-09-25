# Header-only plugin SDK shared by the application and external plugins.
add_library(applecat_interfaces INTERFACE)
add_library(AppleCat::interfaces ALIAS applecat_interfaces)

target_include_directories(applecat_interfaces INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(applecat_interfaces INTERFACE Qt6::Core Qt6::Gui Qt6::Widgets)
