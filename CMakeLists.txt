cmake_minimum_required(VERSION 3.16)
project(AndroidToolbox VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Build against Qt5 on Linux. This project is configured for Qt5 and the
# Qt6 fallback causes the current CMake failure on systems that only have Qt5
# installed.
find_package(Qt5 REQUIRED COMPONENTS Core Widgets Concurrent)
set(QT_VERSION_MAJOR 5)
message(STATUS "Using Qt5")

# Qt5 translation tools are optional in some cross-toolchains (e.g. MXE minimal installs).
# If they are absent, skip translation generation instead of failing the entire build.
find_package(Qt5LinguistTools QUIET)
set(HAS_TRANSLATION_TOOLS ${Qt5LinguistTools_FOUND})

if(HAS_TRANSLATION_TOOLS)
    message(STATUS "Translation tools available")
else()
    message(STATUS "Translation tools unavailable; building without generated .qm files")
endif()

set(TS_FILES
    translations/toolbox_en.ts
    translations/toolbox_zh_CN.ts
    translations/toolbox_zh_TW.ts
    translations/toolbox_ru.ts
    translations/toolbox_ja.ts
)

set(SOURCES
    main.cpp
    mainwindow.cpp
    appsettings.cpp
    adbrunner.cpp
    theme/materialtheme.cpp
    theme/iconhelper.cpp
    dialogs/touchsimudialog.cpp
    dialogs/keysimudialog.cpp
    dialogs/softsimudialog.cpp
    dialogs/sysctldialog.cpp
    dialogs/runcontroldialog.cpp
    dialogs/newscriptdialog.cpp
)

set(HEADERS
    mainwindow.h
    appsettings.h
    adbrunner.h
    theme/materialtheme.h
    theme/iconhelper.h
    dialogs/touchsimudialog.h
    dialogs/keysimudialog.h
    dialogs/softsimudialog.h
    dialogs/sysctldialog.h
    dialogs/runcontroldialog.h
    dialogs/newscriptdialog.h
)

set(RESOURCES
    resources/app.qrc
)

add_executable(toolbox
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
)

if(QT_VERSION_MAJOR EQUAL 5)
    if(HAS_TRANSLATION_TOOLS)
        qt5_add_translation(QM_FILES ${TS_FILES})
        target_sources(toolbox PRIVATE ${QM_FILES})
    endif()
    target_link_libraries(toolbox PRIVATE
        Qt5::Core
        Qt5::Widgets
        Qt5::Concurrent
    )
endif()

target_include_directories(toolbox PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(toolbox PROPERTIES
    OUTPUT_NAME "toolbox"
)

include(GNUInstallDirs)
install(TARGETS toolbox
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
