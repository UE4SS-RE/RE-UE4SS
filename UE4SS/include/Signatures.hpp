#pragma once

#include <functional>

namespace RC::Unreal::UnrealInitializer
{
    struct Config;
}

namespace RC
{
    class SignatureContainer;

    enum class DidLuaScanSucceed
    {
        Yes,
        No,
    };

    auto scan_complete_default_func(DidLuaScanSucceed) -> void;

    using LuaScriptMatchFoundFunc = const std::function<DidLuaScanSucceed(void*)>;
    using LuaScriptScanCompleteFunc = const std::function<void(DidLuaScanSucceed)>;
    auto scan_from_lua_script(SystemStringType& script_file_path_and_name,
                              std::vector<SignatureContainer>&,
                              LuaScriptMatchFoundFunc& match_found_func,
                              LuaScriptScanCompleteFunc& scan_complete_func = &scan_complete_default_func) -> void;

    auto setup_lua_scan_overrides(std::filesystem::path& working_directory, Unreal::UnrealInitializer::Config&) -> void;
} // namespace RC
