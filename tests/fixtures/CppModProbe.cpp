#include <cstdlib>
#include <fstream>

#include <Mod/CppUserModBase.hpp>

namespace
{
    auto record(const char* marker) -> void
    {
        const auto* log_path = std::getenv("CPP_MOD_PROBE_LOG");
        if (!log_path)
        {
            return;
        }
        std::ofstream log{log_path, std::ios::app};
        log << marker << '\n';
    }

    class CppModProbe : public RC::CppUserModBase
    {
      public:
        auto on_program_start() -> void override
        {
            record("program_start");
        }

        auto on_unreal_init() -> void override
        {
            record("unreal_init");
        }

        auto on_update() -> void override
        {
            record("update");
        }

        auto on_dll_load(RC::StringViewType) -> void override
        {
            record("dll_load");
        }

        auto on_cpp_mods_loaded() -> void override
        {
            record("cpp_mods_loaded");
        }
    };
} // namespace

extern "C" __attribute__((visibility("default"))) RC::CppUserModBase* start_mod()
{
    record("start_mod");
    return new CppModProbe{};
}

extern "C" __attribute__((visibility("default"))) void uninstall_mod(RC::CppUserModBase* mod)
{
    record("uninstall_mod");
    delete mod;
}
