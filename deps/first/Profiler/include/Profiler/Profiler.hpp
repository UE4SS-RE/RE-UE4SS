#pragma once

#if STATS && !DISABLE_PROFILER

#if IS_TRACY

#define TRACY_ENABLE 1

#include <tracy/Tracy.hpp>

#define ProfilerFrameMark() FrameMark
#define ProfilerFrameMarkNamed(name) FrameMarkNamed(name)

#define ProfilerScope() ZoneScoped
#define ProfilerScopeNamed(name) ZoneScopedN(name)
#define ProfilerScopeColor(color) ZoneScopedC(color)
#define ProfilerScopeNameColor(name, color) ZoneScopedNC(color)

#define ProfilerSetThreadName(name) tracy::SetThreadName(name)

#elif IS_SUPERLUMINAL

#include <Superluminal/PerformanceAPI.h>

#define ProfilerFrameMark() PERFORMANCEAPI_INSTRUMENT_FUNCTION()
#define ProfilerFrameMarkNamed(name) PERFORMANCEAPI_INSTRUMENT_FUNCTION_DATA(name)

#define ProfilerScope() PERFORMANCEAPI_INSTRUMENT_FUNCTION()
#define ProfilerScopeNamed(name) PERFORMANCEAPI_INSTRUMENT_FUNCTION_DATA(name)
#define ProfilerScopeColor(color) PERFORMANCEAPI_INSTRUMENT_FUNCTION_COLOR(color)
#define ProfilerScopeNameColor(name, color) PERFORMANCEAPI_INSTRUMENT_FUNCTION_DATA_COLOR(name, color)

#define ProfilerSetThreadName(name) PerformanceAPI::SetCurrentThreadName(name)

#else
#error "At least one profiler flavor must be selected"
#endif

#else

#define ProfilerFrameMark()
#define ProfilerFrameMarkNamed(name)

#define ProfilerScope()
#define ProfilerScopeNamed(name)
#define ProfilerScopeColor(color)
#define ProfilerScopeNameColor(name, color)

#define ProfilerSetThreadName(name)

#endif
