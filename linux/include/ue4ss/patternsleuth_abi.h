#ifndef UE4SS_PATTERNSLEUTH_ABI_H
#define UE4SS_PATTERNSLEUTH_ABI_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define UE4SS_PS_LINUX_ABI_VERSION 1u

    enum ue4ss_ps_log_level
    {
        UE4SS_PS_LOG_INFO = 0,
        UE4SS_PS_LOG_WARNING = 1,
        UE4SS_PS_LOG_ERROR = 2
    };

    enum ue4ss_ps_capability
    {
        UE4SS_PS_GUOBJECT_ARRAY = 1ull << 0u,
        UE4SS_PS_FNAME_TO_STRING = 1ull << 1u,
        UE4SS_PS_FNAME_CTOR = 1ull << 2u,
        UE4SS_PS_GMALLOC = 1ull << 3u,
        UE4SS_PS_STATIC_CONSTRUCT_OBJECT = 1ull << 4u,
        UE4SS_PS_STATIC_FIND_OBJECT = 1ull << 5u,
        UE4SS_PS_GAME_ENGINE_TICK = 1ull << 6u,
        UE4SS_PS_ENGINE_LOOP_INIT = 1ull << 7u,
        UE4SS_PS_UFUNCTION_BIND = 1ull << 8u,
        UE4SS_PS_ALLOCATE_UOBJECT_INDEX = 1ull << 9u,
        UE4SS_PS_FREE_UOBJECT_INDEX = 1ull << 10u,
        UE4SS_PS_ENGINE_VERSION = 1ull << 11u
    };

    typedef void (*ue4ss_ps_log_fn)(uint32_t level, const uint8_t* message, uintptr_t length, void* user_data);

    typedef struct ue4ss_ps_linux_config
    {
        uint32_t struct_size;
        uint32_t abi_version;
        uint64_t enabled_mask;
        ue4ss_ps_log_fn log;
        void* user_data;
    } ue4ss_ps_linux_config;

    typedef struct ue4ss_ps_linux_results
    {
        uint32_t struct_size;
        uint32_t abi_version;
        uint64_t available_mask;
        uint64_t failed_mask;
        uint16_t engine_major;
        uint16_t engine_minor;
        uint32_t reserved;
        uint64_t guobject_array;
        uint64_t fname_to_string;
        uint64_t fname_ctor;
        uint64_t gmalloc;
        uint64_t static_construct_object;
        uint64_t static_find_object;
        uint64_t game_engine_tick;
        uint64_t engine_loop_init;
        uint64_t ufunction_bind;
        uint64_t allocate_uobject_index;
        uint64_t free_uobject_index;
    } ue4ss_ps_linux_results;

    int32_t ps_scan_linux_v1(const ue4ss_ps_linux_config* config, ue4ss_ps_linux_results* results);
    int32_t ps_scan_linux_file_v1(const char* path, const ue4ss_ps_linux_config* config, ue4ss_ps_linux_results* results);

#ifdef __cplusplus
}
#endif

#endif
