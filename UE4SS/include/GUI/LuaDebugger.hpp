#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <filesystem>
#include <functional>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include <File/Macros.hpp>
#include <TextEditor.h>

struct lua_State;
struct lua_Debug;

namespace RC::LuaMadeSimple
{
    class Lua;
}

namespace RC::GUI
{
    // Represents a single entry in the Lua call stack
    struct LuaCallStackEntry
    {
        std::string function_name;
        std::string source;
        int line_number{0};
        int current_line{0};
        std::string what; // "Lua", "C", "main", "tail"
        std::string name_what; // "global", "local", "method", "field", ""
    };

    // Represents a single stack slot value
    struct LuaStackSlot
    {
        int index{0};
        std::string type_name; // "nil", "boolean", "number", "string", "table", "function", "userdata", "thread", "lightuserdata"
        std::string value_preview; // Short string representation of the value
        bool is_table{false};
        bool is_userdata{false};
    };

    // Represents a local variable at a stack frame
    struct LuaLocalVariable
    {
        std::string name;
        std::string type_name;
        std::string value_preview;
        bool is_table{false};
        bool is_userdata{false};
    };

    // Represents a stack frame with its local variables
    struct LuaStackFrame
    {
        std::string function_name;
        std::string source;
        int line_number{0};
        int current_line{0};
        std::string what;
        std::vector<LuaLocalVariable> locals;
    };

    // Represents a value in the variable tree (for table inspection)
    struct LuaValueNode
    {
        std::string key;
        std::string type_name;
        std::string value_preview;
        bool is_expandable{false}; // true for tables/userdata with metatable
        bool is_expanded{false};
        std::vector<LuaValueNode> children;
        int depth{0};
        std::string path; // Full path for fetching (e.g., "myTable.subTable.value")
    };

    // Represents a breakpoint
    struct LuaBreakpoint
    {
        std::string source_file; // Source file path
        int line{0};
        bool enabled{true};
        std::string condition; // Optional condition expression
        int hit_count{0};
    };

    // Represents a script file for viewing
    struct LuaScriptFile
    {
        std::string path;
        std::string content;
        std::vector<std::string> lines;
        bool loaded{false};
    };

    // REPL history entry
    struct ReplHistoryEntry
    {
        std::string input;
        std::string output;
        bool is_error{false};
        std::chrono::system_clock::time_point timestamp;
    };

    // Represents a captured Lua error with full context
    struct LuaErrorRecord
    {
        std::chrono::system_clock::time_point timestamp;
        std::string mod_name;
        std::string error_message;
        std::string traceback;
        std::vector<LuaCallStackEntry> call_stack;
        std::vector<LuaStackSlot> stack_snapshot;
    };

    // Information about a tracked Lua state
    struct LuaStateInfo
    {
        lua_State* lua_state{nullptr};
        std::string mod_name;
        std::string state_type; // "main", "hook", "async"
        int current_stack_size{0};
        std::vector<LuaStackSlot> current_stack;
        bool is_active{true};
    };

    class LuaDebugger
    {
    public:
        static constexpr size_t MAX_ERROR_HISTORY = 100;
        static constexpr size_t MAX_STACK_PREVIEW_LENGTH = 64;
        static constexpr size_t MAX_REPL_HISTORY = 100;
        static constexpr size_t MAX_TABLE_DEPTH = 10;

    private:
        // All tracked Lua states
        std::unordered_map<lua_State*, LuaStateInfo> m_lua_states;
        mutable std::mutex m_states_mutex;
        
        std::deque<LuaErrorRecord> m_error_history;
        mutable std::mutex m_errors_mutex;

        // UI state
        bool m_auto_scroll_errors{true};
        int m_selected_error_index{-1};
        lua_State* m_selected_state{nullptr};
        bool m_show_stack_details{true};
        bool m_pause_on_error{false};
        std::string m_error_filter;
        
        static LuaDebugger* s_instance;

