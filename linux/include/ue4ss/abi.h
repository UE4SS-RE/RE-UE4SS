#ifndef UE4SS_LINUX_ABI_H
#define UE4SS_LINUX_ABI_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define UE4SS_LINUX_ABI_VERSION 1u
#define UE4SS_STATUS_MESSAGE_SIZE 512u
#define UE4SS_STATUS_PATH_SIZE 4096u
#define UE4SS_STATUS_BUILD_ID_SIZE 128u
#define UE4SS_STATUS_SHA256_SIZE 65u

#if defined(__GNUC__) && defined(UE4SS_BUILDING_CORE)
#define UE4SS_API __attribute__((visibility("default")))
#else
#define UE4SS_API
#endif

    typedef enum ue4ss_runtime_state
    {
        UE4SS_STATE_NOT_STARTED = 0,
        UE4SS_STATE_INITIALIZING = 1,
        UE4SS_STATE_PLATFORM_READY = 2,
        UE4SS_STATE_DEGRADED = 3,
        UE4SS_STATE_FAILED = 4,
        UE4SS_STATE_STOPPED = 5
    } ue4ss_runtime_state;

    typedef struct ue4ss_boot_config
    {
        uint32_t struct_size;
        uint32_t abi_version;
        const char* root_path;
        const char* state_path;
        const char* executable_path;
        uint8_t required;
        uint8_t reserved[7];
    } ue4ss_boot_config;

    typedef struct ue4ss_status
    {
        uint32_t struct_size;
        uint32_t abi_version;
        uint32_t state;
        uint32_t capability_count;
        int32_t last_error;
        uint32_t process_id;
        char message[UE4SS_STATUS_MESSAGE_SIZE];
        char executable_path[UE4SS_STATUS_PATH_SIZE];
        char state_path[UE4SS_STATUS_PATH_SIZE];
        char executable_sha256[UE4SS_STATUS_SHA256_SIZE];
        char executable_build_id[UE4SS_STATUS_BUILD_ID_SIZE];
    } ue4ss_status;

    UE4SS_API int ue4ss_start(const ue4ss_boot_config* config);
    UE4SS_API int ue4ss_get_status(ue4ss_status* status);
    UE4SS_API void ue4ss_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif
