include(FetchContent)

set(FETCHCONTENT_QUIET OFF)

# Set option to disable examples before making the content available
set(RAW_PDB_EXAMPLES OFF CACHE BOOL "Build examples for raw_pdb" FORCE)

FetchContent_MakeAvailable(raw_pdb)

# Ensure raw_pdb is in the deps/third folder
if(TARGET raw_pdb)
    set_target_properties(raw_pdb PROPERTIES FOLDER "deps/third")
endif()

if(TARGET Examples)
    set_target_properties(Examples PROPERTIES FOLDER "deps/third")
endif()
