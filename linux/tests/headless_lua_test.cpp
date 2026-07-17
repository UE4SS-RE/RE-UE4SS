#include "headless_lua.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unistd.h>

namespace
{
    void require(bool condition, const std::string& message)
    {
        if (!condition)
        {
            throw std::runtime_error(message);
        }
    }
}

int main()
{
    try
    {
        const auto probe = ue4ss::linux::core::probe_lua_runtime();
        require(probe.success, probe.detail);

        bool cross_thread_probe_succeeded{};
        std::string cross_thread_detail;
        std::thread cross_thread_probe{[&] {
            const auto result = ue4ss::linux::core::probe_lua_runtime();
            cross_thread_probe_succeeded = result.success;
            cross_thread_detail = result.detail;
        }};
        cross_thread_probe.join();
        require(cross_thread_probe_succeeded,
                "Lua state could not be closed on one thread and reopened on another: " + cross_thread_detail);

        ue4ss::linux::core::HeadlessLuaState lua;
        require(lua.is_ready(), "headless Lua state was not created");
        require(lua.execute_string("local value = 40 + 2; assert(value == 42)").success, "valid Lua source failed");

        const auto callback_error = lua.execute_string("error('callback boundary sentinel')");
        require(!callback_error.success && callback_error.detail.find("callback boundary sentinel") != std::string::npos,
                "Lua callback error did not remain inside the protected boundary");

        const auto syntax_error = lua.execute_string("local broken = )");
        require(!syntax_error.success, "invalid Lua syntax was accepted");

        const std::filesystem::path script = std::filesystem::temp_directory_path() /
                                             ("ue4ss-headless-lua-" + std::to_string(getpid()) + ".lua");
        std::ofstream{script} << "assert(type(package) == 'table')\n";
        const auto file_result = lua.execute_file(script);
        std::filesystem::remove(script);
        require(file_result.success, file_result.detail);

        std::cout << "headless Lua runtime tests passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "test failure: " << error.what() << '\n';
        return 1;
    }
}
