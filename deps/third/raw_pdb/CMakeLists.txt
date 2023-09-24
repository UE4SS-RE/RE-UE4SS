include(FetchContent)

set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(raw_pdb
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/raw_pdb
    GIT_REPOSITORY git@github.com:MolecularMatters/raw_pdb.git
    GIT_TAG 8c6a7146393c83d27fa101e8bc8017f2a7f151df
    GIT_PROGRESS ON)

FetchContent_MakeAvailable(raw_pdb)