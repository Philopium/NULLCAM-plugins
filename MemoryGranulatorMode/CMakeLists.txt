cmake_minimum_required(VERSION 3.22)

project(MemoryGranulatorMode VERSION 1.5.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use an existing JUCE checkout when one is available:
#   cmake -B build -DJUCE_DIR=/path/to/JUCE
# Otherwise JUCE is cloned once.
set(JUCE_DIR "" CACHE PATH "Existing JUCE checkout to use instead of downloading one")

if(JUCE_DIR)
    add_subdirectory("${JUCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/JUCE")
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 9.0.5
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

set(MEMORY_GRANULATOR_MODE_VST3_DIR "" CACHE PATH
    "Override: folder receiving MemoryGranulatorMode.vst3 (empty = auto)")

if(MEMORY_GRANULATOR_MODE_VST3_DIR)
    set(_mgm_vst3_copy_dir "${MEMORY_GRANULATOR_MODE_VST3_DIR}")
elseif(WIN32)
    set(_vst3_system_dir "$ENV{ProgramW6432}/Common Files/VST3")
    set(_vst3_probe "${_vst3_system_dir}/.memory_granulator_mode_write_probe")
    execute_process(COMMAND "${CMAKE_COMMAND}" -E touch "${_vst3_probe}"
        RESULT_VARIABLE _vst3_can_write OUTPUT_QUIET ERROR_QUIET)

    if(_vst3_can_write EQUAL 0)
        file(REMOVE "${_vst3_probe}")
        set(_mgm_vst3_copy_dir "${_vst3_system_dir}")
    else()
        set(_mgm_vst3_copy_dir "$ENV{LOCALAPPDATA}/Programs/Common/VST3")
    endif()
elseif(APPLE)
    set(_mgm_vst3_copy_dir "$ENV{HOME}/Library/Audio/Plug-Ins/VST3")
endif()

message(STATUS "MemoryGranulatorMode.vst3 will be copied to: ${_mgm_vst3_copy_dir}")

juce_add_plugin(MemoryGranulatorMode
    COMPANY_NAME "NULLCAM"
    PLUGIN_MANUFACTURER_CODE Nlcm
    PLUGIN_CODE Mgmm
    PRODUCT_NAME "MemoryGranulatorMode"
    DESCRIPTION "Circular memory, granular clouds and genuine MIDI or audio pitch control"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    VST3_COPY_DIR "${_mgm_vst3_copy_dir}"
    FORMATS VST3 Standalone)

target_sources(MemoryGranulatorMode PRIVATE
    Source/MemoryGranulatorEngine.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_include_directories(MemoryGranulatorMode PRIVATE Source)

target_compile_definitions(MemoryGranulatorMode PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_VST3_EMULATE_MIDI_CC_WITH_PARAMETERS=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(MemoryGranulatorMode PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_formats
    juce::juce_gui_extra
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

# The pure engine, measured without JUCE or a host.
add_executable(memorygranulator_dsp_test
    tests/dsp_test.cpp
    Source/MemoryGranulatorEngine.cpp)
target_include_directories(memorygranulator_dsp_test PRIVATE Source)

# The faceplate and the preset/state API, rendered offscreen.
juce_add_console_app(memorygranulator_ui_check
    PRODUCT_NAME "memorygranulator_ui_check")
target_sources(memorygranulator_ui_check PRIVATE
    tests/ui_check.cpp
    Source/MemoryGranulatorEngine.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)
target_include_directories(memorygranulator_ui_check PRIVATE Source)
target_compile_definitions(memorygranulator_ui_check PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0)
target_link_libraries(memorygranulator_ui_check PRIVATE
    juce::juce_audio_utils
    juce::juce_gui_extra
    juce::juce_recommended_warning_flags)

# The real VST3 bundle, opened by JUCE's own headless VST3 host code.
juce_add_console_app(memorygranulator_vst3_check
    PRODUCT_NAME "memorygranulator_vst3_check")
target_sources(memorygranulator_vst3_check PRIVATE tests/vst3_check.cpp)
target_compile_definitions(memorygranulator_vst3_check PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_PLUGINHOST_VST3=1
    JUCE_STANDALONE_APPLICATION=1)
target_link_libraries(memorygranulator_vst3_check PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_recommended_warning_flags)

# A portable gate that keeps the public parameter API, the APVTS
# specifications, the visible editor controls and the pure DSP Settings
# structure aligned. It also proves that every Settings field is read by the
# engine, so a visible parameter cannot silently become decorative.
add_custom_target(memorygranulator_parameter_coverage_check
    COMMAND ${CMAKE_COMMAND}
        -DPLUGIN_DIR=${CMAKE_CURRENT_SOURCE_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/parameter_coverage_check.cmake
    VERBATIM)
