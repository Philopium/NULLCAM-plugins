# Portable coverage gate for MemoryGranulatorMode.
#
# It checks that the public parameter API, the APVTS specifications, the
# visible editor controls and the pure DSP Settings structure stay in sync.
# It also proves that every Settings field is read by the real engine, so a
# visible control cannot silently become decorative.
#
#   cmake -DPLUGIN_DIR=<plugin-folder> -P tests/parameter_coverage_check.cmake

if(NOT DEFINED PLUGIN_DIR)
    message(FATAL_ERROR "PLUGIN_DIR was not supplied")
endif()

set(engine_header "${PLUGIN_DIR}/Source/MemoryGranulatorEngine.h")
set(engine_source "${PLUGIN_DIR}/Source/MemoryGranulatorEngine.cpp")
set(processor_header "${PLUGIN_DIR}/Source/PluginProcessor.h")
set(processor_source "${PLUGIN_DIR}/Source/PluginProcessor.cpp")
set(editor_source "${PLUGIN_DIR}/Source/PluginEditor.cpp")

foreach(required_file
        "${engine_header}"
        "${engine_source}"
        "${processor_header}"
        "${processor_source}"
        "${editor_source}")
    if(NOT EXISTS "${required_file}")
        message(FATAL_ERROR "Missing required file: ${required_file}")
    endif()
endforeach()

file(READ "${engine_header}" engine_header_text)
file(READ "${engine_source}" engine_source_text)
file(READ "${processor_header}" processor_header_text)
file(READ "${processor_source}" processor_source_text)
file(READ "${editor_source}" editor_source_text)

function(fail message)
    message(FATAL_ERROR "MemoryGranulatorMode parameter coverage: ${message}")
endfunction()

# ---------------------------------------------------------------------------
# Settings fields: 99 host parameters plus hostBpm supplied by the transport.
# ---------------------------------------------------------------------------
string(FIND "${engine_header_text}" "struct Settings" settings_start)
if(settings_start EQUAL -1)
    fail("the Settings structure was not found")
endif()

string(SUBSTRING "${engine_header_text}" ${settings_start} -1 settings_tail)
string(FIND "${settings_tail}" "\n};" settings_end)
if(settings_end EQUAL -1)
    fail("the end of the Settings structure was not found")
endif()

string(SUBSTRING "${settings_tail}" 0 ${settings_end} settings_block)
string(REGEX MATCHALL
       "(float|int|bool)[ \t\r\n]+[A-Za-z_][A-Za-z0-9_]*[ \t\r\n]*="
       settings_declarations
       "${settings_block}")

set(settings_fields "")
foreach(declaration IN LISTS settings_declarations)
    string(REGEX REPLACE
           "^(float|int|bool)[ \t\r\n]+([A-Za-z_][A-Za-z0-9_]*)[ \t\r\n]*=$"
           "\\2"
           field
           "${declaration}")
    list(APPEND settings_fields "${field}")
endforeach()

list(REMOVE_DUPLICATES settings_fields)
list(LENGTH settings_fields settings_field_count)

if(NOT settings_field_count EQUAL 100)
    fail("expected 100 Settings fields (99 parameters + hostBpm), found ${settings_field_count}")
endif()

foreach(field IN LISTS settings_fields)
    string(FIND "${engine_source_text}" "settings.${field}" field_position)
    if(field_position EQUAL -1)
        fail("settings.${field} is never read by MemoryGranulatorEngine.cpp")
    endif()
endforeach()

set(parameter_settings "${settings_fields}")
list(REMOVE_ITEM parameter_settings "hostBpm")
list(LENGTH parameter_settings parameter_settings_count)

if(NOT parameter_settings_count EQUAL 99)
    fail("expected 99 host-controlled Settings fields, found ${parameter_settings_count}")
endif()

# ---------------------------------------------------------------------------
# The parameter enum and its stable IDs.
# ---------------------------------------------------------------------------
string(FIND "${processor_header_text}" "enum class Index" enum_start)
if(enum_start EQUAL -1)
    fail("the parameter Index enum was not found")
endif()

string(SUBSTRING "${processor_header_text}" ${enum_start} -1 enum_tail)
string(FIND "${enum_tail}" "count" enum_end)
if(enum_end EQUAL -1)
    fail("the end of the parameter Index enum was not found")
endif()

string(SUBSTRING "${enum_tail}" 0 ${enum_end} enum_block)

string(FIND "${processor_header_text}"
       "inline constexpr std::array<const char*, param::kNumParameters> names = {"
       ids_start)
if(ids_start EQUAL -1)
    fail("the parameter ID array was not found")
endif()

string(SUBSTRING "${processor_header_text}" ${ids_start} -1 ids_tail)
string(FIND "${ids_tail}" "};" ids_end)
if(ids_end EQUAL -1)
    fail("the end of the parameter ID array was not found")
endif()

string(SUBSTRING "${ids_tail}" 0 ${ids_end} ids_block)
string(REGEX MATCHALL "\"[A-Za-z_][A-Za-z0-9_]*\"" quoted_ids "${ids_block}")

set(parameter_ids "")
foreach(quoted_id IN LISTS quoted_ids)
    string(REGEX REPLACE "^\"(.*)\"$" "\\1" parameter_id "${quoted_id}")
    list(APPEND parameter_ids "${parameter_id}")
endforeach()

list(REMOVE_DUPLICATES parameter_ids)
list(LENGTH parameter_ids parameter_id_count)

if(NOT parameter_id_count EQUAL 99)
    fail("expected 99 unique parameter IDs, found ${parameter_id_count}")
