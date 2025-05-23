# IDE-specific fixes and workarounds module
# This module provides fixes for various IDE quirks and limitations

# Apply CLion-specific fixes
#
# CLion requires certain API macros to be defined for proper code analysis.
# This function adds empty definitions for these macros when running under CLion.
#
# Arguments:
#   None (uses TARGET_COMPILE_DEFINITIONS from parent scope)
#
# Example usage:
#   apply_clion_fixes()
#
# Note: Only applies fixes if CLION_IDE environment variable is set
#
function(apply_clion_fixes)
    if(DEFINED ENV{CLION_IDE})
        # Get current definitions from parent scope
        set(CLION_DEFINITIONS ${TARGET_COMPILE_DEFINITIONS})
        
        # Add CLion-specific API macro definitions
        list(APPEND CLION_DEFINITIONS
            RC_UE4SS_API=
            RC_UE_API=
            RC_ASM_API=
            RC_DYNOUT_API=
            RC_FILE_API=
            RC_FNCTMR_API=
            RC_INI_PARSER_API=
            RC_INPUT_API=
            RC_JSON_API=
            RC_LMS_API=
            RC_PB_API=
            RC_SPSS_API=
        )
        
        # Update parent scope
        set(TARGET_COMPILE_DEFINITIONS ${CLION_DEFINITIONS} PARENT_SCOPE)
        
        message(STATUS "Applied CLion-specific fixes")
    endif()
endfunction()

# Apply general IDE fixes
#
# This is a wrapper function that applies all IDE-specific fixes.
# Currently only includes CLion fixes, but can be extended for other IDEs.
#
# Arguments:
#   None
#
# Example usage:
#   apply_ide_fixes()
#
function(apply_ide_fixes)
    apply_clion_fixes()
    # Add other IDE fixes here as needed
endfunction()
