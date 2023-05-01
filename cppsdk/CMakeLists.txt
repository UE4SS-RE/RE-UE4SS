cmake_minimum_required(VERSION 3.18)

set(TARGET cppsdk)
project(${TARGET})

# set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_library(${TARGET} SHARED "dllmain.cpp" "${LIBRARY_OUTPUT_PATH}/cppsdk.def")
set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME "UE4SS-cppsdk")

add_custom_command(
	OUTPUT
		"${LIBRARY_OUTPUT_PATH}/cppsdk.def"
	COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/../internal_build_tools/cppsdk_gen.py ${LIBRARY_OUTPUT_PATH}/ue4ss.lib ${LIBRARY_OUTPUT_PATH}/cppsdk.def
	DEPENDS
		${CMAKE_CURRENT_SOURCE_DIR}/../internal_build_tools/cppsdk_gen.py
		${LIBRARY_OUTPUT_PATH}/ue4ss.lib
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../internal_build_tools/"
)

add_custom_target(cppsdk_ue4ss_def DEPENDS "${LIBRARY_OUTPUT_PATH}/cppsdk.def")
add_dependencies(${TARGET} cppsdk_ue4ss_def)

target_link_libraries(${TARGET} PRIVATE ue4ss)
get_target_property(UE4SS_INCLUDES ue4ss INCLUDE_DIRECTORIES)
target_include_directories(${TARGET} PUBLIC ${UE4SS_INCLUDES})


add_library(${TARGET}_xinput SHARED "dllmain.cpp" "${LIBRARY_OUTPUT_PATH}/cppsdk_xinput.def")
set_target_properties(${TARGET}_xinput PROPERTIES OUTPUT_NAME "UE4SS-cppsdk_xinput")

add_custom_command(
	OUTPUT
		"${LIBRARY_OUTPUT_PATH}/cppsdk_xinput.def"
	COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/../internal_build_tools/cppsdk_gen.py ${LIBRARY_OUTPUT_PATH}/xinput1_3.lib ${LIBRARY_OUTPUT_PATH}/cppsdk_xinput.def
	DEPENDS
		${CMAKE_CURRENT_SOURCE_DIR}/../internal_build_tools/cppsdk_gen.py
		${LIBRARY_OUTPUT_PATH}/xinput1_3.lib
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../internal_build_tools/"
)

add_custom_target(cppsdk_xinput_def DEPENDS "${LIBRARY_OUTPUT_PATH}/cppsdk_xinput.def")
add_dependencies(${TARGET}_xinput cppsdk_xinput_def)

target_link_libraries(${TARGET}_xinput PRIVATE xinput1_3)
get_target_property(UE4SS_XINPUT_INCLUDES xinput1_3 INCLUDE_DIRECTORIES)
target_include_directories(${TARGET}_xinput PUBLIC ${UE4SS_XINPUT_INCLUDES})