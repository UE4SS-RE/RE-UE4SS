# This module can be used to find the Superluminal API libs & headers via find_package.
#
# For example: find_package(SuperluminalAPI REQUIRED)
#
# You can use it by adding the API directory of your Superluminal install to your CMAKE_PREFIX_PATH, or alternatively
# by copying the entire API directory to a place of your own choosing and adding that location to CMAKE_PREFIX_PATH.
#
# The following (optional) variables can be set prior to issueing the find_package command:
# - SuperluminalAPI_ROOT				: 	The root directory where the libs & headers should be found. For example <SuperluminalInstallDir>\API.
#						  					If this is not set, the libs & headers are assumed to be next to the location of FindSuperluminalAPI.cmake
# - SuperluminalAPI_USE_STATIC_RUNTIME	:	If this is set, the libraries linked to the static C runtime (i.e. /MT and /MTd) will be returned
#											If not set, the libraries linked to the dynamic C runtime (i.e. /MD and /MDd) will be returned
# 
# On completion of find_package, the following variables will be set:
#
# SuperluminalAPI_FOUND			: Whether the package was found
# SuperluminalAPI_LIBS_RELEASE	: The Release libraries to link against
# SuperluminalAPI_LIBS_DEBUG	: The Debug libraries to link against
# SuperluminalAPI_INCLUDE_DIRS	: The include directories to use
#
# In addition, if find_package completed successfully, the target "SuperluminalAPI" will be defined. 
# You should prefer to consume this target via target_link_libraries(YOUR_TARGET PRIVATE SuperluminalAPI), rather than by using the above variables directly
SET(SuperluminalAPI_SEARCH_PATHS
	${CMAKE_CURRENT_LIST_DIR}
	${SuperluminalAPI_ROOT}
    )

find_path(SuperluminalAPI_INCLUDE_DIRS Superluminal/PerformanceAPI.h
          PATHS ${SuperluminalAPI_SEARCH_PATHS}
		  PATH_SUFFIXES include
          )

if (CMAKE_SIZEOF_VOID_P MATCHES 4)
	set(SELECTED_ARCH "x86")
else(CMAKE_SIZEOF_VOID_P MATCHES 8)
	set(SELECTED_ARCH "x64")
endif()

if(WIN32)
	if (NOT (${MSVC_VERSION} LESS 1900))	# Test for VS2015 and higher (older CMake versions don't have a >= operator)
		if (${SuperluminalAPI_USE_STATIC_RUNTIME})
			find_library(SuperluminalAPI_LIBS_RELEASE
						NAMES PerformanceAPI_MT
						PATH_SUFFIXES lib/${SELECTED_ARCH}
						PATHS ${SuperluminalAPI_SEARCH_PATHS}
						)
						
			find_library(SuperluminalAPI_LIBS_DEBUG
						NAMES PerformanceAPI_MTd
						PATH_SUFFIXES lib/${SELECTED_ARCH}
						PATHS ${SuperluminalAPI_SEARCH_PATHS}
						)
		else()
			find_library(SuperluminalAPI_LIBS_RELEASE
						NAMES PerformanceAPI_MD
						PATH_SUFFIXES lib/${SELECTED_ARCH}
						PATHS ${SuperluminalAPI_SEARCH_PATHS}
						)
						
			find_library(SuperluminalAPI_LIBS_DEBUG
						NAMES PerformanceAPI_MDd
						PATH_SUFFIXES lib/${SELECTED_ARCH}
						PATHS ${SuperluminalAPI_SEARCH_PATHS}
						)
		endif()
	else()
		message(SEND_ERROR "Your Visual Studio version is not currently supported. Please contact Superluminal support.")
	endif()
endif()

mark_as_advanced(SuperluminalAPI_FOUND)
mark_as_advanced(SuperluminalAPI_LIBS_RELEASE)
mark_as_advanced(SuperluminalAPI_LIBS_DEBUG)
mark_as_advanced(SuperluminalAPI_INCLUDE_DIRS)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SuperluminalAPI REQUIRED_VARS SuperluminalAPI_INCLUDE_DIRS SuperluminalAPI_LIBS_RELEASE SuperluminalAPI_LIBS_DEBUG)

if(SuperluminalAPI_FOUND AND NOT TARGET SuperluminalAPI)
    add_library(SuperluminalAPI INTERFACE IMPORTED)
    target_include_directories(SuperluminalAPI INTERFACE "${SuperluminalAPI_INCLUDE_DIRS}")
    target_link_libraries(SuperluminalAPI INTERFACE debug "${SuperluminalAPI_LIBS_DEBUG}" optimized "${SuperluminalAPI_LIBS_RELEASE}" )
endif()