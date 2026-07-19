foreach(required_variable
        UE4SS_LIBRARY
        LAUNCHER_SOURCE
        PROBE_EXECUTABLE
        EXISTING_PRELOAD
        SETTINGS_TEMPLATE
        STAGE_DIRECTORY)
    if(NOT DEFINED ${required_variable})
        message(FATAL_ERROR "Missing required variable: ${required_variable}")
    endif()
endforeach()

file(REMOVE_RECURSE "${STAGE_DIRECTORY}")
file(MAKE_DIRECTORY "${STAGE_DIRECTORY}/UE4SS_Signatures")
file(COPY "${UE4SS_LIBRARY}" "${LAUNCHER_SOURCE}" "${EXISTING_PRELOAD}" DESTINATION "${STAGE_DIRECTORY}")
file(CHMOD "${STAGE_DIRECTORY}/run_ue4ss.sh"
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

get_filename_component(existing_preload_name "${EXISTING_PRELOAD}" NAME)
set(staged_existing_preload "${STAGE_DIRECTORY}/${existing_preload_name}")

file(READ "${SETTINGS_TEMPLATE}" settings_contents)
string(REPLACE
    "SecondsToScanBeforeGivingUp = 30"
    "SecondsToScanBeforeGivingUp = 0"
    settings_contents
    "${settings_contents}"
)
string(REPLACE "ConsoleEnabled = 1" "ConsoleEnabled = 0" settings_contents "${settings_contents}")
file(WRITE "${STAGE_DIRECTORY}/UE4SS-settings.ini" "${settings_contents}")
file(WRITE "${STAGE_DIRECTORY}/UE4SS_Signatures/GUObjectArray.lua" [=[
function Register()
    return "DE AD BE EF FE ED FA CE 01 23 45 67 89 AB CD EF"
end

function OnMatchFound(matchAddress)
    return matchAddress
end
]=])

set(staged_ue4ss "${STAGE_DIRECTORY}/libUE4SS.so")
set(ue4ss_log "${STAGE_DIRECTORY}/UE4SS.log")
file(REMOVE "${ue4ss_log}")
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env
        --unset=UE4SS_LAUNCH_TARGET_EXE
        --unset=UE4SS_LAUNCH_LD_PRELOAD_WAS_SET
        --unset=UE4SS_LAUNCH_ORIGINAL_LD_PRELOAD
        --unset=UE4SS_MODULE_PATH
        "LD_PRELOAD=${staged_ue4ss}"
        "BOX64_LD_PRELOAD=${staged_ue4ss}"
        "UE4SS_DIAGNOSE=1"
        "${PROBE_EXECUTABLE}" --box64-orphan "${staged_ue4ss}"
    RESULT_VARIABLE orphan_result
    OUTPUT_VARIABLE orphan_stdout
    ERROR_VARIABLE orphan_stderr
    TIMEOUT 4
)
if(NOT orphan_result EQUAL 0)
    message(FATAL_ERROR "Box64 orphan probe exited with ${orphan_result}:\n${orphan_stdout}\n${orphan_stderr}")
endif()
if(NOT orphan_stdout MATCHES "UE4SS_BOX64_ORPHAN_PROBE_CLEAN")
    message(FATAL_ERROR "Box64 orphan probe did not reach its clean marker:\n${orphan_stdout}\n${orphan_stderr}")
endif()
if(NOT orphan_stderr MATCHES "DIAG: startup_skipped executable=.* reason=box64_target_missing")
    message(FATAL_ERROR "Box64 orphan preload did not fail closed:\n${orphan_stdout}\n${orphan_stderr}")
endif()
if(EXISTS "${ue4ss_log}")
    message(FATAL_ERROR "Box64 orphan preload created UE4SS.log")
endif()

file(WRITE "${STAGE_DIRECTORY}/wrapper.sh"
    "#!/bin/bash\nset -euo pipefail\nexec \"${PROBE_EXECUTABLE}\" \"$@\"\n")
file(CHMOD "${STAGE_DIRECTORY}/wrapper.sh"
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env
        "LD_PRELOAD=${staged_existing_preload}"
        "UE4SS_DIAGNOSE=1"
        "${STAGE_DIRECTORY}/run_ue4ss.sh"
        --host-executable "${PROBE_EXECUTABLE}"
        "${STAGE_DIRECTORY}/wrapper.sh"
        "${STAGE_DIRECTORY}/libUE4SS.so"
        "${staged_existing_preload}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
    TIMEOUT 8
)

set(process_output "${stdout}\n${stderr}")
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Process-scope probe exited with ${result}:\n${process_output}")
endif()
foreach(expected_line
        "HOST_LD_PRELOAD=${staged_existing_preload}"
        "HOST_TARGET=<unset>"
        "HOST_WAS_SET=<unset>"
        "HOST_ORIGINAL=<unset>"
        "HOST_MODULE=<unset>"
        "HELPER_LD_PRELOAD=${staged_existing_preload}"
        "HELPER_TARGET=<unset>"
        "HELPER_WAS_SET=<unset>"
        "HELPER_ORIGINAL=<unset>"
        "HELPER_MODULE=<unset>"
        "UE4SS_PRELOAD_SCOPE_HELPER_CLEAN"
        "UE4SS_PRELOAD_SCOPE_HOST_READY")
    string(FIND "${process_output}" "${expected_line}" expected_position)
    if(expected_position EQUAL -1)
        message(FATAL_ERROR "Process-scope output did not contain '${expected_line}':\n${process_output}")
    endif()
endforeach()
if(NOT stderr MATCHES "DIAG: startup_skipped executable=.* expected=.* reason=target_mismatch")
    message(FATAL_ERROR "Wrapper mismatch did not emit startup_skipped:\n${stderr}")
endif()
if(process_output MATCHES "DIAG: inactive_reason=.*target_mismatch")
    message(FATAL_ERROR "Normal wrapper mismatch was reported as inactive:\n${process_output}")
endif()

if(NOT EXISTS "${ue4ss_log}")
    message(FATAL_ERROR "Accepted host did not create UE4SS.log")
endif()
file(READ "${ue4ss_log}" ue4ss_log_contents)
get_filename_component(probe_canonical "${PROBE_EXECUTABLE}" REALPATH)
string(REGEX MATCHALL "DIAG: executable=[^\r\n]*" executable_diagnostics "${ue4ss_log_contents}")
list(LENGTH executable_diagnostics executable_diagnostic_count)
if(NOT executable_diagnostic_count EQUAL 1)
    message(FATAL_ERROR "Expected one executable diagnostic, found ${executable_diagnostic_count}:\n${ue4ss_log_contents}")
endif()
list(GET executable_diagnostics 0 executable_diagnostic)
if(NOT executable_diagnostic STREQUAL "DIAG: executable=${probe_canonical}")
    message(FATAL_ERROR "Unexpected executable diagnostic '${executable_diagnostic}'")
endif()
if(ue4ss_log_contents MATCHES "crashpad_handler")
    message(FATAL_ERROR "Helper diagnostics leaked into UE4SS.log:\n${ue4ss_log_contents}")
endif()

file(REMOVE_RECURSE "${STAGE_DIRECTORY}")
