#include <cstdint>
#include <memory>

#include <polyhook2/Detour/x64Detour.hpp>
#include <polyhook2/ErrorLog.hpp>

namespace
{
    uint64_t s_trampoline{};
    volatile bool s_replacement_called{};

    extern "C" __attribute__((noinline)) int hook_target(int value)
    {
        asm volatile("nop\n\t"
                     "nop\n\t"
                     "nop\n\t"
                     "nop\n\t"
                     "nop\n\t"
                     "nop\n\t"
                     "nop\n\t"
                     "nop\n\t"
                     "nop\n\t"
                     "nop\n\t"
                     "nop\n\t"
                     "nop\n\t"
                     "nop\n\t"
                     "nop\n\t"
                     "nop\n\t"
                     "nop");
        volatile int adjusted = value + 1;
        return adjusted;
    }

    extern "C" __attribute__((noinline)) int hook_replacement(int value)
    {
        s_replacement_called = true;
        const auto original = reinterpret_cast<int (*)(int)>(s_trampoline);
        return original(value) + 10;
    }
} // namespace

int main()
{
    auto logger = std::make_shared<PLH::ErrorLog>();
    logger->setLogLevel(PLH::ErrorLevel::INFO);
    PLH::Log::registerLogger(logger);

    if (hook_target(5) != 6)
    {
        return 1;
    }

    PLH::x64Detour detour{
            reinterpret_cast<uint64_t>(&hook_target), reinterpret_cast<uint64_t>(&hook_replacement), &s_trampoline};
    if (!detour.hook() || s_trampoline == 0)
    {
        return 2;
    }

    s_replacement_called = false;
    if (hook_target(5) != 16 || !s_replacement_called)
    {
        return 3;
    }

    if (!detour.unHook())
    {
        return 4;
    }
    return hook_target(5) == 6 ? 0 : 5;
}
