#ifdef _WIN32
#include <windows.h>
#include "lua.h"

static struct {
    CRITICAL_SECTION LockSct;
    BOOL Init;
} Gl;

void LuaLockInitial(lua_State* L)
{
    if (!Gl.Init)
    {
        /* Create a mutex */
        InitializeCriticalSection(&Gl.LockSct);
        Gl.Init = TRUE;
    }
}

void LuaLockFinal(lua_State* L)
{
    /* Destroy a mutex. */
    if (Gl.Init)
    {
        DeleteCriticalSection(&Gl.LockSct);
        Gl.Init = FALSE;
    }
}

void LuaLock(lua_State* L)
{
    LuaLockInitial(L);
    /* Wait for control of mutex */
    EnterCriticalSection(&Gl.LockSct);
}

void LuaUnlock(lua_State* L)
{
    /* Release control of mutex */
    LeaveCriticalSection(&Gl.LockSct);
}
#else
#include <pthread.h>
#include "lua.h"

/* CRITICAL_SECTION is recursive, so the POSIX equivalent must be a recursive mutex.
   pthread_once removes the lazy-init race the Windows implementation has. */
static pthread_mutex_t Gl_lock;
static pthread_once_t Gl_once = PTHREAD_ONCE_INIT;

static void lua_lock_init(void)
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&Gl_lock, &attr);
    pthread_mutexattr_destroy(&attr);
}

void LuaLockInitial(lua_State* L)
{
    (void)L;
    pthread_once(&Gl_once, lua_lock_init);
}

void LuaLockFinal(lua_State* L)
{
    (void)L;
}

void LuaLock(lua_State* L)
{
    LuaLockInitial(L);
    pthread_mutex_lock(&Gl_lock);
}

void LuaUnlock(lua_State* L)
{
    (void)L;
    pthread_mutex_unlock(&Gl_lock);
}
#endif
