cmake_minimum_required(VERSION 3.22)

project(HarmonicResonatorMode VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE comes with the configure step. If you already have a JUCE checkout, point at
# it and skip the download:
#   cmake -B build -DJUCE_DIR=/path/to/JUCE
# Otherwise it is cloned once.
set(JUCE_DIR "" CACHE PATH "Existing JUCE checkout to use instead of downloading one")

if(JUCE_DIR)
    add_subdirectory("${JUCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/JUCE")
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG master
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

# Where the built HarmonicResonatorMode.vst3 is copied after every build.
set(HARMONIC_RESONATOR_MODE_VST3_DIR "" CACHE PATH "Override: folder receiving HarmonicResonatorMode.vst3 (empty = auto)")

if(HARMONIC_RESONATOR_MODE_VST3_DIR)
    set(_hr_vst3_copy_dir "${HARMONIC_RESONATOR_MODE_VST3_DIR}")
elseif(WIN32)
    set(_vst3_system_dir "$ENV{ProgramW6432}/Common Files/VST3")
    set(_vst3_probe "${_vst3_system_dir}/.harmonic_resonator_mode_write_probe")
    execute_process(COMMAND "${CMAKE_COMMAND}" -E touch "${_vst3_probe}"
        RESULT_VARIABLE _vst3_can_write OUTPUT_QUIET ERROR_QUIET)

    if(_vst3_can_write EQUAL 0)
        file(REMOVE "${_vst3_probe}")
        set(_hr_vst3_copy_dir "${_vst3_system_dir}")
    else()
        set(_hr_vst3_copy_dir "$ENV{LOCALAPPDATA}/Programs/Common/VST3")
    endif()
elseif(APPLE)
    set(_hr_vst3_copy_dir "$ENV{HOME}/Library/Audio/Plug-Ins/VST3")
endif()

message(STATUS "HarmonicResonatorMode.vst3 will be copied to: ${_hr_vst3_copy_dir}")

juce_add_plugin(HarmonicResonatorMode
    COMPANY_NAME "NULLCAM"
    PLUGIN_MANUFACTURER_CODE Nlcm
    PLUGIN_CODE Hrsm
    PRODUCT_NAME "HarmonicResonatorMode"
    DESCRIPTION "Interactive distortion and resonance: the nonlinear stage and the partial bank drive each other"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    VST3_COPY_DIR "${_hr_vst3_copy_dir}"
    FORMATS VST3 Standalone)

target_sources(HarmonicResonatorMode PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_include_directories(HarmonicResonatorMode PRIVATE Source)

target_compile_definitions(HarmonicResonatorMode PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(HarmonicResonatorMode PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_formats
    juce::juce_gui_extra
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

# The DSP on its own, so the shaper, the bank and the loop can be measured
# without a host:
add_executable(harmonicresonator_dsp_test tests/dsp_test.cpp)
target_include_directories(harmonicresonator_dsp_test PRIVATE Source)

# The faceplate, rendered offscreen:
juce_add_console_app(harmonicresonator_ui_check PRODUCT_NAME "harmonicresonator_ui_check")
target_sources(harmonicresonator_ui_check PRIVATE
    tests/ui_check.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)
target_include_directories(harmonicresonator_ui_check PRIVATE Source)
target_compile_definitions(harmonicresonator_ui_check PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0)
target_link_libraries(harmonicresonator_ui_check PRIVATE
    juce::juce_audio_utils
    juce::juce_gui_extra
    juce::juce_recommended_warning_flags)

# The built VST3, opened by JUCE's own VST3 host code: the closest thing to
# "it loads in the DAW" that can run without a DAW.
juce_add_console_app(harmonicresonator_vst3_check PRODUCT_NAME "harmonicresonator_vst3_check")
target_sources(harmonicresonator_vst3_check PRIVATE tests/vst3_check.cpp)
target_compile_definitions(harmonicresonator_vst3_check PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_PLUGINHOST_VST3=1
    JUCE_STANDALONE_APPLICATION=1)
target_link_libraries(harmonicresonator_vst3_check PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_recommended_warning_flags)
