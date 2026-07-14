#include <atomic>
#include <cstdio>

#include <Mod/CppUserModBase.hpp>

namespace
{
    auto record(const char* marker) -> void
    {
        std::fprintf(stderr, "%s\n", marker);
        std::fflush(stderr);
    }

    class CppProbeMod final : public RC::CppUserModBase
    {
      private:
        std::atomic_uint32_t m_update_count{};

      public:
        auto on_program_start() -> void override
        {
            record("ACCEPT:CPP_PROGRAM_START");
        }

        auto on_unreal_init() -> void override
        {
            record("ACCEPT:CPP_UNREAL_INIT");
        }

        auto on_update() -> void override
        {
            const auto count = m_update_count.fetch_add(1, std::memory_order_relaxed) + 1;
            if (count <= 10)
            {
                std::fprintf(stderr, "ACCEPT:CPP_UPDATE count=%u\n", count);
                std::fflush(stderr);
            }
        }
    };
} // namespace

extern "C" __attribute__((visibility("default"))) RC::CppUserModBase* start_mod()
{
    record("ACCEPT:CPP_START_MOD");
    return new CppProbeMod{};
}

extern "C" __attribute__((visibility("default"))) void uninstall_mod(RC::CppUserModBase* mod)
{
    record("ACCEPT:CPP_UNINSTALL_MOD");
    delete mod;
}
