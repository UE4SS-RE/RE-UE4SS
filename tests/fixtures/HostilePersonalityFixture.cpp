#include <cstdlib>
#include <unwind.h>

extern "C" void hostile_personality_fixture_loaded()
{
}

extern "C" _Unwind_Reason_Code __gxx_personality_v0(int,
                                                     _Unwind_Action,
                                                     unsigned long long,
                                                     _Unwind_Exception*,
                                                     _Unwind_Context*)
{
    std::abort();
}

extern "C" [[noreturn]] void hostile_cxa_throw(void*, void*, void (*)(void*)) __asm__("__cxa_throw");

extern "C" [[noreturn]] void hostile_cxa_throw(void*, void*, void (*)(void*))
{
    std::abort();
}