        // Breakpoints
        std::vector<LuaBreakpoint> m_breakpoints;
        mutable std::mutex m_breakpoints_mutex;
        std::atomic<bool> m_is_paused{false};
        std::atomic<bool> m_step_requested{false};
        std::atomic<bool> m_step_over_requested{false};
        std::atomic<bool> m_step_out_requested{false};
        std::atomic<bool> m_continue_requested{false};
        lua_State* m_paused_state{nullptr};
        int m_paused_line{0};
        std::string m_paused_source;
        std::condition_variable m_pause_cv;
        std::mutex m_pause_mutex;
        int m_step_start_depth{0};

        // Cached paused state info (captured on Lua thread, read on GUI thread)
        // IMPORTANT: Never manipulate the Lua state from the GUI thread while paused!
        std::vector<LuaStackSlot> m_paused_stack_slots;
        std::vector<LuaCallStackEntry> m_paused_call_stack;
        std::vector<LuaStackFrame> m_paused_stack_frames; // Call stack with local variables
        mutable std::mutex m_paused_data_mutex;

        // Track which states have debug hooks installed
        std::set<lua_State*> m_states_with_hooks;

        // Script viewer (for Debug view)
        std::unordered_map<std::string, LuaScriptFile> m_script_cache;
        std::string m_current_script_path;
        int m_script_scroll_to_line{-1};
        mutable std::mutex m_scripts_mutex;

        // Script editor state (for Script Editor tab)
        TextEditor m_script_editor;
        bool m_script_is_dirty{false};
        std::string m_script_edit_path;
        std::string m_script_original_content;

        // Track which states had debug enabled (for auto-restore on reload)
        std::set<std::string> m_debug_enabled_mods;

        // Cached loaded module paths per state (populated on Lua thread)
        std::unordered_map<lua_State*, std::vector<std::string>> m_loaded_modules_cache;
        mutable std::mutex m_loaded_modules_mutex;

        // REPL
        std::string m_repl_input;
        std::deque<ReplHistoryEntry> m_repl_history;
        int m_repl_history_index{-1};
        std::vector<std::string> m_repl_input_history;
        mutable std::mutex m_repl_mutex;
        bool m_repl_pending{false};
        std::string m_repl_pending_result;
        bool m_repl_pending_is_error{false};

        // Table inspection
        std::vector<LuaValueNode> m_globals_tree;
        std::set<std::string> m_expanded_paths;
        bool m_tree_refresh_requested{false};

        // Stack frame expansion state (-1 = no change, 0 = collapse all, 1 = expand all)
        int m_stack_frames_expand_action{-1};

    public:
        LuaDebugger();
        ~LuaDebugger();

        static auto get() -> LuaDebugger&;
        static auto has_instance() -> bool;

        // State tracking
        auto register_lua_state(lua_State* L, const std::string& mod_name, const std::string& state_type) -> void;
        auto unregister_lua_state(lua_State* L) -> void;
        auto update_state_stack(lua_State* L) -> void;

        // Error recording
        auto record_error(lua_State* L, const std::string& error_message, const std::string& traceback) -> void;
        auto clear_error_history() -> void;
        auto get_error_count() const -> size_t;

        // Stack inspection utilities
        static auto get_stack_slots(lua_State* L) -> std::vector<LuaStackSlot>;
        static auto get_call_stack(lua_State* L) -> std::vector<LuaCallStackEntry>;
        static auto get_stack_frames_with_locals(lua_State* L) -> std::vector<LuaStackFrame>;
        static auto format_stack_value(lua_State* L, int index, size_t max_length = MAX_STACK_PREVIEW_LENGTH) -> std::string;
        static auto get_enhanced_traceback(lua_State* L, const std::string& message, int level = 0) -> std::string;
        static auto get_globals(lua_State* L, size_t max_entries = 100) -> std::vector<std::pair<std::string, LuaStackSlot>>;

        // Breakpoint management
        auto add_breakpoint(const std::string& source, int line, const std::string& condition = "") -> void;
        auto remove_breakpoint(const std::string& source, int line) -> void;
        auto toggle_breakpoint(const std::string& source, int line) -> void;
        auto clear_all_breakpoints() -> void;
        auto has_breakpoint(const std::string& source, int line) const -> bool;
        auto is_paused() const -> bool { return m_is_paused.load(); }

        // Debug control
        auto continue_execution() -> void;
        auto step_into() -> void;
        auto step_over() -> void;
        auto step_out() -> void;

