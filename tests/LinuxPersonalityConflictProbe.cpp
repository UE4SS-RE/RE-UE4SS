#include <chrono>
#include <cstdio>
#include <thread>

extern "C" void hostile_personality_fixture_loaded();

int main()
{
    hostile_personality_fixture_loaded();
    std::this_thread::sleep_for(std::chrono::milliseconds{1500});
    std::puts("UE4SS_FAIL_SOFT_PROBE_READY");
    return 0;
}
