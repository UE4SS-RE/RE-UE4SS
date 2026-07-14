foreach(required_variable UE4SS_LIBRARY PROBE_EXECUTABLE SETTINGS_TEMPLATE STAGE_DIRECTORY)
    if(NOT DEFINED ${required_variable})
        message(FATAL_ERROR "Missing required variable: ${required_variable}")
    endif()
endforeach()

file(REMOVE_RECURSE "${STAGE_DIRECTORY}")
file(MAKE_DIRECTORY "${STAGE_DIRECTORY}/UE4SS_Signatures")

get_filename_component(ue4ss_library_name "${UE4SS_LIBRARY}" NAME)
file(COPY "${UE4SS_LIBRARY}" DESTINATION "${STAGE_DIRECTORY}")

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

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env
        "LD_PRELOAD=${STAGE_DIRECTORY}/${ue4ss_library_name}"
        "${PROBE_EXECUTABLE}"
    RESULT_VARIABLE probe_result
    OUTPUT_VARIABLE probe_stdout
    ERROR_VARIABLE probe_stderr
    TIMEOUT 5
)

set(probe_output "${probe_stdout}\n${probe_stderr}")
if(NOT probe_result EQUAL 0)
    message(FATAL_ERROR "Fail-soft probe exited with ${probe_result}:\n${probe_output}")
endif()
if(NOT probe_output MATCHES "UE4SS_FAIL_SOFT_PROBE_READY")
    message(FATAL_ERROR "Fail-soft probe did not reach its ready marker:\n${probe_output}")
endif()

set(ue4ss_log "${STAGE_DIRECTORY}/UE4SS.log")
if(NOT EXISTS "${ue4ss_log}")
    message(FATAL_ERROR "Fail-soft probe did not create UE4SS.log")
endif()
file(READ "${ue4ss_log}" ue4ss_log_contents)
if(NOT ue4ss_log_contents MATCHES "PS scan timed out")
    message(FATAL_ERROR "UE4SS.log did not record the expected signature failure")
endif()
file(SIZE "${ue4ss_log}" ue4ss_log_size)
if(ue4ss_log_size GREATER 1048576)
    message(FATAL_ERROR "Fail-soft signature scanning produced an excessive ${ue4ss_log_size}-byte log")
endif()

file(REMOVE_RECURSE "${STAGE_DIRECTORY}")
