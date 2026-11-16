cmake_minimum_required(VERSION 3.22)

project(MauriMode VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE comes with the configure step. If you already have a JUCE checkout, point at
# it and skip the download:
#   cmake -B build -DJUCE_DIR=/path/to/JUCE
# Otherwise it is cloned once (pin GIT_TAG to a release instead of master if you like).
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

# Where the built MAURI-MODE.vst3 is copied after every build.
#   empty  -> chosen on every configure: the system VST3 folder when this shell can
#             write to it (build launched as Administrator), otherwise a per-user
#             folder. Copying into "C:\Program Files\Common Files\VST3" without
#             permission is what made the copy step fail with MSB3073 and abort
#             an otherwise successful build.
#   force  -> cmake -B build -DMAURI_MODE_VST3_DIR="C:/Program Files/Common Files/VST3"
set(MAURI_MODE_VST3_DIR "" CACHE PATH "Override: folder receiving MAURI-MODE.vst3 (empty = auto)")

if(MAURI_MODE_VST3_DIR)
    set(_mauri_vst3_copy_dir "${MAURI_MODE_VST3_DIR}")
elseif(WIN32)
    set(_vst3_system_dir "$ENV{ProgramW6432}/Common Files/VST3")
    set(_vst3_probe "${_vst3_system_dir}/.mauri_mode_write_probe")
    execute_process(COMMAND "${CMAKE_COMMAND}" -E touch "${_vst3_probe}"
        RESULT_VARIABLE _vst3_can_write OUTPUT_QUIET ERROR_QUIET)

    if(_vst3_can_write EQUAL 0)
        file(REMOVE "${_vst3_probe}")
        set(_mauri_vst3_copy_dir "${_vst3_system_dir}")
    else()
        set(_mauri_vst3_copy_dir "$ENV{LOCALAPPDATA}/Programs/Common/VST3")
    endif()
elseif(APPLE)
    set(_mauri_vst3_copy_dir "$ENV{HOME}/Library/Audio/Plug-Ins/VST3")
endif()

message(STATUS "MAURI-MODE.vst3 will be copied to: ${_mauri_vst3_copy_dir}")

juce_add_plugin(MauriMode
    COMPANY_NAME "NULLCAM"
    PLUGIN_MANUFACTURER_CODE Nlcm
    PLUGIN_CODE Mmde
    PRODUCT_NAME "MAURI-MODE"
    DESCRIPTION "Rhythm slicer: the input becomes a sequence, timed by the playing"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    VST3_COPY_DIR "${_mauri_vst3_copy_dir}"
    # AU is macOS only; uncomment it there if you also want the Audio Unit build
    # FORMATS VST3 AU Standalone)
    FORMATS VST3 Standalone)

target_sources(MauriMode PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_include_directories(MauriMode PRIVATE Source)

# The faceplate is typographic, so the fonts travel inside the binary instead of
# hoping the machine has them. Bungee / Bungee Shade are SIL OFL, see Source/Fonts.
juce_add_binary_data(mauri_binarydata
    NAMESPACE MauriAssets
    HEADER_NAME MauriAssets.h
    SOURCES
        Source/Fonts/Bungee-Regular.ttf
        Source/Fonts/BungeeShade-Regular.ttf)

target_compile_definitions(MauriMode PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(MauriMode PRIVATE
    juce::juce_audio_utils
    juce::juce_gui_extra
    mauri_binarydata
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

# The DSP on its own, so the slicer can be measured without a host:
#   cmake --build . --target mauri_dsp_test && ./mauri_dsp_test
add_executable(mauri_dsp_test tests/dsp_test.cpp)
target_include_directories(mauri_dsp_test PRIVATE Source)

# The faceplate, rendered offscreen, so the UI can be looked at without a DAW:
#   cmake --build . --target mauri_ui_check
#   ./mauri_ui_check panel.png
# Also checks that a user preset survives a save / change / load round trip.
juce_add_console_app(mauri_ui_check PRODUCT_NAME "mauri_ui_check")
target_sources(mauri_ui_check PRIVATE
    tests/ui_check.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)
target_include_directories(mauri_ui_check PRIVATE Source)
target_compile_definitions(mauri_ui_check PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0)
target_link_libraries(mauri_ui_check PRIVATE
    juce::juce_audio_utils
    juce::juce_gui_extra
    mauri_binarydata
    juce::juce_recommended_warning_flags)
