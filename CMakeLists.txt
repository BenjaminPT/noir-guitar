cmake_minimum_required(VERSION 3.22)
project(NoirGuitar VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")  # static CRT: no redistributable needed

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.12
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(NoirGuitar
    COMPANY_NAME "Noir Audio"
    PLUGIN_MANUFACTURER_CODE Noir
    PLUGIN_CODE Ngtr
    FORMATS VST3 Standalone
    PRODUCT_NAME "Noir Guitar"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    VST3_CATEGORIES Fx Distortion
    COPY_PLUGIN_AFTER_BUILD FALSE
    # Standalone: let the user pick any ASIO / WASAPI / DirectSound device, and open the settings button.
    STANDALONE_FOR_GUITAR TRUE)

target_sources(NoirGuitar PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/DawPanel.cpp
    Source/MidiPanel.cpp)

target_compile_definitions(NoirGuitar PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    JUCE_WASAPI=1
    JUCE_DIRECTSOUND=1
    JUCE_USE_WINRT_MIDI=0)

# ASIO: needs Steinberg's ASIO SDK (not bundled). The CI downloads it and passes -DASIO_SDK_DIR=...
set(ASIO_SDK_DIR "" CACHE PATH "Path to the unpacked Steinberg ASIO SDK")
if(ASIO_SDK_DIR AND EXISTS "${ASIO_SDK_DIR}/common/iasiodrv.h")
    message(STATUS "ASIO enabled: ${ASIO_SDK_DIR}")
    target_compile_definitions(NoirGuitar PUBLIC JUCE_ASIO=1)
    target_include_directories(NoirGuitar PRIVATE
        "${ASIO_SDK_DIR}/common" "${ASIO_SDK_DIR}/host" "${ASIO_SDK_DIR}/host/pc")
else()
    message(STATUS "ASIO disabled (no SDK found)")
    target_compile_definitions(NoirGuitar PUBLIC JUCE_ASIO=0)
endif()

target_link_libraries(NoirGuitar
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
