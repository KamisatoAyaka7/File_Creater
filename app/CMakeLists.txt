add_executable(AppleCat WIN32
    src/main.cpp
    src/codeeditor.h
    src/codeeditor.cpp
    src/completionpopup.h
    src/completionpopup.cpp
    src/findreplacebar.h
    src/findreplacebar.cpp
    src/docview.h
    src/editortab.h
    src/editortab.cpp
    src/hexeditor.h
    src/hexeditor.cpp
    src/settingsdialog.h
    src/settingsdialog.cpp
    src/pluginhost.h
    src/pluginhost.cpp
    src/mainwindow.h
    src/mainwindow.cpp
    ../core/core.qrc
)

target_include_directories(AppleCat PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(AppleCat PRIVATE AppleCat::core Qt6::Widgets)

if(WIN32)
    # Let --selftest print to the launching console.
    target_link_libraries(AppleCat PRIVATE user32)

    # Compile the icon resource manually: CMake's RC path would pass the
    # project's include directories to windres, and they contain spaces
    # (".../Aurora Heliarchy/...") which windres mangles. The .rc needs no
    # includes, so a plain invocation with the source dir as working
    # directory is sufficient.
    if(CMAKE_RC_COMPILER)
        set(_icon_obj ${CMAKE_CURRENT_BINARY_DIR}/applecat_icon.obj)
        add_custom_command(
            OUTPUT ${_icon_obj}
            COMMAND ${CMAKE_RC_COMPILER} -O coff -i applecat.rc -o ${_icon_obj}
            DEPENDS applecat.rc logo.ico
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            VERBATIM
            COMMENT "Compiling icon resource")
        set_source_files_properties(${_icon_obj} PROPERTIES
            EXTERNAL_OBJECT TRUE
            GENERATED TRUE)
        target_sources(AppleCat PRIVATE ${_icon_obj})
    endif()
endif()

set_target_properties(AppleCat PROPERTIES
    OUTPUT_NAME AppleCat
    WIN32_EXECUTABLE TRUE
)
