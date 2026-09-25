add_library(applecat_core STATIC
    src/coreinit.cpp
    src/appsettings.cpp
    src/encodingservice.cpp
    src/asyncio.cpp
    src/syntaxrepository.cpp
    src/syntaxhighlighter.cpp
    src/completion.cpp
    src/selftest.cpp

    src/coreinit.h
    src/appsettings.h
    src/encodingservice.h
    src/asyncio.h
    src/syntaxdefinition.h
    src/syntaxrepository.h
    src/syntaxhighlighter.h
    src/completion.h
    src/selftest.h
)

add_library(AppleCat::core ALIAS applecat_core)

target_include_directories(applecat_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(applecat_core
    PUBLIC
        Qt6::Core
        Qt6::Gui
        AppleCat::interfaces
)
target_compile_definitions(applecat_core PRIVATE APPLECAT_VERSION="${PROJECT_VERSION}")
