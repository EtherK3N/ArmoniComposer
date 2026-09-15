cmake_minimum_required(VERSION 3.22)
project(ArmoniComposer VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# JUCE Resolution: Subdirectory -> Installed Package -> FetchContent Auto-Download
# ---------------------------------------------------------------------------
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
else()
    find_package(JUCE CONFIG QUIET)
    if (NOT JUCE_FOUND)
        message(STATUS "JUCE not found locally. Automatically downloading via FetchContent...")
        include(FetchContent)
        FetchContent_Declare(
            JUCE
            GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
            GIT_TAG 7.0.12
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(JUCE)
    endif()
endif()


# ===========================================================================
# Main GUI Application
# ===========================================================================
juce_add_gui_app(ArmoniComposer
    PRODUCT_NAME "Armoni Composer"
    COMPANY_NAME "EtherK3N"
    VERSION 0.1.0
)

target_sources(ArmoniComposer PRIVATE
    Source/Main.cpp
    Source/MainComponent.cpp
    Source/DeviceManager.cpp
    Source/RawInputHandler.cpp
    Source/MappingEngine.cpp
    Source/AudioEngine.cpp
    Source/MetronomeClock.cpp
    Source/BpmQuantizer.cpp
    Source/ShiftLayerSystem.cpp
    Source/DspFxRack.cpp
    Source/MappingEditorComponent.cpp
    Source/PerformanceViewComponent.cpp
)

target_include_directories(ArmoniComposer PRIVATE
    Source
)

if (WIN32)
    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

target_compile_definitions(ArmoniComposer PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    NOMINMAX=1
    WIN32_LEAN_AND_MEAN=1
)

target_link_libraries(ArmoniComposer PRIVATE
    juce::juce_core
    juce::juce_events
    juce::juce_graphics
    juce::juce_data_structures
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_recommended_config_flags
)

if (WIN32)
    target_link_libraries(ArmoniComposer PRIVATE hid.lib setupapi.lib user32.lib wininet.lib)
endif()


# ===========================================================================
# Automated Unit Test Suite
# ===========================================================================
juce_add_console_app(ArmoniComposer_Tests
    PRODUCT_NAME "ArmoniComposer_Tests"
)

target_sources(ArmoniComposer_Tests PRIVATE
    Tests/TestRunner.cpp
    Source/DeviceManager.cpp
    Source/RawInputHandler.cpp
    Source/MappingEngine.cpp
    Source/AudioEngine.cpp
    Source/MetronomeClock.cpp
    Source/BpmQuantizer.cpp
    Source/ShiftLayerSystem.cpp
    Source/DspFxRack.cpp
)

target_include_directories(ArmoniComposer_Tests PRIVATE
    Source
)

target_compile_definitions(ArmoniComposer_Tests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    NOMINMAX=1
    WIN32_LEAN_AND_MEAN=1
)

target_link_libraries(ArmoniComposer_Tests PRIVATE
    juce::juce_core
    juce::juce_events
    juce::juce_graphics
    juce::juce_data_structures
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_recommended_config_flags
)

if (WIN32)
    target_link_libraries(ArmoniComposer_Tests PRIVATE hid.lib setupapi.lib user32.lib wininet.lib)
endif()