endif()

foreach(parameter_id IN LISTS parameter_ids)
    string(FIND "${enum_block}" "${parameter_id}" enum_position)
    if(enum_position EQUAL -1)
        fail("${parameter_id} has an ID but no Index enum member")
    endif()
endforeach()

# ---------------------------------------------------------------------------
# Every parameter has exactly one APVTS specification.
# ---------------------------------------------------------------------------
string(FIND "${processor_source_text}" "makeParameterSpecs()" specs_start)
if(specs_start EQUAL -1)
    fail("makeParameterSpecs() was not found")
endif()

string(SUBSTRING "${processor_source_text}" ${specs_start} -1 specs_tail)
string(FIND "${specs_tail}" "defaultValues" specs_end)
if(specs_end EQUAL -1)
    fail("the end of makeParameterSpecs() was not found")
endif()

string(SUBSTRING "${specs_tail}" 0 ${specs_end} specs_block)
string(REGEX MATCHALL "param::Index::[A-Za-z_][A-Za-z0-9_]*" spec_references
       "${specs_block}")

set(spec_ids "")
foreach(reference IN LISTS spec_references)
    string(REGEX REPLACE "^param::Index::" "" parameter_id "${reference}")
    list(APPEND spec_ids "${parameter_id}")
endforeach()

list(REMOVE_DUPLICATES spec_ids)
list(LENGTH spec_ids spec_id_count)

if(NOT spec_id_count EQUAL 99)
    fail("expected 99 APVTS parameter specifications, found ${spec_id_count}")
endif()

foreach(parameter_id IN LISTS parameter_ids)
    list(FIND spec_ids "${parameter_id}" spec_position)
    if(spec_position EQUAL -1)
        fail("${parameter_id} has no APVTS specification")
    endif()
endforeach()

# ---------------------------------------------------------------------------
# Every APVTS field is filled from the matching public parameter.
# ---------------------------------------------------------------------------
string(FIND "${processor_source_text}" "processBlock (juce::AudioBuffer<float>& buffer" process_start)
if(process_start EQUAL -1)
    fail("processBlock() was not found")
endif()

string(SUBSTRING "${processor_source_text}" ${process_start} -1 process_tail)
string(FIND "${process_tail}" "if (auto* playHead" process_end)
if(process_end EQUAL -1)
    fail("the parameter mapping section of processBlock() was not found")
endif()

string(SUBSTRING "${process_tail}" 0 ${process_end} process_block)
string(REGEX MATCHALL "settings\\.[A-Za-z_][A-Za-z0-9_]*[ \t\r\n]*=" mapped_settings
       "${process_block}")

set(mapped_fields "")
foreach(mapped_setting IN LISTS mapped_settings)
    string(REGEX REPLACE "^settings\\.([A-Za-z_][A-Za-z0-9_]*)[ \t\r\n]*=$"
           "\\1"
           field
           "${mapped_setting}")
    list(APPEND mapped_fields "${field}")
endforeach()

list(REMOVE_DUPLICATES mapped_fields)
list(LENGTH mapped_fields mapped_field_count)

if(NOT mapped_field_count EQUAL 99)
    fail("expected 99 Settings fields mapped from APVTS values, found ${mapped_field_count}")
endif()

foreach(field IN LISTS parameter_settings)
    list(FIND mapped_fields "${field}" mapped_position)
    if(mapped_position EQUAL -1)
        fail("${field} is not filled from its public parameter")
    endif()
endforeach()

# ---------------------------------------------------------------------------
# Every public parameter has one visible editor control: 76 knobs, 14
# parameter choices and 9 switches. The preset chooser is not a parameter.
# ---------------------------------------------------------------------------
string(FIND "${editor_source_text}" "// ---- memory and evolution ----" editor_start)
if(editor_start EQUAL -1)
    fail("the editor control section was not found")
endif()

string(SUBSTRING "${editor_source_text}" ${editor_start} -1 editor_tail)
string(FIND "${editor_tail}" "presetBox.setJustificationType" editor_end)
if(editor_end EQUAL -1)
    fail("the end of the editor control section was not found")
endif()

string(SUBSTRING "${editor_tail}" 0 ${editor_end} editor_block)
string(REGEX MATCHALL "ids::[A-Za-z_][A-Za-z0-9_]*" editor_references
       "${editor_block}")

set(editor_ids "")
foreach(reference IN LISTS editor_references)
    string(REGEX REPLACE "^ids::" "" parameter_id "${reference}")
    list(APPEND editor_ids "${parameter_id}")
endforeach()

list(REMOVE_DUPLICATES editor_ids)
list(LENGTH editor_ids editor_id_count)

if(NOT editor_id_count EQUAL 99)
    fail("expected 99 visible parameter controls, found ${editor_id_count}")
endif()

foreach(parameter_id IN LISTS parameter_ids)
    list(FIND editor_ids "${parameter_id}" editor_position)
    if(editor_position EQUAL -1)
        fail("${parameter_id} has no visible editor control")
    endif()
endforeach()

foreach(parameter_id IN LISTS editor_ids)
    list(FIND parameter_ids "${parameter_id}" parameter_position)
    if(parameter_position EQUAL -1)
        fail("${parameter_id} is visible but is not a public parameter")
    endif()
endforeach()

message(STATUS "MemoryGranulatorMode parameter coverage: PASS")
message(STATUS "  99 APVTS parameters = 99 visible controls = 99 Settings fields")
message(STATUS "  100 Settings fields read by the engine (including host BPM)")
