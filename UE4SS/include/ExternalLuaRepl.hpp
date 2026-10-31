#pragma once

#include <filesystem>

namespace RC
{
    class LuaMod;

    // An opt-in, local-only interface through which an external tool runs a Lua chunk in a started Lua mod and gets
    // back its print output, its return values or its error: the Lua Debugger's REPL without the GUI.
    // It is off unless [Debug] ExternalLuaReplEnabled is set. Requests and answers are files in the 'LuaRepl' folder
    // of the working directory; docs/feature-overview/external-lua-repl.md describes the protocol.
    //
    // Requests run on the game thread from a UEngine::Tick pre-callback, one per tick, on a Lua thread of the target
    // mod that is used for nothing else. The callback holds the lock that LuaMod::uninstall takes before it closes a
    // mod's state (LuaMod::game_thread_execution_mutex), so a mod cannot be torn down under a running request.
    class ExternalLuaRepl
    {
      public:
        // The value of [Debug] ExternalLuaReplEnabled. While it is false, nothing in this class creates or registers
        // anything.
        static auto is_enabled() -> bool;

        // Called by UE4SSProgram once the Lua mods have been started, and when the program is destroyed.
        static auto start(const std::filesystem::path& working_directory) -> void;
        static auto stop() -> void;

        // Called by LuaMod. add_mod at the end of start_mod. remove_mod from uninstall while it holds
        // LuaMod::game_thread_execution_mutex, before anything that belongs to the mod is released.
        static auto add_mod(const LuaMod& mod) -> void;
        static auto remove_mod(const LuaMod& mod) -> void;
    };
} // namespace RC
