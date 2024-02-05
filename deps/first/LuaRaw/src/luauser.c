#ifdef WIN32
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
#include <stdbool.h>
#include <pthread.h>
#include "lua.h"
 
static struct {
    pthread_mutex_t mutex;
    bool init;
} lua_lock_mutex;
 
void LuaLock(lua_State * L){
    pthread_mutex_lock(&lua_lock_mutex.mutex);
}
 
void LuaUnlock(lua_State * L){
    pthread_mutex_unlock(&lua_lock_mutex.mutex);
}
 
void LuaLockInitial(lua_State * L){
    if (!lua_lock_mutex.init){
        pthread_mutex_init(&lua_lock_mutex.mutex, NULL);
        lua_lock_mutex.init = true;
    }
}

void LuaLockFinal(lua_State * L){
    if (lua_lock_mutex.init){
        pthread_mutex_destroy(&lua_lock_mutex.mutex);
        lua_lock_mutex.init = false;
    }
}

#endif