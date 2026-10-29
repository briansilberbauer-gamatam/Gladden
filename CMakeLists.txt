cmake_minimum_required(VERSION 3.22)

project(SynthwaveLegacy
    VERSION 1.0.0
    DESCRIPTION "Synthwave Legacy - 16-voice virtual analog synthesizer"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# The project is pinned to JUCE 7.0.12. Point SYNTHWAVE_JUCE_DIR at an existing
# JUCE checkout for offline builds; otherwise the exact release is fetched.
set(SYNTHWAVE_JUCE_DIR "" CACHE PATH "Path to a local JUCE 7 checkout")
if(SYNTHWAVE_JUCE_DIR AND EXISTS "${SYNTHWAVE_JUCE_DIR}/CMakeLists.txt")
    message(STATUS "Using local JUCE at ${SYNTHWAVE_JUCE_DIR}")
    add_subdirectory("${SYNTHWAVE_JUCE_DIR}" JUCE EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        7.0.12
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

set(SYNTHWAVE_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND SYNTHWAVE_FORMATS AU)
endif()

juce_add_plugin(SynthwaveLegacy
    COMPANY_NAME              "Gladden Audio"
    BUNDLE_ID                 "com.gladdenaudio.synthwavelegacy"
    PLUGIN_MANUFACTURER_CODE  Glad
    PLUGIN_CODE               Swl1
    PRODUCT_NAME              "Synthwave Legacy"
    DESCRIPTION               "16-voice virtual analog synthesizer"
    FORMATS                   ${SYNTHWAVE_FORMATS}
    IS_SYNTH                  TRUE
    NEEDS_MIDI_INPUT          TRUE
    NEEDS_MIDI_OUTPUT         FALSE
    IS_MIDI_EFFECT            FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD   FALSE
    VST3_CATEGORIES           "Instrument Synth"
    AU_MAIN_TYPE              kAudioUnitType_MusicDevice
    VST3_COPY_DIR             "${CMAKE_BINARY_DIR}/vst3"
    AU_COPY_DIR               "${CMAKE_BINARY_DIR}/au"
    STANDALONE_PLUGIN_COPY_DIR "${CMAKE_BINARY_DIR}/standalone")

target_sources(SynthwaveLegacy
    PRIVATE
        Source/PluginProcessor.cpp
        Source/SynthVoice.cpp
        Source/DSP.cpp
        Source/Parameters.cpp
        Source/PluginEditor.cpp
        Source/LookAndFeel.cpp
        Source/UIComponents.cpp
        Source/Presets.cpp)

target_include_directories(SynthwaveLegacy PRIVATE Source)

target_compile_definitions(SynthwaveLegacy
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
        JUCE_MODAL_LOOPS_PERMITTED=1
    PRIVATE
        SYNTHWAVE_VERSION="${PROJECT_VERSION}")

target_link_libraries(SynthwaveLegacy
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

# Recommended warning flags are useful but this codebase deliberately opts out
# of a few legacy-warning categories that are noisy under JUCE 7 + newer GCC.
target_compile_options(SynthwaveLegacy PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wno-unused-parameter -Wno-float-equal>)

option(SYNTHWAVE_BUILD_TESTS "Build the offline DSP/state smoke test" ON)
if(SYNTHWAVE_BUILD_TESTS)
    juce_add_console_app(SynthwaveLegacyTests
        PRODUCT_NAME "SynthwaveLegacyTests")
    target_sources(SynthwaveLegacyTests PRIVATE
        Tests/SmokeTest.cpp
        Source/PluginProcessor.cpp
        Source/SynthVoice.cpp
        Source/DSP.cpp
        Source/Parameters.cpp
        Source/Presets.cpp
        Source/PluginEditor.cpp
        Source/LookAndFeel.cpp
        Source/UIComponents.cpp)
    target_include_directories(SynthwaveLegacyTests PRIVATE Source)
    target_compile_definitions(SynthwaveLegacyTests PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_MODAL_LOOPS_PERMITTED=1
        SYNTHWAVE_VERSION="${PROJECT_VERSION}")
    target_link_libraries(SynthwaveLegacyTests PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
        juce::juce_recommended_config_flags)

    enable_testing()
    add_test(NAME SynthwaveLegacyDsp COMMAND SynthwaveLegacyTests)
endif()
