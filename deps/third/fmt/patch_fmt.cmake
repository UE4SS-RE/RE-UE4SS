# Script to patch fmt for UE macro conflicts
# This is run as a PATCH_COMMAND during FetchContent
# The script runs from the fmt source directory after it's fetched

# Use the script directory passed from parent to find the modules
get_filename_component(PROJECT_ROOT "${PATCH_SCRIPT_DIR}/../../.." ABSOLUTE)
include(${PROJECT_ROOT}/cmake/modules/MacroPatch.cmake)

# Uses patch_macro_conflict() from cmake/modules/MacroPatch.cmake
patch_macro_conflict(
    FILE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/include/fmt/ranges.h"
    MACRO_NAME "check"
    GUARD_PREFIX "FMT_UE"
    INSERT_AFTER "#define FMT_RANGES_H_[^\n]*\n|#pragma once[^\n]*\n"
    RESTORE_BEFORE "\n#endif[^\n]*\n?$"
)