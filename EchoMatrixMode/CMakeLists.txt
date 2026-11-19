cmake_minimum_required(VERSION 3.22)

project(EchoMatrixMode VERSION 1.0.0 LANGUAGES C CXX)

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

# Where the built EchoMatrixMode.vst3 is copied after every build.
set(ECHO_MATRIX_MODE_VST3_DIR "" CACHE PATH "Override: folder receiving EchoMatrixMode.vst3 (empty = auto)")

if(ECHO_MATRIX_MODE_VST3_DIR)
    set(_em_vst3_copy_dir "${ECHO_MATRIX_MODE_VST3_DIR}")
elseif(WIN32)
    set(_vst3_system_dir "$ENV{ProgramW6432}/Common Files/VST3")
    set(_vst3_probe "${_vst3_system_dir}/.echo_matrix_mode_write_probe")
    execute_process(COMMAND "${CMAKE_COMMAND}" -E touch "${_vst3_probe}"
        RESULT_VARIABLE _vst3_can_write OUTPUT_QUIET ERROR_QUIET)

    if(_vst3_can_write EQUAL 0)
        file(REMOVE "${_vst3_probe}")
        set(_em_vst3_copy_dir "${_vst3_system_dir}")
    else()
        set(_em_vst3_copy_dir "$ENV{LOCALAPPDATA}/Programs/Common/VST3")
    endif()
elseif(APPLE)
    set(_em_vst3_copy_dir "$ENV{HOME}/Library/Audio/Plug-Ins/VST3")
endif()

message(STATUS "EchoMatrixMode.vst3 will be copied to: ${_em_vst3_copy_dir}")

juce_add_plugin(EchoMatrixMode
    COMPANY_NAME "NULLCAM"
    PLUGIN_MANUFACTURER_CODE Nlcm
    PLUGIN_CODE Echm
    PRODUCT_NAME "EchoMatrixMode"
    DESCRIPTION "Memoryless pattern delay: every repeat is an independently controllable audio event"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    VST3_COPY_DIR "${_em_vst3_copy_dir}"
    FORMATS VST3 Standalone)

target_sources(EchoMatrixMode PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_include_directories(EchoMatrixMode PRIVATE Source)

target_compile_definitions(EchoMatrixMode PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(EchoMatrixMode PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_formats
    juce::juce_gui_extra
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

# The DSP on its own, so the delay and sequencer can be measured without a host:
add_executable(echomatrix_dsp_test tests/dsp_test.cpp)
target_include_directories(echomatrix_dsp_test PRIVATE Source)

# The faceplate, rendered offscreen:
juce_add_console_app(echomatrix_ui_check PRODUCT_NAME "echomatrix_ui_check")
target_sources(echomatrix_ui_check PRIVATE
    tests/ui_check.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)
target_include_directories(echomatrix_ui_check PRIVATE Source)
target_compile_definitions(echomatrix_ui_check PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0)
target_link_libraries(echomatrix_ui_check PRIVATE
    juce::juce_audio_utils
    juce::juce_gui_extra
    juce::juce_recommended_warning_flags)

# The built VST3, opened by JUCE's own VST3 host code: the closest thing to
# "it loads in the DAW" that can run without a DAW.
juce_add_console_app(echomatrix_vst3_check PRODUCT_NAME "echomatrix_vst3_check")
target_sources(echomatrix_vst3_check PRIVATE tests/vst3_check.cpp)
target_compile_definitions(echomatrix_vst3_check PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_PLUGINHOST_VST3=1
    JUCE_STANDALONE_APPLICATION=1)
target_link_libraries(echomatrix_vst3_check PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_recommended_warning_flags)

