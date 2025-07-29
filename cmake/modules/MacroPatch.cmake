# MacroPatch.cmake - Utility for patching third-party headers to handle macro conflicts
#
# Provides functions to inject macro guards into third-party headers to prevent
# conflicts with project macros (like UE's 'check' macro)

# Patches a header file to temporarily undefine conflicting macros
#
# Arguments:
#   FILE_PATH - Path to the file to patch
#   MACRO_NAME - Name of the macro to guard (e.g., "check")
#   GUARD_PREFIX - Prefix for the guard macro (e.g., "FMT_UE")
#   INSERT_AFTER - Regex pattern to find where to insert the guard (e.g., "#define.*_H_|#pragma once")
#   RESTORE_BEFORE - Regex pattern to find where to restore (e.g., "\n#endif[^\n]*$")
#
# Example usage:
#   patch_macro_conflict(
#       FILE_PATH "${fmt_SOURCE_DIR}/include/fmt/ranges.h"
#       MACRO_NAME "check"
#       GUARD_PREFIX "FMT_UE"
#       INSERT_AFTER "#define FMT_RANGES_H_[^\n]*\n|#pragma once[^\n]*\n"
#       RESTORE_BEFORE "\n#endif[^\n]*\n?$"
#   )
#
function(patch_macro_conflict)
    set(options "")
    set(oneValueArgs FILE_PATH MACRO_NAME GUARD_PREFIX INSERT_AFTER RESTORE_BEFORE)
    set(multiValueArgs "")
    cmake_parse_arguments(PATCH "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    # Validate required arguments
    if(NOT PATCH_FILE_PATH)
        message(FATAL_ERROR "patch_macro_conflict: FILE_PATH is required")
    endif()
    if(NOT PATCH_MACRO_NAME)
        message(FATAL_ERROR "patch_macro_conflict: MACRO_NAME is required")
    endif()
    if(NOT PATCH_GUARD_PREFIX)
        message(FATAL_ERROR "patch_macro_conflict: GUARD_PREFIX is required")
    endif()
    
    # Set defaults for optional arguments
    if(NOT PATCH_INSERT_AFTER)
        set(PATCH_INSERT_AFTER "#define.*_H_[^\n]*\n|#pragma once[^\n]*\n")
    endif()
    if(NOT PATCH_RESTORE_BEFORE)
        set(PATCH_RESTORE_BEFORE "\n#endif[^\n]*\n?$")
    endif()
    
    # Read the file
    file(READ "${PATCH_FILE_PATH}" FILE_CONTENT)
    
    # Create the guard macro name
    set(GUARD_MACRO "${PATCH_GUARD_PREFIX}_HAD_${PATCH_MACRO_NAME}")
    string(TOUPPER "${GUARD_MACRO}" GUARD_MACRO)
    
    # Create the guard code
    set(MACRO_GUARD "
// Macro conflict guards for '${PATCH_MACRO_NAME}'
#ifdef ${PATCH_MACRO_NAME}
  #pragma push_macro(\"${PATCH_MACRO_NAME}\")
  #undef ${PATCH_MACRO_NAME}
  #define ${GUARD_MACRO}
#endif
")
    
    # Create the restore code
    set(MACRO_RESTORE "
// Restore '${PATCH_MACRO_NAME}' macro
#ifdef ${GUARD_MACRO}
  #pragma pop_macro(\"${PATCH_MACRO_NAME}\")
  #undef ${GUARD_MACRO}
#endif
")
    
    # Find where to insert the guard
    string(REGEX MATCH "${PATCH_INSERT_AFTER}" INSERT_MATCH "${FILE_CONTENT}")
    if(INSERT_MATCH)
        string(REPLACE "${INSERT_MATCH}" "${INSERT_MATCH}${MACRO_GUARD}" FILE_CONTENT "${FILE_CONTENT}")
        message(STATUS "Added macro guard for '${PATCH_MACRO_NAME}' after: ${INSERT_MATCH}")
    else()
        message(WARNING "Could not find insertion point '${PATCH_INSERT_AFTER}' in ${PATCH_FILE_PATH}")
        # Try to insert at the beginning as fallback
        set(FILE_CONTENT "${MACRO_GUARD}${FILE_CONTENT}")
    endif()
    
    # Find where to restore
    string(REGEX MATCH "${PATCH_RESTORE_BEFORE}" RESTORE_MATCH "${FILE_CONTENT}")
    if(RESTORE_MATCH)
        string(REPLACE "${RESTORE_MATCH}" "${MACRO_RESTORE}${RESTORE_MATCH}" FILE_CONTENT "${FILE_CONTENT}")
        message(STATUS "Added macro restore for '${PATCH_MACRO_NAME}' before: ${RESTORE_MATCH}")
    else()
        message(WARNING "Could not find restore point '${PATCH_RESTORE_BEFORE}' in ${PATCH_FILE_PATH}")
        # Append at the end as fallback
        set(FILE_CONTENT "${FILE_CONTENT}${MACRO_RESTORE}")
    endif()
    
    # Write the patched file
    file(WRITE "${PATCH_FILE_PATH}" "${FILE_CONTENT}")
endfunction()

# Convenience function to patch multiple macros in the same file
#
# Arguments:
#   FILE_PATH - Path to the file to patch
#   GUARD_PREFIX - Prefix for the guard macros
#   MACRO_NAMES - List of macro names to guard
#   INSERT_AFTER - (Optional) Regex pattern for insertion point
#   RESTORE_BEFORE - (Optional) Regex pattern for restore point
#
# Example usage:
#   patch_multiple_macro_conflicts(
#       FILE_PATH "${fmt_SOURCE_DIR}/include/fmt/ranges.h"
#       GUARD_PREFIX "FMT_UE"
#       MACRO_NAMES check CHECK verify
#   )
#
function(patch_multiple_macro_conflicts)
    set(options "")
    set(oneValueArgs FILE_PATH GUARD_PREFIX INSERT_AFTER RESTORE_BEFORE)
    set(multiValueArgs MACRO_NAMES)
    cmake_parse_arguments(PATCH "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    foreach(macro ${PATCH_MACRO_NAMES})
        patch_macro_conflict(
            FILE_PATH "${PATCH_FILE_PATH}"
            MACRO_NAME "${macro}"
            GUARD_PREFIX "${PATCH_GUARD_PREFIX}"
            INSERT_AFTER "${PATCH_INSERT_AFTER}"
            RESTORE_BEFORE "${PATCH_RESTORE_BEFORE}"
        )
    endforeach()
endfunction()