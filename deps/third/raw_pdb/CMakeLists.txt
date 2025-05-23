include(FetchContent)

set(FETCHCONTENT_QUIET OFF)

# Set option to disable examples before making the content available
set(RAW_PDB_EXAMPLES OFF CACHE BOOL "Build examples for raw_pdb" FORCE)

FetchContent_MakeAvailable(raw_pdb)

# Suppress warnings from raw_pdb
# Uses suppress_third_party_warnings() from cmake/modules/ThirdPartyWarnings.cmake
suppress_third_party_warnings(raw_pdb)

# Use the generic helper function to organize raw_pdb targets
# Uses organize_targets() from cmake/modules/IDEOrganization.cmake
organize_targets("^raw_pdb$|^Examples$" "raw_pdb")
