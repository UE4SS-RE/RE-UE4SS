cmake_minimum_required(VERSION 3.22)
set(TARGET EventViewerMod)
project(${TARGET})

include(FetchContent)

FetchContent_Declare(
        concurrentqueue
        GIT_REPOSITORY git@github.com:cameron314/concurrentqueue.git
        GIT_TAG c68072129c8a5b4025122ca5a0c82ab14b30cb03
)
FetchContent_MakeAvailable(concurrentqueue)

add_library(${TARGET} SHARED
        src/dllmain.cpp
        src/EventViewer.cpp
        src/Middleware.cpp
        include/Middleware.hpp
        include/Structs.h
        src/Client.cpp
        include/Client.h
        include/Enums.h
        src/Structs.cpp
        include/StringPool.hpp
        src/StringPool.cpp
)

target_include_directories(${TARGET} PRIVATE "include")
target_link_libraries(${TARGET} PRIVATE ImGui)
target_link_libraries(${TARGET} PRIVATE concurrentqueue)
target_link_libraries(${TARGET} PUBLIC UE4SS)