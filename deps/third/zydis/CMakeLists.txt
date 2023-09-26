include(FetchContent)

set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(zydis
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/zydis
    GIT_REPOSITORY git@github.com:zyantific/zydis.git
    GIT_TAG v3.1.0
    GIT_PROGRESS ON)

FetchContent_MakeAvailable(zydis)