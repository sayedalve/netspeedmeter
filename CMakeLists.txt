cmake_minimum_required(VERSION 3.21)

project(NetSpeedMeter
    VERSION 1.0.0
    DESCRIPTION "Lightweight Network Speed Meter"
    LANGUAGES CXX
)

# ── C++ standard ─────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Qt6 auto-tools ────────────────────────────────────────────────────────────
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ── Find Qt6 modules ──────────────────────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    Network
)

qt_standard_project_setup()

# ── Source files ──────────────────────────────────────────────────────────────
set(NSM_SOURCES
    src/main.cpp

    # Config subsystem
    src/config/AppConfig.h
    src/config/ConfigManager.h
    src/config/ConfigManager.cpp

    # Core network engine
    src/core/NetworkPoller.h
    src/core/NetworkPoller.cpp
    src/core/SpeedSample.h

    # Settings Dialog (Moved to ui/ by AI)
    src/ui/SettingsDialog.h
    src/ui/SettingsDialog.cpp

    # UI — Tray Manager
    src/ui/TrayManager.h
    src/ui/TrayManager.cpp

    # UI — Frameless Speed Widget
    src/ui/SpeedWidget.h
    src/ui/SpeedWidget.cpp
)

# ── Executable ───────────────────────────────────────────────────────────────
if(WIN32)
    qt_add_executable(NetSpeedMeter WIN32 ${NSM_SOURCES})
else()
    qt_add_executable(NetSpeedMeter ${NSM_SOURCES})
endif()

# ── Include paths ─────────────────────────────────────────────────────────────
target_include_directories(NetSpeedMeter PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ── Link libraries ────────────────────────────────────────────────────────────
target_link_libraries(NetSpeedMeter PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
)

if(WIN32)
    target_link_libraries(NetSpeedMeter PRIVATE
        iphlpapi
        ws2_32
    )
    target_compile_definitions(NetSpeedMeter PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        UNICODE
        _UNICODE
    )
endif()

# ── Compiler warnings ─────────────────────────────────────────────────────────
if(MSVC)
    target_compile_options(NetSpeedMeter PRIVATE /W4 /WX-)
else()
    target_compile_options(NetSpeedMeter PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ── Release optimisations ────────────────────────────────────────────────────
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        target_compile_options(NetSpeedMeter PRIVATE /O2 /GL)
        target_link_options(NetSpeedMeter   PRIVATE /LTCG)
    else()
        target_compile_options(NetSpeedMeter PRIVATE -O3)
    endif()
endif()

# ── windeployqt ───────────────────────────────────────────────────────────────
if(WIN32)
    find_program(WINDEPLOYQT windeployqt HINTS "${Qt6_DIR}/../../../bin")
    if(WINDEPLOYQT)
        add_custom_command(TARGET NetSpeedMeter POST_BUILD
            COMMAND "${WINDEPLOYQT}"
                    --no-translations
                    --no-compiler-runtime
                    "$<TARGET_FILE:NetSpeedMeter>"
            COMMENT "Running windeployqt..."
        )
    endif()
endif()