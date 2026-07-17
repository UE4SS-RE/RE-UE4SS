#define _GNU_SOURCE

#include "ue4ss/abi.h"
#include "ue4ss/sha256.h"

#include <dlfcn.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

typedef int (*ue4ss_main_fn)(int, char**, char**);
typedef int (*ue4ss_libc_start_main_fn)(ue4ss_main_fn, int, char**, void (*)(void), void (*)(void), void (*)(void), void*);
typedef int (*ue4ss_start_fn)(const ue4ss_boot_config*);
typedef int (*ue4ss_get_status_fn)(ue4ss_status*);
typedef void (*ue4ss_shutdown_fn)(void);

static ue4ss_main_fn g_application_main;
static void* g_core_handle;
static ue4ss_shutdown_fn g_shutdown;

static void loader_log(const char* level, const char* message)
{
    char buffer[2048];
    const int length = snprintf(buffer, sizeof(buffer), "[ue4ss-preload] [%s] %s\n", level, message != NULL ? message : "");
    if (length <= 0)
    {
        return;
    }
    size_t remaining = (size_t)length < sizeof(buffer) ? (size_t)length : sizeof(buffer) - 1u;
    const char* cursor = buffer;
    while (remaining > 0u)
    {
        const ssize_t written = write(STDERR_FILENO, cursor, remaining);
        if (written > 0)
        {
            cursor += written;
            remaining -= (size_t)written;
        }
        else if (written < 0 && errno == EINTR)
        {
            continue;
        }
        else
        {
            break;
        }
    }
}

static bool env_true(const char* name)
{
    const char* value = getenv(name);
    return value != NULL && (strcmp(value, "1") == 0 || strcasecmp(value, "true") == 0 || strcasecmp(value, "yes") == 0 || strcasecmp(value, "on") == 0);
}

static int executable_path(char output[PATH_MAX])
{
    const ssize_t length = readlink("/proc/self/exe", output, PATH_MAX - 1u);
    if (length < 0)
    {
        return errno != 0 ? errno : EIO;
    }
    output[(size_t)length] = '\0';
    return 0;
}

static int loader_directory(char output[PATH_MAX])
{
    Dl_info info;
    memset(&info, 0, sizeof(info));
    if (dladdr((const void*)&loader_directory, &info) == 0 || info.dli_fname == NULL)
    {
        return ENOENT;
    }
    char resolved[PATH_MAX];
    const char* source = realpath(info.dli_fname, resolved) != NULL ? resolved : info.dli_fname;
    const size_t length = strlen(source);
    if (length >= PATH_MAX)
    {
        return ENAMETOOLONG;
    }
    memcpy(output, source, length + 1u);
    char* slash = strrchr(output, '/');
    if (slash == NULL)
    {
        memcpy(output, ".", 2u);
    }
    else if (slash == output)
    {
        slash[1] = '\0';
    }
    else
    {
        *slash = '\0';
    }
    return 0;
}

static void strip_own_preload(void)
{
    const char* preload = getenv("LD_PRELOAD");
    if (preload == NULL || preload[0] == '\0')
    {
        return;
    }

    Dl_info info;
    memset(&info, 0, sizeof(info));
    if (dladdr((const void*)&strip_own_preload, &info) == 0 || info.dli_fname == NULL)
    {
        return;
    }
    char own_path[PATH_MAX];
    const char* own = realpath(info.dli_fname, own_path) != NULL ? own_path : info.dli_fname;

    char* input = strdup(preload);
    char* output = calloc(strlen(preload) + 1u, 1u);
    if (input == NULL || output == NULL)
    {
        free(input);
        free(output);
        return;
    }

    bool first = true;
    char* save = NULL;
    for (char* token = strtok_r(input, " :\t\r\n", &save); token != NULL; token = strtok_r(NULL, " :\t\r\n", &save))
    {
        char token_path[PATH_MAX];
        const char* comparable = realpath(token, token_path) != NULL ? token_path : token;
        if (strcmp(comparable, own) == 0)
        {
            continue;
        }
        if (!first)
        {
            strcat(output, ":");
        }
        strcat(output, token);
        first = false;
    }
    if (output[0] == '\0')
    {
        unsetenv("LD_PRELOAD");
    }
    else
    {
        setenv("LD_PRELOAD", output, 1);
    }
    free(input);
    free(output);
}

