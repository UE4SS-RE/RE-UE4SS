include(FetchContent)

set(FETCHCONTENT_QUIET OFF)

# Set option to disable examples before making the content available
set(RAW_PDB_EXAMPLES OFF CACHE BOOL "Build examples for raw_pdb" FORCE)

FetchContent_MakeAvailable(raw_pdb)

# Ensure raw_pdb is in the deps/third folder
# Uses set_folder_for_target() from cmake/modules/IDEOrganization.cmake
if(TARGET raw_pdb)
    set_folder_for_target(raw_pdb "deps/third")
endif()

if(TARGET Examples)
    set_folder_for_target(Examples "deps/third")
endif()
