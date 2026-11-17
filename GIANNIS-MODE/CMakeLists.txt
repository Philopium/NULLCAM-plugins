cmake_minimum_required(VERSION 3.22)

project(GiannisMode VERSION 1.0.0 LANGUAGES C CXX)

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

# Where the built GIANNIS-MODE.vst3 is copied after every build.
#   empty  -> chosen on every configure: the system VST3 folder when this shell can
#             write to it, otherwise a per-user folder.
#   force  -> cmake -B build -DGIANNIS_MODE_VST3_DIR="C:/Program Files/Common Files/VST3"
set(GIANNIS_MODE_VST3_DIR "" CACHE PATH "Override: folder receiving GIANNIS-MODE.vst3 (empty = auto)")

if(GIANNIS_MODE_VST3_DIR)
    set(_giannis_vst3_copy_dir "${GIANNIS_MODE_VST3_DIR}")
elseif(WIN32)
    set(_vst3_system_dir "$ENV{ProgramW6432}/Common Files/VST3")
    set(_vst3_probe "${_vst3_system_dir}/.giannis_mode_write_probe")
    execute_process(COMMAND "${CMAKE_COMMAND}" -E touch "${_vst3_probe}"
        RESULT_VARIABLE _vst3_can_write OUTPUT_QUIET ERROR_QUIET)

    if(_vst3_can_write EQUAL 0)
        file(REMOVE "${_vst3_probe}")
        set(_giannis_vst3_copy_dir "${_vst3_system_dir}")
    else()
        set(_giannis_vst3_copy_dir "$ENV{LOCALAPPDATA}/Programs/Common/VST3")
    endif()
elseif(APPLE)
    set(_giannis_vst3_copy_dir "$ENV{HOME}/Library/Audio/Plug-Ins/VST3")
endif()

message(STATUS "GIANNIS-MODE.vst3 will be copied to: ${_giannis_vst3_copy_dir}")

juce_add_plugin(GiannisMode
    COMPANY_NAME "NULLCAM"
    PLUGIN_MANUFACTURER_CODE Nlcm
    PLUGIN_CODE Gmde
    PRODUCT_NAME "GIANNIS-MODE"
    DESCRIPTION "Octaver and sub octaver: analog style divider, sine subs, fuzz"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    VST3_COPY_DIR "${_giannis_vst3_copy_dir}"
    FORMATS VST3 Standalone)

target_sources(GiannisMode PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_include_directories(GiannisMode PRIVATE Source)

# The faces travel inside the binary (all SIL OFL, see Source/Fonts).
juce_add_binary_data(giannis_binarydata
    NAMESPACE GiannisAssets
    HEADER_NAME GiannisAssets.h
    SOURCES
        Source/Fonts/BagelFatOne-Regular.ttf
        Source/Fonts/Rowdies-Bold.ttf
        Source/Fonts/Rowdies-Regular.ttf
        Source/Fonts/TitanOne-Regular.ttf)

target_compile_definitions(GiannisMode PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(GiannisMode PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_formats
    juce::juce_gui_extra
    giannis_binarydata
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

# The DSP on its own, so the divider and the subs can be measured without a host:
#   cmake --build . --target giannis_dsp_test && ./giannis_dsp_test
add_executable(giannis_dsp_test tests/dsp_test.cpp)
target_include_directories(giannis_dsp_test PRIVATE Source)

# The faceplate, rendered offscreen, so the UI can be looked at without a DAW:
#   cmake --build . --target giannis_ui_check
#   ./giannis_ui_check panel.png
juce_add_console_app(giannis_ui_check PRODUCT_NAME "giannis_ui_check")
target_sources(giannis_ui_check PRIVATE
    tests/ui_check.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)
target_include_directories(giannis_ui_check PRIVATE Source)
target_compile_definitions(giannis_ui_check PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0)
target_link_libraries(giannis_ui_check PRIVATE
    juce::juce_audio_utils
    juce::juce_gui_extra
    giannis_binarydata
    juce::juce_recommended_warning_flags)