static int load_core(bool required)
{
    char root[PATH_MAX];
    const char* configured_root = getenv("UE4SS_ROOT");
    if (configured_root != NULL && configured_root[0] != '\0')
    {
        if (strlen(configured_root) >= sizeof(root))
        {
            loader_log("error", "UE4SS_ROOT is too long");
            return ENAMETOOLONG;
        }
        strcpy(root, configured_root);
    }
    else
    {
        const int root_error = loader_directory(root);
        if (root_error != 0)
        {
            loader_log("error", "cannot determine preload library directory");
            return root_error;
        }
    }

    char state_path[PATH_MAX];
    const char* configured_state = getenv("UE4SS_STATE_DIR");
    const char* selected_state = configured_state != NULL && configured_state[0] != '\0' ? configured_state : root;
    if (strlen(selected_state) >= sizeof(state_path))
    {
        loader_log("error", "UE4SS_STATE_DIR is too long");
        return ENAMETOOLONG;
    }
    strcpy(state_path, selected_state);

    char core_path[PATH_MAX];
    const int core_path_length = snprintf(core_path, sizeof(core_path), "%s/libUE4SS.so", root);
    if (core_path_length < 0 || (size_t)core_path_length >= sizeof(core_path))
    {
        loader_log("error", "UE4SS core path is too long");
        return ENAMETOOLONG;
    }

    const char* expected_core_hash = getenv("UE4SS_EXPECTED_SHA256");
    if (expected_core_hash != NULL && expected_core_hash[0] != '\0')
    {
        char actual_hash[65];
        const int hash_error = ue4ss_sha256_file(core_path, actual_hash);
        if (hash_error != 0)
        {
            loader_log("error", "cannot hash libUE4SS.so");
            return hash_error;
        }
        if (strlen(expected_core_hash) != 64u || strcasecmp(expected_core_hash, actual_hash) != 0)
        {
            loader_log("error", "libUE4SS.so does not match UE4SS_EXPECTED_SHA256");
            return EPERM;
        }
    }

    int flags = RTLD_NOW | RTLD_LOCAL;
#ifdef RTLD_DEEPBIND
    flags |= RTLD_DEEPBIND;
#endif
#ifdef RTLD_NODELETE
    // Inline entry patches are restored before shutdown, but a thread may
    // already have fetched the old jump and enter the detour late. Keeping the
    // isolated core mapped until process exit makes that retirement path safe.
    flags |= RTLD_NODELETE;
#endif
    g_core_handle = dlopen(core_path, flags);
    if (g_core_handle == NULL)
    {
        loader_log("error", dlerror());
        return ENOENT;
    }

    ue4ss_start_fn start = NULL;
    ue4ss_get_status_fn get_status = NULL;
    *(void**)(&start) = dlsym(g_core_handle, "ue4ss_start");
    *(void**)(&get_status) = dlsym(g_core_handle, "ue4ss_get_status");
    *(void**)(&g_shutdown) = dlsym(g_core_handle, "ue4ss_shutdown");
    if (start == NULL || get_status == NULL || g_shutdown == NULL)
    {
        loader_log("error", "libUE4SS.so does not export ABI version 1");
        dlclose(g_core_handle);
        g_core_handle = NULL;
        g_shutdown = NULL;
        return EPROTO;
    }

    char application_path[PATH_MAX];
    const int executable_error = executable_path(application_path);
    if (executable_error != 0)
    {
        loader_log("error", "cannot resolve /proc/self/exe");
        return executable_error;
    }

    const ue4ss_boot_config config = {
            .struct_size = sizeof(ue4ss_boot_config),
            .abi_version = UE4SS_LINUX_ABI_VERSION,
            .root_path = root,
            .state_path = state_path,
            .executable_path = application_path,
            .required = required ? 1u : 0u,
            .reserved = {0},
    };
    const int start_error = start(&config);
    ue4ss_status status;
    memset(&status, 0, sizeof(status));
    const int status_error = get_status(&status);
    if (start_error != 0 || status_error != 0 || status.state == UE4SS_STATE_FAILED ||
        (required && status.state != UE4SS_STATE_PLATFORM_READY && status.state != UE4SS_STATE_DEGRADED))
    {
        loader_log("error", status_error == 0 ? status.message : "cannot query UE4SS core status");
        return start_error != 0 ? start_error : EIO;
    }
    loader_log("info",
               status.state == UE4SS_STATE_INITIALIZING ? "core initialization started asynchronously" : "core ready; unavailable capabilities remain fail-closed");
    return 0;
}

static int ue4ss_wrapped_main(int argc, char** argv, char** environment)
{
    const bool required = env_true("UE4SS_REQUIRED");
    strip_own_preload();
    const int core_error = load_core(required);
    if (core_error != 0 && required)
    {
        loader_log("error", "UE4SS_REQUIRED is true; refusing to start the application");
        return 78;
    }
    if (core_error != 0)
    {
        loader_log("warning", "UE4SS unavailable; starting the application in vanilla mode");
    }

    const int result = g_application_main(argc, argv, environment);
    if (g_shutdown != NULL)
    {
        g_shutdown();
    }
    if (g_core_handle != NULL)
    {
        dlclose(g_core_handle);
        g_core_handle = NULL;
    }
    return result;
}

__attribute__((visibility("default"))) int __libc_start_main(
        ue4ss_main_fn main_function, int argc, char** argv, void (*init)(void), void (*fini)(void), void (*rtld_fini)(void), void* stack_end)
{
    ue4ss_libc_start_main_fn real_start = NULL;
    *(void**)(&real_start) = dlsym(RTLD_NEXT, "__libc_start_main");
    if (real_start == NULL)
    {
        loader_log("fatal", "cannot resolve the real __libc_start_main");
        _exit(127);
    }
    g_application_main = main_function;
    return real_start(ue4ss_wrapped_main, argc, argv, init, fini, rtld_fini, stack_end);
}