        // Debug hook (called from Lua)
        static auto debug_hook(lua_State* L, lua_Debug* ar) -> void;
        auto install_debug_hook(lua_State* L) -> void;
        auto uninstall_debug_hook(lua_State* L) -> void;
        auto has_debug_hook(lua_State* L) const -> bool;

        // Script loading
        auto load_script(const std::string& path) -> const LuaScriptFile*;
        auto get_mod_scripts(lua_State* L) -> std::vector<std::string>;
        auto save_script(const std::string& path, const std::string& content) -> bool;
        auto reload_mod_for_state(lua_State* L) -> void;

        // REPL
        auto execute_repl(lua_State* L, const std::string& code) -> void;

        // Table inspection
        static auto get_table_children(lua_State* L, const std::string& path, int depth = 0) -> std::vector<LuaValueNode>;
        auto request_table_expand(const std::string& path) -> void;

        // GUI rendering
        auto render() -> void;

    private:
        auto render_state_list() -> void;
        auto render_stack_view() -> void;
        auto render_globals_view() -> void;
        auto render_error_log() -> void;
        auto render_error_details(const LuaErrorRecord& error) -> void;
        auto render_call_stack(const std::vector<LuaCallStackEntry>& call_stack) -> void;
        auto render_controls() -> void;
        auto render_debug_view() -> void;
        auto render_script_editor() -> void;
        auto render_mods_tab() -> void;
        auto render_breakpoints_panel() -> void;
        auto render_repl() -> void;
        auto render_value_tree(std::vector<LuaValueNode>& nodes) -> void;
        auto render_debug_controls() -> void;

        auto find_mod_name_for_state(lua_State* L) const -> std::string;
        auto request_globals_refresh() -> void;
        auto request_loaded_modules_refresh() -> void;
        auto check_breakpoint(lua_State* L, lua_Debug* ar) -> bool;
        auto wait_for_continue() -> void;
        auto get_call_depth(lua_State* L) -> int;

        // Filter for globals view
        std::string m_globals_filter;

        // Cached globals (since we can't access Lua from GUI thread directly)
        std::vector<std::pair<std::string, LuaStackSlot>> m_cached_globals;
        lua_State* m_cached_globals_state{nullptr};
        bool m_globals_refresh_requested{false};
        mutable std::mutex m_globals_mutex;

        // Tree view support for globals
        std::unordered_map<std::string, std::vector<std::pair<std::string, LuaStackSlot>>> m_globals_children_cache;
        std::set<std::string> m_pending_table_expansions;

        // Helper for recursive tree rendering
        auto render_globals_tree_node(const std::vector<std::pair<std::string, LuaStackSlot>>& children, const std::string& parent_path, int depth) -> void;

        // Helper to get table entries at a given path
        static auto get_table_entries_at_path(lua_State* L, const std::string& path) -> std::vector<std::pair<std::string, LuaStackSlot>>;

        // Mod management
        struct ModInfo
        {
            std::string name;
            std::filesystem::path path;
            bool enabled_via_txt{false};
            bool enabled_via_mods_txt{false};
            bool is_running{false};

            bool is_enabled() const { return enabled_via_txt || enabled_via_mods_txt; }
        };

        std::vector<ModInfo> m_discovered_mods;
        bool m_mods_list_dirty{true};
        std::string m_new_mod_name;
        bool m_show_create_mod_popup{false};
        std::string m_new_file_name;
        bool m_show_create_file_popup{false};
        bool m_add_require_to_main{true};
        std::filesystem::path m_create_file_mod_path;
        int m_pending_editor_tab_switch{-1};

        auto refresh_mods_list() -> void;
        auto create_new_mod(const std::string& name) -> bool;
        auto create_new_file(const std::string& mod_path, const std::string& filename, bool add_require_to_main) -> bool;
        auto set_mod_enabled(const std::filesystem::path& mod_path, bool enabled) -> void;
        auto restart_mod_by_name(const std::string& mod_name) -> void;
        auto uninstall_mod_by_name(const std::string& mod_name) -> void;
        auto start_mod_by_path(const std::filesystem::path& mod_path) -> void;
    };

} // namespace RC::GUI