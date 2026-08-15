#include "lua.h"

#if defined(_WIN32)
#include <windows.h>

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

/*
 * Lua's user lock must be recursive: several public API paths can re-enter
 * the VM while the outer operation still owns the lock.  The mutex is kept
 * for the lifetime of the process because luai_userstatethread also invokes
 * LuaLockInitial for coroutines, so destroying a single shared lock when one
 * state closes would race all remaining states. lua_close enters lua_lock but
 * cannot call lua_unlock after freeing the state; LuaLockFinal is therefore
 * responsible for releasing that final acquisition.
 */
static pthread_once_t GlOnce = PTHREAD_ONCE_INIT;
static pthread_mutex_t GlLock;

static void LuaCreateLock(void)
{
    pthread_mutexattr_t attributes;
    pthread_mutexattr_init(&attributes);
    pthread_mutexattr_settype(&attributes, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&GlLock, &attributes);
    pthread_mutexattr_destroy(&attributes);
}

void LuaLockInitial(lua_State* L)
{
    (void)L;
    pthread_once(&GlOnce, LuaCreateLock);
}

void LuaLockFinal(lua_State* L)
{
    (void)L;
    pthread_mutex_unlock(&GlLock);
}

void LuaLock(lua_State* L)
{
    LuaLockInitial(L);
    pthread_mutex_lock(&GlLock);
}

void LuaUnlock(lua_State* L)
{
    (void)L;
    pthread_mutex_unlock(&GlLock);
}
#endif
