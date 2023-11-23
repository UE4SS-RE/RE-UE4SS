include(FetchContent)

set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(
    safetyhook
    GIT_REPOSITORY "https://github.com/UE4SS-RE/safetyhook.git"
    GIT_TAG "4def59079b95e08dc086cecf5515abde377059e5"
)
FetchContent_MakeAvailable(safetyhook)