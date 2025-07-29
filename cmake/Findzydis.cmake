# Find module for zydis
# This module defines:
#  zydis_FOUND - system has zydis
#  Zydis::Zydis - imported target for zydis

# If zydis is already a target (being built in this project), use it
if(TARGET Zydis)
    set(zydis_FOUND TRUE)
    if(NOT TARGET Zydis::Zydis)
        add_library(Zydis::Zydis ALIAS Zydis)
    endif()
    
    # Also handle Zycore dependency
    if(TARGET Zycore AND NOT TARGET Zycore::Zycore)
        add_library(Zycore::Zycore ALIAS Zycore)
    endif()
else()
    # Otherwise, try to find installed zydis
    find_package(zydis CONFIG QUIET)
    if(zydis_FOUND)
        # Config file found, nothing else to do
    else()
        # Try to find it manually
        find_path(ZYDIS_INCLUDE_DIR 
            NAMES Zydis/Zydis.h
            PATHS ${CMAKE_PREFIX_PATH}
        )
        
        find_library(ZYDIS_LIBRARY
            NAMES Zydis zydis
            PATHS ${CMAKE_PREFIX_PATH}
        )
        
        find_path(ZYCORE_INCLUDE_DIR 
            NAMES Zycore/Zycore.h
            PATHS ${CMAKE_PREFIX_PATH}
        )
        
        find_library(ZYCORE_LIBRARY
            NAMES Zycore zycore
            PATHS ${CMAKE_PREFIX_PATH}
        )
        
        if(ZYDIS_INCLUDE_DIR AND ZYDIS_LIBRARY AND ZYCORE_INCLUDE_DIR AND ZYCORE_LIBRARY)
            set(zydis_FOUND TRUE)
            
            # Create imported targets
            if(NOT TARGET Zycore::Zycore)
                add_library(Zycore::Zycore UNKNOWN IMPORTED)
                set_target_properties(Zycore::Zycore PROPERTIES
                    IMPORTED_LOCATION "${ZYCORE_LIBRARY}"
                    INTERFACE_INCLUDE_DIRECTORIES "${ZYCORE_INCLUDE_DIR}"
                )
            endif()
            
            if(NOT TARGET Zydis::Zydis)
                add_library(Zydis::Zydis UNKNOWN IMPORTED)
                set_target_properties(Zydis::Zydis PROPERTIES
                    IMPORTED_LOCATION "${ZYDIS_LIBRARY}"
                    INTERFACE_INCLUDE_DIRECTORIES "${ZYDIS_INCLUDE_DIR}"
                    INTERFACE_LINK_LIBRARIES "Zycore::Zycore"
                )
            endif()
        endif()
    endif()
endif()

# Handle standard find_package arguments
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(zydis DEFAULT_MSG zydis_FOUND)