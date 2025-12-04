#include <GUI/LuaDebugger.hpp>

#include <algorithm>
#include <bit>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <set>
#include <sstream>
#include <thread>

#include <DynamicOutput/DynamicOutput.hpp>
#include <GUI/GUI.hpp>
#include <GUI/ImGuiUtility.hpp>
#include <Helpers/String.hpp>
#include <LuaMadeSimple/LuaMadeSimple.hpp>
#include <LuaMadeSimple/LuaObject.hpp>
#include <LuaType/LuaUObject.hpp>
#include <LuaType/LuaXProperty.hpp>
#include <LuaType/LuaFName.hpp>
#include <LuaType/LuaFText.hpp>
#include <LuaType/LuaUnrealString.hpp>
#include <LuaType/LuaTArray.hpp>
#include <Mod/LuaMod.hpp>
#include <UE4SSProgram.hpp>
#include <Unreal/CoreUObject/UObject/UnrealType.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/FText.hpp>

#include <lua.hpp>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <IconsFontAwesome5.h>

namespace RC::GUI
{
    LuaDebugger* LuaDebugger::s_instance = nullptr;

    static auto format_stack_value_safe(lua_State* L, int index, size_t max_length) -> std::string;

    static auto get_ue4ss_userdata_info(lua_State* L, int index, const std::string& metatable_name) -> std::string
    {
        try
        {
            // For UObject-derived types, get the full name
            if (metatable_name == "UObject" || metatable_name == "AActor" ||
                metatable_name == "UClass" || metatable_name == "UStruct" ||
                metatable_name == "UScriptStruct" || metatable_name == "UFunction" ||
                metatable_name == "UEnum" || metatable_name == "UWorld" ||
                metatable_name == "UInterface")
            {
                auto& lua_object = *static_cast<LuaType::UObject*>(lua_touserdata(L, index));
                auto* obj = lua_object.get_remote_cpp_object();
                if (obj)
                {
                    return to_string(obj->GetFullName());
                }
                return "(null)";
            }
            // For Property types, show the property class type and name
            else if (metatable_name == "Property" || metatable_name.find("Property") != std::string::npos)
            {
                auto& lua_object = *static_cast<LuaType::XProperty*>(lua_touserdata(L, index));
                auto* prop = lua_object.get_remote_cpp_object();
                if (prop)
                {
                    std::string result;
                    // Get the property class name (like IntProperty, StrProperty, etc.)
                    auto field_class = prop->GetClass();
                    std::string class_name = to_string(field_class.GetFName().ToString());
                    // Remove the leading "F" if present
                    if (class_name.length() > 1 && class_name[0] == 'F')
                    {
                        class_name = class_name.substr(1);
                    }
                    result = class_name + " ";

                    // Get the property name
                    result += to_string(prop->GetFName().ToString());
                    return result;
                }
                return "(null)";
            }
            // For FName, get the string value
            else if (metatable_name == "FName")
            {
                auto& lua_object = *static_cast<LuaType::FName*>(lua_touserdata(L, index));
                Unreal::FName& fname = lua_object.get_local_cpp_object();
                return "\"" + to_string(fname.ToString()) + "\"";
            }
            // For FText, get the string value
            else if (metatable_name == "FText")
            {
                auto& lua_object = *static_cast<LuaType::FText*>(lua_touserdata(L, index));
                Unreal::FText& ftext = lua_object.get_local_cpp_object();
                return "\"" + to_string(ftext.ToString()) + "\"";
            }
            // For FString/UnrealString, get the string value
            else if (metatable_name == "FString")
            {
                auto& lua_object = *static_cast<LuaType::FString*>(lua_touserdata(L, index));
                Unreal::FString& fstr = lua_object.get_local_cpp_object();
                return "\"" + to_string(*fstr) + "\"";
            }
            // For TArray, get the count and inner type if possible
            else if (metatable_name == "TArray")
            {
                auto& lua_object = *static_cast<LuaType::TArray*>(lua_touserdata(L, index));
                auto* arr = lua_object.get_remote_cpp_object();
                if (arr)
                {
                    return "count=" + std::to_string(arr->Num());
                }
                return "(null)";
            }
        }
        catch (...)
        {
        }
        return "";
    }

    static auto get_uobject_from_userdata(lua_State* L, int index, const std::string& metatable_name) -> Unreal::UObject*
    {
        try
        {
            if (metatable_name == "UObject" || metatable_name == "AActor" ||
                metatable_name == "UClass" || metatable_name == "UStruct" ||
                metatable_name == "UScriptStruct" || metatable_name == "UFunction" ||
                metatable_name == "UEnum" || metatable_name == "UWorld" ||
                metatable_name == "UInterface")
            {
                auto& lua_object = *static_cast<LuaType::UObject*>(lua_touserdata(L, index));
                return lua_object.get_remote_cpp_object();
            }
        }
        catch (...)
        {
        }
        return nullptr;
    }

    static auto lua_error_callback(lua_State* L, const std::string& error_message, const std::string& traceback) -> void
    {
        if (LuaDebugger::has_instance())
        {
            LuaDebugger::get().record_error(L, error_message, traceback);
        }
    }

    LuaDebugger::LuaDebugger()
    {
        s_instance = this;
        LuaMadeSimple::register_error_callback(lua_error_callback);
        m_script_editor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
        m_script_editor.SetShowWhitespaces(false);
    }

    LuaDebugger::~LuaDebugger()
    {
        LuaMadeSimple::unregister_error_callback(lua_error_callback);

        if (s_instance == this)
        {
            s_instance = nullptr;
        }
    }

    auto LuaDebugger::get() -> LuaDebugger&
    {
        static LuaDebugger instance;
        return instance;
    }

    auto LuaDebugger::has_instance() -> bool
    {
        return s_instance != nullptr;
    }

    auto LuaDebugger::register_lua_state(lua_State* L, const std::string& mod_name, const std::string& state_type) -> void
    {
        std::lock_guard<std::mutex> lock(m_states_mutex);

        LuaStateInfo info;
        info.lua_state = L;
        info.mod_name = mod_name;
        info.state_type = state_type;
        info.is_active = true;

        m_lua_states[L] = std::move(info);

        // Auto-restore debug hook if this mod had debug enabled before reload
        if (m_debug_enabled_mods.count(mod_name) > 0)
        {
            install_debug_hook(L);
        }
    }

    auto LuaDebugger::unregister_lua_state(lua_State* L) -> void
    {
        std::lock_guard<std::mutex> lock(m_states_mutex);
        m_lua_states.erase(L);
    }

    auto LuaDebugger::update_state_stack(lua_State* L) -> void
    {
        std::lock_guard<std::mutex> lock(m_states_mutex);

        auto it = m_lua_states.find(L);
        if (it != m_lua_states.end())
        {
            it->second.current_stack_size = lua_gettop(L);
            it->second.current_stack = get_stack_slots(L);
        }
    }

    auto LuaDebugger::record_error(lua_State* L, const std::string& error_message, const std::string& traceback) -> void
    {
        LuaErrorRecord record;
        record.timestamp = std::chrono::system_clock::now();
        record.mod_name = find_mod_name_for_state(L);
        record.error_message = error_message;
        record.traceback = traceback;
        record.call_stack = get_call_stack(L);
        record.stack_snapshot = get_stack_slots(L);

        {
            std::lock_guard<std::mutex> lock(m_errors_mutex);
            m_error_history.push_back(std::move(record));

            // Limit history size
            while (m_error_history.size() > MAX_ERROR_HISTORY)
            {
                m_error_history.pop_front();
            }

            m_selected_error_index = static_cast<int>(m_error_history.size()) - 1;
        }

        if (m_pause_on_error && has_debug_hook(L))
        {
            m_paused_state = L;
            m_paused_line = 0;
            m_paused_source = "";

            if (!record.call_stack.empty())
            {
                m_paused_line = record.call_stack[0].current_line;
                m_paused_source = record.call_stack[0].source;
            }

            m_is_paused.store(true);
            wait_for_continue();
            m_is_paused.store(false);
            m_paused_state = nullptr;
            m_continue_requested.store(false);
        }
    }

    auto LuaDebugger::clear_error_history() -> void
    {
        std::lock_guard<std::mutex> lock(m_errors_mutex);
        m_error_history.clear();
        m_selected_error_index = -1;
    }

    auto LuaDebugger::get_error_count() const -> size_t
    {
        std::lock_guard<std::mutex> lock(m_errors_mutex);
        return m_error_history.size();
    }

    auto LuaDebugger::get_stack_slots(lua_State* L) -> std::vector<LuaStackSlot>
    {
        std::vector<LuaStackSlot> slots;

        if (!L)
        {
            return slots;
        }

        int top = lua_gettop(L);
        slots.reserve(top);

        // IMPORTANT: Use format_stack_value_safe to avoid corrupting the Lua stack
        // during the debug hook. format_stack_value pushes/pops which can cause crashes.
        for (int i = 1; i <= top; ++i)
        {
            LuaStackSlot slot;
            slot.index = i;
            slot.type_name = luaL_typename(L, i);
            slot.is_table = lua_istable(L, i);
            slot.is_userdata = lua_isuserdata(L, i);
            slot.value_preview = format_stack_value_safe(L, i, MAX_STACK_PREVIEW_LENGTH);
            slots.push_back(std::move(slot));
        }

        return slots;
    }

    auto LuaDebugger::get_call_stack(lua_State* L) -> std::vector<LuaCallStackEntry>
    {
        std::vector<LuaCallStackEntry> stack;

        if (!L)
        {
            return stack;
        }

        lua_Debug ar;
        int level = 0;

        while (lua_getstack(L, level, &ar))
        {
            if (lua_getinfo(L, "Slntu", &ar))
            {
                LuaCallStackEntry entry;

                if (ar.name)
                {
                    entry.function_name = ar.name;
                }
                else
                {
                    entry.function_name = "(anonymous)";
                }

                if (ar.source)
                {
                    entry.source = ar.source;
                    if (!entry.source.empty() && entry.source[0] == '@')
                    {
                        entry.source = entry.source.substr(1);
                    }
                }

                entry.line_number = ar.linedefined;
                entry.current_line = ar.currentline;
                entry.what = ar.what ? ar.what : "";
                entry.name_what = ar.namewhat ? ar.namewhat : "";

                stack.push_back(std::move(entry));
            }
            ++level;
        }

        return stack;
    }

    auto LuaDebugger::get_stack_frames_with_locals(lua_State* L) -> std::vector<LuaStackFrame>
    {
        std::vector<LuaStackFrame> frames;

        if (!L)
        {
            return frames;
        }

        lua_Debug ar;
        int level = 0;

        while (lua_getstack(L, level, &ar))
        {
            if (lua_getinfo(L, "Slntu", &ar))
            {
                LuaStackFrame frame;
                frame.function_name = ar.name ? ar.name : "(anonymous)";
                if (ar.source)
                {
                    frame.source = ar.source;
                    if (!frame.source.empty() && frame.source[0] == '@')
                    {
                        frame.source = frame.source.substr(1);
                    }
                }
                frame.line_number = ar.linedefined;
                frame.current_line = ar.currentline;
                frame.what = ar.what ? ar.what : "";

                // Get local variables for this frame
                // IMPORTANT: Use format_stack_value_safe to avoid corrupting the Lua stack
                // during the debug hook. format_stack_value pushes/pops which can cause crashes.
                int local_index = 1;
                const char* local_name;
                while ((local_name = lua_getlocal(L, &ar, local_index)) != nullptr)
                {
                    LuaLocalVariable local;
                    local.name = local_name;
                    local.type_name = luaL_typename(L, -1);
                    local.is_table = lua_istable(L, -1);
                    local.is_userdata = lua_isuserdata(L, -1);
                    local.value_preview = format_stack_value_safe(L, -1, MAX_STACK_PREVIEW_LENGTH);

                    frame.locals.push_back(std::move(local));
                    lua_pop(L, 1); // Pop the local value
                    ++local_index;
                }

                frames.push_back(std::move(frame));
            }
            ++level;
        }

        return frames;
    }

    // Safe to call during lua_next loops - doesn't iterate tables internally
    static auto format_stack_value_safe(lua_State* L, int index, size_t max_length) -> std::string
    {
        if (!L)
        {
            return "<invalid state>";
        }

        int type = lua_type(L, index);
        std::string result;

        switch (type)
        {
        case LUA_TNIL:
            result = "nil";
            break;

        case LUA_TBOOLEAN:
            result = lua_toboolean(L, index) ? "true" : "false";
            break;

        case LUA_TNUMBER:
            if (lua_isinteger(L, index))
            {
                result = std::to_string(lua_tointeger(L, index));
            }
            else
            {
                std::ostringstream oss;
                oss << std::setprecision(6) << lua_tonumber(L, index);
                result = oss.str();
            }
            break;

        case LUA_TSTRING: {
            size_t len;
            const char* str = lua_tolstring(L, index, &len);
            if (str)
            {
                result = "\"";
                result.append(str, len);
                result += "\"";
            }
            else
            {
                result = "<null string>";
            }
            break;
        }

        case LUA_TTABLE: {
            // lua_rawlen gives us array length without pushing anything
            // For hash part we'd need to iterate, but this is a reasonable approximation
            size_t len = lua_rawlen(L, index);
            if (len > 0)
            {
                result = "table[" + std::to_string(len) + "]";
            }
            else
            {
                // Could be empty or a pure hash table - just show "table"
                result = "table";
            }
            break;
        }

        case LUA_TFUNCTION: {
            // lua_iscfunction doesn't push anything
            if (lua_iscfunction(L, index))
            {
                result = "C function";
            }
            else
            {
                result = "Lua function";
            }
            break;
        }

        case LUA_TUSERDATA: {
            // lua_touserdata doesn't push anything
            void* ptr = lua_touserdata(L, index);

            // For UE4SS userdata, we can directly cast and get info without Lua stack manipulation
            // Try common UE4SS types by checking the memory layout
            std::string rich_info;

            // Try to get rich info using C++ direct access
            // We'll try each known type - get_ue4ss_userdata_info handles this
            // But we need the metatable name first, which requires stack manipulation
            // So we do minimal stack work with immediate restore
            std::string metatable_name;
            int saved_top = lua_gettop(L);
            if (lua_getmetatable(L, index))
            {
                lua_pushstring(L, "__name");
                if (lua_rawget(L, -2) == LUA_TSTRING)
                {
                    metatable_name = lua_tostring(L, -1);
                }
            }
            lua_settop(L, saved_top);

            if (!metatable_name.empty())
            {
                rich_info = get_ue4ss_userdata_info(L, index, metatable_name);
            }

            std::ostringstream oss;
            if (!metatable_name.empty())
            {
                oss << metatable_name;
                if (!rich_info.empty())
                {
                    oss << ": " << rich_info;
                }
                else
                {
                    oss << "(" << ptr << ")";
                }
            }
            else
            {
                oss << "userdata(" << ptr << ")";
            }
            result = oss.str();
            break;
        }

        case LUA_TLIGHTUSERDATA: {
            void* ptr = lua_touserdata(L, index);
            std::ostringstream oss;
            oss << "lightuserdata(" << ptr << ")";
            result = oss.str();
            break;
        }

        case LUA_TTHREAD: {
            lua_State* thread = lua_tothread(L, index);
            std::ostringstream oss;
            oss << "thread(" << (void*)thread << ")";
            result = oss.str();
            break;
        }

        default:
            result = "<unknown type>";
            break;
        }

        return result;
    }

    auto LuaDebugger::format_stack_value(lua_State* L, int index, size_t max_length) -> std::string
    {
        if (!L)
        {
            return "<invalid state>";
        }

        int type = lua_type(L, index);
        std::string result;

        switch (type)
        {
        case LUA_TNIL:
            result = "nil";
            break;

        case LUA_TBOOLEAN:
            result = lua_toboolean(L, index) ? "true" : "false";
            break;

        case LUA_TNUMBER:
            if (lua_isinteger(L, index))
            {
                result = std::to_string(lua_tointeger(L, index));
            }
            else
            {
                std::ostringstream oss;
                oss << std::setprecision(6) << lua_tonumber(L, index);
                result = oss.str();
            }
            break;

        case LUA_TSTRING: {
            size_t len;
            const char* str = lua_tolstring(L, index, &len);
            if (str)
            {
                result = "\"";
                result.append(str, len);
                result += "\"";
            }
            else
            {
                result = "<null string>";
            }
            break;
        }

        case LUA_TTABLE: {
            // Count table entries (limited)
            // NOTE: This uses lua_next which can be dangerous if called during another lua_next loop
            // Use format_stack_value_safe() when iterating tables
            int count = 0;
            lua_pushnil(L);
            while (lua_next(L, index < 0 ? index - 1 : index) != 0 && count < 10)
            {
                lua_pop(L, 1); // pop value, keep key
                ++count;
            }
            // If we didn't exhaust the table, pop the remaining key
            if (count >= 10)
            {
                lua_pop(L, 1);
            }
            result = "table(" + std::to_string(count) + (count >= 10 ? "+" : "") + " entries)";
            break;
        }

        case LUA_TFUNCTION: {
            lua_Debug ar;
            lua_pushvalue(L, index);
            if (lua_getinfo(L, ">S", &ar))
            {
                if (ar.what && strcmp(ar.what, "C") == 0)
                {
                    result = "C function";
                }
                else if (ar.source)
                {
                    std::string src = ar.source;
                    if (src.length() > 30)
                    {
                        src = "..." + src.substr(src.length() - 27);
                    }
                    result = "function@" + src + ":" + std::to_string(ar.linedefined);
                }
                else
                {
                    result = "Lua function";
                }
            }
            else
            {
                result = "function";
            }
            break;
        }

        case LUA_TUSERDATA: {
            void* ptr = lua_touserdata(L, index);
            std::string metatable_name;

            // Try to get metatable name for more info
            if (lua_getmetatable(L, index))
            {
                lua_pushstring(L, "__name");
                if (lua_rawget(L, -2) == LUA_TSTRING)
                {
                    metatable_name = lua_tostring(L, -1);
                }
                lua_pop(L, 2); // pop name and metatable
            }
            
            std::string rich_info = get_ue4ss_userdata_info(L, index, metatable_name);
            
            std::ostringstream oss;
            if (!metatable_name.empty())
            {
                oss << metatable_name;
                if (!rich_info.empty())
                {
                    oss << ": " << rich_info;
                }
                else
                {
                    oss << "(" << ptr << ")";
                }
            }
            else
            {
                oss << "userdata(" << ptr << ")";
            }
            result = oss.str();
            break;
        }

        case LUA_TLIGHTUSERDATA: {
            void* ptr = lua_touserdata(L, index);
            std::ostringstream oss;
            oss << "lightuserdata(" << ptr << ")";
            result = oss.str();
            break;
        }

        case LUA_TTHREAD: {
            lua_State* thread = lua_tothread(L, index);
            std::ostringstream oss;
            oss << "thread(" << (void*)thread << ")";
            result = oss.str();
            break;
        }

        default:
            result = "<unknown type>";
            break;
        }

        return result;
    }

    auto LuaDebugger::get_enhanced_traceback(lua_State* L, const std::string& message, int level) -> std::string
    {
        if (!L)
        {
            return message + "\n<no Lua state>";
        }

        std::ostringstream oss;
        oss << message << "\n\nCall Stack:\n";

        lua_Debug ar;
        int current_level = level;

        while (lua_getstack(L, current_level, &ar))
        {
            if (lua_getinfo(L, "Slntu", &ar))
            {
                oss << "  [" << current_level << "] ";

                // Function name or description
                if (ar.name)
                {
                    oss << ar.name;
                    if (ar.namewhat && ar.namewhat[0])
                    {
                        oss << " (" << ar.namewhat << ")";
                    }
                }
                else if (ar.what)
                {
                    if (strcmp(ar.what, "main") == 0)
                    {
                        oss << "<main chunk>";
                    }
                    else if (strcmp(ar.what, "C") == 0)
                    {
                        oss << "<C function>";
                    }
                    else if (strcmp(ar.what, "tail") == 0)
                    {
                        oss << "<tail call>";
                    }
                    else
                    {
                        oss << "<" << ar.what << ">";
                    }
                }
                else
                {
                    oss << "<unknown>";
                }

                // Source location
                if (ar.source)
                {
                    std::string src = ar.source;
                    // Remove @ prefix
                    if (!src.empty() && src[0] == '@')
                    {
                        src = src.substr(1);
                    }

                    oss << "\n      at " << src;
                    if (ar.currentline > 0)
                    {
                        oss << ":" << ar.currentline;
                    }
                    else if (ar.linedefined > 0)
                    {
                        oss << ":" << ar.linedefined;
                    }
                }

                oss << "\n";
            }
            ++current_level;
        }

        // Add stack snapshot
        int top = lua_gettop(L);
        if (top > 0)
        {
            oss << "\nStack Snapshot (" << top << " values):\n";
            for (int i = top; i >= 1; --i)
            {
                oss << "  [" << i << "] " << luaL_typename(L, i) << ": " << format_stack_value(L, i, 60) << "\n";
            }
        }

        return oss.str();
    }

    auto LuaDebugger::get_globals(lua_State* L, size_t max_entries) -> std::vector<std::pair<std::string, LuaStackSlot>>
    {
        std::vector<std::pair<std::string, LuaStackSlot>> globals;

        if (!L)
        {
            return globals;
        }

        // Save the current stack top to restore later
        int original_top = lua_gettop(L);

        try
        {
            // Push the global table onto the stack
            lua_pushglobaltable(L);

            if (!lua_istable(L, -1))
            {
                lua_settop(L, original_top);
                return globals;
            }

            // Iterate through the global table
            lua_pushnil(L); // first key
            size_t count = 0;

            while (lua_next(L, -2) != 0 && count < max_entries)
            {
                // Key is at -2, value is at -1
                if (lua_type(L, -2) == LUA_TSTRING)
                {
                    const char* key_str = lua_tostring(L, -2);
                    if (key_str)
                    {
                        std::string key = key_str;

                        // Skip internal/system globals that start with underscore
                        if (!key.empty() && key[0] != '_')
                        {
                            LuaStackSlot slot;
                            slot.index = 0; // Not a stack index
                            slot.type_name = luaL_typename(L, -1);
                            slot.is_table = lua_istable(L, -1);
                            slot.is_userdata = lua_isuserdata(L, -1);
                            // IMPORTANT: Use safe version that doesn't call lua_next internally
                            // Calling lua_next during another lua_next loop corrupts iteration
                            slot.value_preview = format_stack_value_safe(L, -1, MAX_STACK_PREVIEW_LENGTH);

                            globals.emplace_back(key, slot);
                            ++count;
                        }
                    }
                }

                lua_pop(L, 1); // pop value, keep key for next iteration
            }

            // Restore the stack
            lua_settop(L, original_top);
        }
        catch (...)
        {
            // If anything goes wrong, restore the stack and return what we have
            lua_settop(L, original_top);
        }

        std::sort(globals.begin(),
                  globals.end(),
                  [](const auto& a, const auto& b) {
                      return a.first < b.first;
                  });

        return globals;
    }

    auto LuaDebugger::get_table_children(lua_State* L, const std::string& path, int depth) -> std::vector<LuaValueNode>
    {
        std::vector<LuaValueNode> children;

        if (!L || depth > static_cast<int>(MAX_TABLE_DEPTH))
        {
            return children;
        }

        int original_top = lua_gettop(L);

        try
        {
            // Navigate to the table at the given path
            // Path format: "globalName" or "globalName.key1.key2"
            if (path.empty())
            {
                lua_pushglobaltable(L);
            }
            else
            {
                // Parse the path and navigate
                lua_pushglobaltable(L);

                std::string remaining = path;
                while (!remaining.empty())
                {
                    size_t dot_pos = remaining.find('.');
                    std::string key = (dot_pos != std::string::npos) ? remaining.substr(0, dot_pos) : remaining;
                    remaining = (dot_pos != std::string::npos) ? remaining.substr(dot_pos + 1) : "";

                    // Try to get the field
                    lua_pushstring(L, key.c_str());
                    int field_type = lua_gettable(L, -2);
                    lua_remove(L, -2); // Remove the parent table

                    if (field_type == LUA_TNIL)
                    {
                        lua_settop(L, original_top);
                        return children;
                    }
                }
            }

            if (!lua_istable(L, -1))
            {
                lua_settop(L, original_top);
                return children;
            }

            // Iterate the table
            lua_pushnil(L);
            size_t count = 0;
            const size_t max_children = 200;

            while (lua_next(L, -2) != 0 && count < max_children)
            {
                LuaValueNode node;
                node.depth = depth;

                // Get the key
                int key_type = lua_type(L, -2);
                if (key_type == LUA_TSTRING)
                {
                    node.key = lua_tostring(L, -2);
                }
                else if (key_type == LUA_TNUMBER)
                {
                    if (lua_isinteger(L, -2))
                    {
                        node.key = "[" + std::to_string(lua_tointeger(L, -2)) + "]";
                    }
                    else
                    {
                        node.key = "[" + std::to_string(lua_tonumber(L, -2)) + "]";
                    }
                }
                else
                {
                    node.key = "[" + std::string(luaL_typename(L, -2)) + "]";
                }

                // Build path
                if (path.empty())
                {
                    node.path = node.key;
                }
                else if (key_type == LUA_TNUMBER)
                {
                    node.path = path + node.key; // Already has brackets
                }
                else
                {
                    node.path = path + "." + node.key;
                }

                // Get value info
                node.type_name = luaL_typename(L, -1);
                node.is_expandable = lua_istable(L, -1);
                node.value_preview = format_stack_value_safe(L, -1, MAX_STACK_PREVIEW_LENGTH);

                // Skip internal keys starting with _
                if (node.key.empty() || node.key[0] != '_')
                {
                    children.push_back(std::move(node));
                    ++count;
                }

                lua_pop(L, 1); // Pop value, keep key
            }

            lua_settop(L, original_top);
        }
        catch (...)
        {
            lua_settop(L, original_top);
        }

        std::sort(children.begin(),
                  children.end(),
                  [](const auto& a, const auto& b) {
                      return a.key < b.key;
                  });

        return children;
    }

    auto LuaDebugger::request_table_expand(const std::string& path) -> void
    {
        if (!m_selected_state)
        {
            return;
        }

        // Toggle expansion
        if (m_expanded_paths.count(path))
        {
            m_expanded_paths.erase(path);
        }
        else
        {
            m_expanded_paths.insert(path);
        }

        m_tree_refresh_requested = true;
    }

    auto LuaDebugger::add_breakpoint(const std::string& source, int line, const std::string& condition) -> void
    {
        std::lock_guard<std::mutex> lock(m_breakpoints_mutex);

        for (auto& bp : m_breakpoints)
        {
            if (bp.source_file == source && bp.line == line)
            {
                bp.condition = condition;
                bp.enabled = true;
                return;
            }
        }

        LuaBreakpoint bp;
        bp.source_file = source;
        bp.line = line;
        bp.condition = condition;
        bp.enabled = true;
        m_breakpoints.push_back(std::move(bp));
    }

    auto LuaDebugger::remove_breakpoint(const std::string& source, int line) -> void
    {
        std::lock_guard<std::mutex> lock(m_breakpoints_mutex);

        m_breakpoints.erase(
                std::remove_if(m_breakpoints.begin(),
                               m_breakpoints.end(),
                               [&](const LuaBreakpoint& bp) {
                                   return bp.source_file == source && bp.line == line;
                               }),
                m_breakpoints.end());
    }

    auto LuaDebugger::toggle_breakpoint(const std::string& source, int line) -> void
    {
        std::lock_guard<std::mutex> lock(m_breakpoints_mutex);

        for (auto& bp : m_breakpoints)
        {
            if (bp.source_file == source && bp.line == line)
            {
                bp.enabled = !bp.enabled;
                return;
            }
        }

        LuaBreakpoint bp;
        bp.source_file = source;
        bp.line = line;
        bp.enabled = true;
        m_breakpoints.push_back(std::move(bp));
    }

    auto LuaDebugger::clear_all_breakpoints() -> void
    {
        std::lock_guard<std::mutex> lock(m_breakpoints_mutex);
        m_breakpoints.clear();
    }

    auto LuaDebugger::has_breakpoint(const std::string& source, int line) const -> bool
    {
        std::lock_guard<std::mutex> lock(m_breakpoints_mutex);

        for (const auto& bp : m_breakpoints)
        {
            if (bp.enabled && bp.source_file == source && bp.line == line)
            {
                return true;
            }
        }
        return false;
    }

    auto LuaDebugger::continue_execution() -> void
    {
        m_continue_requested.store(true);
        m_step_requested.store(false);
        m_step_over_requested.store(false);
        m_step_out_requested.store(false);
        m_pause_cv.notify_all();
    }

    auto LuaDebugger::step_into() -> void
    {
        m_step_requested.store(true);
        m_continue_requested.store(false);
        m_step_over_requested.store(false);
        m_step_out_requested.store(false);
        m_pause_cv.notify_all();
    }

    auto LuaDebugger::step_over() -> void
    {
        m_step_over_requested.store(true);
        m_continue_requested.store(false);
        m_step_requested.store(false);
        m_step_out_requested.store(false);
        m_pause_cv.notify_all();
    }

    auto LuaDebugger::step_out() -> void
    {
        m_step_out_requested.store(true);
        m_continue_requested.store(false);
        m_step_requested.store(false);
        m_step_over_requested.store(false);
        m_pause_cv.notify_all();
    }

    auto LuaDebugger::get_call_depth(lua_State* L) -> int
    {
        int depth = 0;
        lua_Debug ar;
        while (lua_getstack(L, depth, &ar))
        {
            ++depth;
        }
        return depth;
    }

    auto LuaDebugger::check_breakpoint(lua_State* L, lua_Debug* ar) -> bool
    {
        if (!ar->source)
        {
            return false;
        }

        std::string source = ar->source;
        if (!source.empty() && source[0] == '@')
        {
            source = source.substr(1);
        }

        std::lock_guard<std::mutex> lock(m_breakpoints_mutex);

        for (auto& bp : m_breakpoints)
        {
            if (bp.enabled && ar->currentline == bp.line)
            {
                // Check if source matches (partial match for paths)
                if (source.find(bp.source_file) != std::string::npos ||
                    bp.source_file.find(source) != std::string::npos)
                {
                    ++bp.hit_count;
                    return true;
                }
            }
        }
        return false;
    }

    auto LuaDebugger::wait_for_continue() -> void
    {
        std::unique_lock<std::mutex> lock(m_pause_mutex);
        m_pause_cv.wait(lock,
                        [this]() {
                            return m_continue_requested.load() ||
                                   m_step_requested.load() ||
                                   m_step_over_requested.load() ||
                                   m_step_out_requested.load();
                        });
    }

    auto LuaDebugger::debug_hook(lua_State* L, lua_Debug* ar) -> void
    {
        if (!has_instance())
        {
            return;
        }

        auto& debugger = get();

        // Get debug info
        lua_getinfo(L, "Slnu", ar);

        // Check for breakpoint
        bool should_break = false;

        if (ar->event == LUA_HOOKLINE)
        {
            if (debugger.check_breakpoint(L, ar))
            {
                should_break = true;
            }
            
            if (debugger.m_step_requested.load())
            {
                should_break = true;
                debugger.m_step_requested.store(false);
            }

            if (debugger.m_step_over_requested.load())
            {
                int current_depth = debugger.get_call_depth(L);
                if (current_depth <= debugger.m_step_start_depth)
                {
                    should_break = true;
                    debugger.m_step_over_requested.store(false);
                }
            }

            if (debugger.m_step_out_requested.load())
            {
                int current_depth = debugger.get_call_depth(L);
                if (current_depth < debugger.m_step_start_depth)
                {
                    should_break = true;
                    debugger.m_step_out_requested.store(false);
                }
            }
        }

        if (should_break)
        {
            debugger.m_paused_state = L;
            debugger.m_paused_line = ar->currentline;
            debugger.m_paused_source = ar->source ? ar->source : "";
            debugger.m_step_start_depth = debugger.get_call_depth(L);
            debugger.m_script_scroll_to_line = ar->currentline;

            {
                std::lock_guard<std::mutex> lock(debugger.m_paused_data_mutex);
                debugger.m_paused_stack_slots = debugger.get_stack_slots(L);
                debugger.m_paused_call_stack = debugger.get_call_stack(L);
                debugger.m_paused_stack_frames = debugger.get_stack_frames_with_locals(L);
            }

            debugger.m_is_paused.store(true);
            debugger.m_continue_requested.store(false);
            debugger.wait_for_continue();
            debugger.m_is_paused.store(false);
            debugger.m_paused_state = nullptr;

            {
                std::lock_guard<std::mutex> lock(debugger.m_paused_data_mutex);
                debugger.m_paused_stack_slots.clear();
                debugger.m_paused_call_stack.clear();
                debugger.m_paused_stack_frames.clear();
            }
        }
    }

    auto LuaDebugger::install_debug_hook(lua_State* L) -> void
    {
        if (!L) return;
        lua_sethook(L, debug_hook, LUA_MASKLINE | LUA_MASKCALL | LUA_MASKRET, 0);
        m_states_with_hooks.insert(L);
    }

    auto LuaDebugger::uninstall_debug_hook(lua_State* L) -> void
    {
        if (!L) return;
        lua_sethook(L, nullptr, 0, 0);
        m_states_with_hooks.erase(L);
    }

    auto LuaDebugger::has_debug_hook(lua_State* L) const -> bool
    {
        return m_states_with_hooks.count(L) > 0;
    }

    auto LuaDebugger::load_script(const std::string& path) -> const LuaScriptFile*
    {
        std::lock_guard<std::mutex> lock(m_scripts_mutex);

        auto it = m_script_cache.find(path);
        if (it != m_script_cache.end() && it->second.loaded)
        {
            return &it->second;
        }

        LuaScriptFile script;
        script.path = path;

        std::ifstream file(path);
        if (!file.is_open())
        {
            // Try with @ prefix removed
            std::string clean_path = path;
            if (!clean_path.empty() && clean_path[0] == '@')
            {
                clean_path = clean_path.substr(1);
            }
            file.open(clean_path);
        }

        if (file.is_open())
        {
            std::stringstream buffer;
            buffer << file.rdbuf();
            script.content = buffer.str();

            std::istringstream stream(script.content);
            std::string line;
            while (std::getline(stream, line))
            {
                script.lines.push_back(line);
            }

            if (!script.content.empty())
            {
                size_t last_newline = script.content.rfind('\n');
                if (last_newline != std::string::npos && last_newline < script.content.size() - 1)
                {
                    script.lines.push_back(script.content.substr(last_newline + 1));
                }
            }

            script.loaded = true;
            m_script_cache[path] = std::move(script);
            return &m_script_cache[path];
        }

        return nullptr;
    }

    auto LuaDebugger::get_mod_scripts(lua_State* L) -> std::vector<std::string>
    {
        std::vector<std::string> scripts;
        std::set<std::string> seen_paths; // Avoid duplicates

        // Find the mod for this state
        for (const auto& mod : UE4SSProgram::get_program().m_mods)
        {
            auto* lua_mod = dynamic_cast<LuaMod*>(mod.get());
            if (lua_mod)
            {
                bool is_this_mod = false;
                if (lua_mod->m_main_lua && lua_mod->m_main_lua->get_lua_state() == L)
                    is_this_mod = true;
                if (lua_mod->m_hook_lua && lua_mod->m_hook_lua->get_lua_state() == L)
                    is_this_mod = true;
                if (lua_mod->m_async_lua && lua_mod->m_async_lua->get_lua_state() == L)
                    is_this_mod = true;

                if (is_this_mod)
                {
                    // Add scripts from the mod's directory
                    auto mod_path = lua_mod->get_path();
                    if (std::filesystem::exists(mod_path))
                    {
                        try
                        {
                            for (const auto& entry : std::filesystem::recursive_directory_iterator(mod_path))
                            {
                                if (entry.is_regular_file() && entry.path().extension() == ".lua")
                                {
                                    std::string path_str = entry.path().string();
                                    if (seen_paths.insert(path_str).second)
                                    {
                                        scripts.push_back(path_str);
                                    }
                                }
                            }
                        }
                        catch (const std::filesystem::filesystem_error&)
                        {
                            // Ignore permission errors
                        }
                    }

                    // Add cached loaded modules (captured when debug hook was installed)
                    {
                        std::lock_guard<std::mutex> lock(m_loaded_modules_mutex);
                        auto it = m_loaded_modules_cache.find(L);
                        if (it != m_loaded_modules_cache.end())
                        {
                            for (const auto& path : it->second)
                            {
                                if (seen_paths.insert(path).second)
                                {
                                    scripts.push_back(path);
                                }
                            }
                        }
                    }

                    break;
                }
            }
        }

        return scripts;
    }

    auto LuaDebugger::save_script(const std::string& path, const std::string& content) -> bool
    {
        std::string file_path = path;
        // Remove @ prefix if present
        if (!file_path.empty() && file_path[0] == '@')
        {
            file_path = file_path.substr(1);
        }

        std::ofstream file(file_path, std::ios::out | std::ios::trunc);
        if (!file.is_open())
        {
            Output::send<LogLevel::Error>(STR("Failed to save script: {}\n"), ensure_str(file_path));
            return false;
        }

        file << content;
        file.close();

        // Invalidate the cache so it reloads
        {
            std::lock_guard<std::mutex> lock(m_scripts_mutex);
            m_script_cache.erase(path);
        }

        Output::send<LogLevel::Normal>(STR("Saved script: {}\n"), ensure_str(file_path));
        return true;
    }

    auto LuaDebugger::reload_mod_for_state(lua_State* L) -> void
    {
        if (!L)
        {
            return;
        }

        // Find the mod for this state and restart it
        for (auto& mod : UE4SSProgram::get_program().m_mods)
        {
            auto* lua_mod = dynamic_cast<LuaMod*>(mod.get());
            if (lua_mod)
            {
                bool is_this_mod = false;
                if (lua_mod->m_main_lua && lua_mod->m_main_lua->get_lua_state() == L)
                    is_this_mod = true;
                if (lua_mod->m_hook_lua && lua_mod->m_hook_lua->get_lua_state() == L)
                    is_this_mod = true;
                if (lua_mod->m_async_lua && lua_mod->m_async_lua->get_lua_state() == L)
                    is_this_mod = true;

                if (is_this_mod)
                {
                    std::string mod_name = to_string(lua_mod->get_name());
                    Output::send<LogLevel::Normal>(STR("Reloading mod: {}\n"), ensure_str(mod_name));

                    // Uninstall and reinstall the mod
                    // Queue this on the game thread to be safe
                    UE4SSProgram::get_program().queue_event([](void* data) {
                                                                auto* mod = static_cast<LuaMod*>(data);
                                                                mod->uninstall();
                                                                mod->start_mod();
                                                            },
                                                            lua_mod);
                    break;
                }
            }
        }
    }

    auto LuaDebugger::execute_repl(lua_State* L, const std::string& code) -> void
    {
        if (!L || code.empty())
        {
            return;
        }

        // Queue execution on game thread
        struct ReplData
        {
            LuaDebugger* debugger;
            lua_State* L;
            std::string code;
        };

        auto* data = new ReplData{this, L, code};

        UE4SSProgram::get_program().queue_event(
                [](void* userdata) {
                    auto* repl_data = static_cast<ReplData*>(userdata);
                    auto* debugger = repl_data->debugger;
                    lua_State* L = repl_data->L;
                    const std::string& code = repl_data->code;

                    std::string result;
                    bool is_error = false;

                    int original_top = lua_gettop(L);

                    // Try to execute as expression first (return value)
                    std::string expr_code = "return " + code;
                    int load_result = luaL_loadstring(L, expr_code.c_str());

                    if (load_result != LUA_OK)
                    {
                        lua_pop(L, 1); // Pop error message
                        // Try as statement
                        load_result = luaL_loadstring(L, code.c_str());
                    }

                    if (load_result == LUA_OK)
                    {
                        int call_result = lua_pcall(L, 0, LUA_MULTRET, 0);
                        if (call_result == LUA_OK)
                        {
                            // Collect return values
                            int num_results = lua_gettop(L) - original_top;
                            if (num_results > 0)
                            {
                                std::ostringstream oss;
                                for (int i = 1; i <= num_results; ++i)
                                {
                                    if (i > 1)
                                        oss << ", ";
                                    oss << format_stack_value(L, original_top + i, 256);
                                }
                                result = oss.str();
                            }
                            else
                            {
                                result = "(no return value)";
                            }
                        }
                        else
                        {
                            result = lua_tostring(L, -1);
                            is_error = true;
                        }
                    }
                    else
                    {
                        result = lua_tostring(L, -1);
                        is_error = true;
                    }

                    lua_settop(L, original_top);

                    // Store result
                    {
                        std::lock_guard<std::mutex> lock(debugger->m_repl_mutex);
                        ReplHistoryEntry entry;
                        entry.input = code;
                        entry.output = result;
                        entry.is_error = is_error;
                        entry.timestamp = std::chrono::system_clock::now();
                        debugger->m_repl_history.push_back(std::move(entry));

                        while (debugger->m_repl_history.size() > MAX_REPL_HISTORY)
                        {
                            debugger->m_repl_history.pop_front();
                        }

                        debugger->m_repl_pending = false;
                    }

                    delete repl_data;
                },
                data);

        m_repl_pending = true;
    }

    auto LuaDebugger::find_mod_name_for_state(lua_State* L) const -> std::string
    {
        std::lock_guard<std::mutex> lock(m_states_mutex);

        auto it = m_lua_states.find(L);
        if (it != m_lua_states.end())
        {
            return it->second.mod_name;
        }

        // Try to find it in program mods
        for (const auto& mod : UE4SSProgram::get_program().m_mods)
        {
            auto* lua_mod = dynamic_cast<LuaMod*>(mod.get());
            if (lua_mod)
            {
                if (lua_mod->get_lua_state() == L)
                {
                    return to_string(lua_mod->get_name());
                }
                if (lua_mod->m_main_lua && lua_mod->m_main_lua->get_lua_state() == L)
                {
                    return to_string(lua_mod->get_name()) + " (main)";
                }
                if (lua_mod->m_hook_lua && lua_mod->m_hook_lua->get_lua_state() == L)
                {
                    return to_string(lua_mod->get_name()) + " (hook)";
                }
                if (lua_mod->m_async_lua && lua_mod->m_async_lua->get_lua_state() == L)
                {
                    return to_string(lua_mod->get_name()) + " (async)";
                }
            }
        }

        return "<unknown>";
    }

    auto LuaDebugger::render() -> void
    {
        render_controls();

        // Show debug controls if paused
        if (m_is_paused.load())
        {
            render_debug_controls();
        }

        ImGui::Separator();
        
        float panel_height = ImGui::GetContentRegionAvail().y - scaled(31.0f) - scaled(8.0f);

        // Split layout: left side for state list, right side for details
        float left_panel_width = std::max(scaled(150.0f), ImGui::GetContentRegionAvail().x * 0.2f);
        left_panel_width = std::min(left_panel_width, scaled(250.0f));

        // Left panel - State List and Breakpoints
        ImGui::BeginChild("LeftPanel", ImVec2(left_panel_width, panel_height), true, ImGuiWindowFlags_HorizontalScrollbar);

        if (ImGui::CollapsingHeader(ICON_FA_CODE " Lua States", ImGuiTreeNodeFlags_DefaultOpen))
        {
            render_state_list();
        }

        if (ImGui::CollapsingHeader(ICON_FA_MAP_MARKER_ALT " Breakpoints"))
        {
            render_breakpoints_panel();
        }

        ImGui::EndChild();

        ImGui::SameLine();

        // Right panel - Tabs
        ImGui::BeginChild("RightPanel", ImVec2(scaled(-16.0f), panel_height), false);

        if (ImGui::BeginTabBar("LuaDebuggerTabs"))
        {
            if (ImGui::BeginTabItem(ICON_FA_BUG " Debug"))
            {
                render_debug_view();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem(ICON_FA_GLOBE " Globals"))
            {
                render_globals_view();
                ImGui::EndTabItem();
            }

            ImGuiTabItemFlags editor_flags = 0;
            if (m_pending_editor_tab_switch > 0)
            {
                editor_flags = ImGuiTabItemFlags_SetSelected;
                m_pending_editor_tab_switch = 0;
            }
            if (ImGui::BeginTabItem(ICON_FA_EDIT " Script Editor", nullptr, editor_flags))
            {
                render_script_editor();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem(ICON_FA_TERMINAL " REPL"))
            {
                render_repl();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem(ICON_FA_EXCLAMATION_TRIANGLE " Errors"))
            {
                render_error_log();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem(ICON_FA_CUBES " Mods"))
            {
                render_mods_tab();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::EndChild();
    }

    auto LuaDebugger::render_controls() -> void
    {
        if (ImGui::Button(ICON_FA_SYNC " Refresh States"))
        {
            // Refresh all registered states
            std::lock_guard<std::mutex> lock(m_states_mutex);
            for (auto& [L, info] : m_lua_states)
            {
                info.current_stack_size = lua_gettop(L);
                info.current_stack = get_stack_slots(L);
            }
        }

        ImGui::SameLine();

        if (ImGui::Button(ICON_FA_TRASH " Clear Errors"))
        {
            clear_error_history();
        }

        ImGui::SameLine();

        ImGui::Checkbox("Auto-scroll", &m_auto_scroll_errors);

        ImGui::SameLine();

        ImGui::Checkbox("Pause on Error", &m_pause_on_error);

        // Show total error count
        ImGui::SameLine();
        ImGui::TextDisabled("| Errors: %zu", get_error_count());
    }

    auto LuaDebugger::render_state_list() -> void
    {
        ImGui::Text(ICON_FA_CODE " Lua States");
        ImGui::Separator();

        // Gather all states from program mods dynamically
        std::vector<std::pair<lua_State*, std::string>> all_states;

        for (const auto& mod : UE4SSProgram::get_program().m_mods)
        {
            auto* lua_mod = dynamic_cast<LuaMod*>(mod.get());
            if (lua_mod && lua_mod->is_started())
            {
                std::string mod_name = to_string(lua_mod->get_name());

                if (lua_mod->m_main_lua)
                {
                    all_states.emplace_back(lua_mod->m_main_lua->get_lua_state(), mod_name + " [main]");
                }
                if (lua_mod->m_hook_lua)
                {
                    all_states.emplace_back(lua_mod->m_hook_lua->get_lua_state(), mod_name + " [hook]");
                }
                if (lua_mod->m_async_lua)
                {
                    all_states.emplace_back(lua_mod->m_async_lua->get_lua_state(), mod_name + " [async]");
                }
            }
        }

        if (all_states.empty())
        {
            ImGui::TextDisabled("No active Lua states");
            return;
        }

        for (const auto& [L, name] : all_states)
        {
            if (!L)
            {
                continue;
            }

            bool is_selected = (m_selected_state == L);

            // Don't call lua_gettop from GUI thread - it's not thread safe
            if (ImGui::Selectable(name.c_str(), is_selected))
            {
                m_selected_state = L;
                // Clear cached globals when selecting a new state
                {
                    std::lock_guard<std::mutex> lock(m_globals_mutex);
                    m_cached_globals_state = nullptr;
                    m_globals_children_cache.clear();
                    m_expanded_paths.clear();
                }
                // Request loaded modules refresh
                request_loaded_modules_refresh();
                // Clear current script selection and auto-select main.lua
                m_current_script_path.clear();
                auto scripts = get_mod_scripts(L);
                for (const auto& script : scripts)
                {
                    // Find main.lua
                    std::string filename = script;
                    size_t last_slash = filename.find_last_of("\\/");
                    if (last_slash != std::string::npos)
                    {
                        filename = filename.substr(last_slash + 1);
                    }
                    if (filename == "main.lua")
                    {
                        m_current_script_path = script;
                        break;
                    }
                }
            }

            // Tooltip with more info
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("State: %p", (void*)L);
                ImGui::EndTooltip();
            }
        }
    }

    auto LuaDebugger::render_stack_view() -> void
    {
        if (!m_selected_state)
        {
            ImGui::TextDisabled("Select a Lua state from the list to view its stack");
            return;
        }

        ImGui::Text("Stack for: %p", (void*)m_selected_state);
        ImGui::Separator();

        // Wrap content in scrollable child
        ImGui::BeginChild("StackViewContent", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

        // When paused at a breakpoint, show cached data (captured on Lua thread)
        // IMPORTANT: Do NOT manipulate the Lua state from GUI thread - it causes crashes!
        if (m_is_paused.load() && m_paused_state)
        {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), ICON_FA_PAUSE " Paused at line %d", m_paused_line);
            ImGui::Separator();

            // Get cached data (thread-safe copy)
            std::vector<LuaCallStackEntry> call_stack;
            std::vector<LuaStackFrame> stack_frames;
            std::vector<LuaStackSlot> stack_slots;
            {
                std::lock_guard<std::mutex> lock(m_paused_data_mutex);
                call_stack = m_paused_call_stack;
                stack_frames = m_paused_stack_frames;
                stack_slots = m_paused_stack_slots;
            }

            // Show call stack
            if (ImGui::CollapsingHeader("Call Stack", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (call_stack.empty())
                {
                    ImGui::TextDisabled("No call stack");
                }
                else
                {
                    render_call_stack(call_stack);
                }
            }

            // Show local variables at each stack level (using cached data)
            if (ImGui::CollapsingHeader("Local Variables", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (stack_frames.empty())
                {
                    ImGui::TextDisabled("No stack frames");
                }
                else
                {
                    // Expand/Collapse all buttons
                    if (ImGui::SmallButton("Expand All##Locals"))
                    {
                        m_stack_frames_expand_action = 1;
                    }
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Collapse All##Locals"))
                    {
                        m_stack_frames_expand_action = 0;
                    }

                    for (size_t level = 0; level < stack_frames.size(); ++level)
                    {
                        const auto& frame = stack_frames[level];
                        std::string header = "Level " + std::to_string(level) + ": " + frame.function_name;

                        // Apply expand/collapse action
                        if (m_stack_frames_expand_action >= 0)
                        {
                            ImGui::SetNextItemOpen(m_stack_frames_expand_action == 1);
                        }

                        if (ImGui::TreeNode(header.c_str()))
                        {
                            if (frame.locals.empty())
                            {
                                ImGui::TextDisabled("(no local variables)");
                            }
                            else
                            {
                                for (size_t local_idx = 0; local_idx < frame.locals.size(); ++local_idx)
                                {
                                    const auto& local = frame.locals[local_idx];

                                    // Color by type
                                    ImVec4 type_color;
                                    if (local.type_name == "nil")
                                        type_color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
                                    else if (local.type_name == "boolean")
                                        type_color = ImVec4(0.6f, 0.8f, 1.0f, 1.0f);
                                    else if (local.type_name == "number")
                                        type_color = ImVec4(0.6f, 1.0f, 0.6f, 1.0f);
                                    else if (local.type_name == "string")
                                        type_color = ImVec4(1.0f, 0.8f, 0.5f, 1.0f);
                                    else if (local.type_name == "table")
                                        type_color = ImVec4(0.8f, 0.6f, 1.0f, 1.0f);
                                    else if (local.type_name == "function")
                                        type_color = ImVec4(1.0f, 0.6f, 0.6f, 1.0f);
                                    else if (local.type_name == "userdata")
                                        type_color = ImVec4(1.0f, 1.0f, 0.6f, 1.0f);
                                    else
                                        type_color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

                                    ImGui::Text("%s", local.name.c_str());
                                    ImGui::SameLine();
                                    ImGui::TextColored(type_color, "[%s]", local.type_name.c_str());
                                    ImGui::SameLine();
                                    ImGui::TextDisabled("= %s", local.value_preview.c_str());

                                    // Context menu for copying
                                    std::string context_id = fmt::format("local-{}_{}", level, local_idx);
                                    if (ImGui::BeginPopupContextItem(context_id.c_str()))
                                    {
                                        if (ImGui::MenuItem(ICON_FA_COPY " Copy Name"))
                                        {
                                            ImGui::SetClipboardText(local.name.c_str());
                                        }
                                        if (ImGui::MenuItem(ICON_FA_COPY " Copy Value"))
                                        {
                                            ImGui::SetClipboardText(local.value_preview.c_str());
                                        }
                                        ImGui::EndPopup();
                                    }
                                }
                            }

                            ImGui::TreePop();
                        }
                    }

                    // Reset expand action after processing all nodes
                    m_stack_frames_expand_action = -1;
                }
            }

            // Show Lua value stack (using cached data)
            if (ImGui::CollapsingHeader("Value Stack"))
            {
                if (stack_slots.empty())
                {
                    ImGui::TextDisabled("Stack is empty");
                }
                else
                {
                    // Context menu for entire stack section
                    if (ImGui::BeginPopupContextItem("value-stack-header"))
                    {
                        if (ImGui::MenuItem(ICON_FA_COPY " Dump All Stack Slots"))
                        {
                            std::ostringstream dump;
                            dump << "=== Lua Value Stack Dump (" << stack_slots.size() << " slots) ===\n";
                            for (const auto& s : stack_slots)
                            {
                                dump << "[" << s.index << "] " << s.type_name << " = " << s.value_preview << "\n";
                            }
                            ImGui::SetClipboardText(dump.str().c_str());
                        }
                        ImGui::EndPopup();
                    }

                    ImGui::Text("%zu stack slots", stack_slots.size());
                    ImGui::SameLine();
                    ImGui::TextDisabled("(right-click for dump)");
                    ImGui::Separator();

                    for (const auto& slot : stack_slots)
                    {
                        ImVec4 type_color;
                        if (slot.type_name == "nil")
                            type_color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
                        else if (slot.type_name == "boolean")
                            type_color = ImVec4(0.6f, 0.8f, 1.0f, 1.0f);
                        else if (slot.type_name == "number")
                            type_color = ImVec4(0.6f, 1.0f, 0.6f, 1.0f);
                        else if (slot.type_name == "string")
                            type_color = ImVec4(1.0f, 0.8f, 0.5f, 1.0f);
                        else if (slot.type_name == "table")
                            type_color = ImVec4(0.8f, 0.6f, 1.0f, 1.0f);
                        else if (slot.type_name == "function")
                            type_color = ImVec4(1.0f, 0.6f, 0.6f, 1.0f);
                        else if (slot.type_name == "userdata")
                            type_color = ImVec4(1.0f, 1.0f, 0.6f, 1.0f);
                        else
                            type_color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

                        ImGui::Text("[%d]", slot.index);
                        ImGui::SameLine();
                        ImGui::TextColored(type_color, "%s", slot.type_name.c_str());
                        ImGui::SameLine();
                        ImGui::TextDisabled("= %s", slot.value_preview.c_str());

                        // Context menu for stack slot
                        std::string context_id = fmt::format("stack-slot-{}", slot.index);
                        if (ImGui::BeginPopupContextItem(context_id.c_str()))
                        {
                            if (ImGui::MenuItem(ICON_FA_COPY " Copy Value"))
                            {
                                ImGui::SetClipboardText(slot.value_preview.c_str());
                            }
                            if (ImGui::MenuItem(ICON_FA_COPY " Copy Type"))
                            {
                                ImGui::SetClipboardText(slot.type_name.c_str());
                            }
                            ImGui::EndPopup();
                        }
                    }
                }
            }
        }
        else
        {
            ImGui::TextDisabled("Not paused at breakpoint");
            ImGui::TextDisabled("");
            ImGui::TextDisabled("To inspect the stack:");
            ImGui::BulletText("Enable Debug checkbox");
            ImGui::BulletText("Click margin to set breakpoints");
            ImGui::BulletText("Run code to hit breakpoint");
        }

        ImGui::EndChild(); // End StackViewContent
    }

    auto LuaDebugger::request_globals_refresh() -> void
    {
        if (!m_selected_state)
        {
            return;
        }

        // Move pending expansions into expanded paths so they get fetched
        {
            std::lock_guard<std::mutex> lock(m_globals_mutex);
            for (const auto& path : m_pending_table_expansions)
            {
                m_expanded_paths.insert(path);
            }
            m_pending_table_expansions.clear();
        }

        // Queue the globals fetch on the game thread
        UE4SSProgram::get_program().queue_event(
                [](void* data) {
                    auto* debugger = static_cast<LuaDebugger*>(data);
                    lua_State* L = debugger->m_selected_state;

                    if (!L)
                    {
                        return;
                    }

                    auto globals = get_globals(L, 500);

                    // Fetch table children for expanded paths
                    std::unordered_map<std::string, std::vector<std::pair<std::string, LuaStackSlot>>> children_cache;

                    // Fetch children for all expanded paths (cache even if empty)
                    for (const auto& path : debugger->m_expanded_paths)
                    {
                        auto children = get_table_entries_at_path(L, path);
                        children_cache[path] = std::move(children);
                    }

                    {
                        std::lock_guard<std::mutex> lock(debugger->m_globals_mutex);
                        debugger->m_cached_globals = std::move(globals);
                        debugger->m_cached_globals_state = L;
                        debugger->m_globals_children_cache = std::move(children_cache);
                        debugger->m_globals_refresh_requested = false;
                    }
                },
                this);

        m_globals_refresh_requested = true;
    }

    auto LuaDebugger::request_loaded_modules_refresh() -> void
    {
        if (!m_selected_state)
        {
            return;
        }

        lua_State* L = m_selected_state;

        UE4SSProgram::get_program().queue_event(
                [](void* data) {
                    auto* params = static_cast<std::pair<LuaDebugger*, lua_State*>*>(data);
                    auto* debugger = params->first;
                    lua_State* L = params->second;
                    delete params;

                    if (!L)
                    {
                        return;
                    }

                    std::vector<std::string> loaded_modules;
                    lua_getglobal(L, "ue4ss_loaded_modules");
                    if (lua_istable(L, -1))
                    {
                        lua_pushnil(L);
                        while (lua_next(L, -2) != 0)
                        {
                            if (lua_isstring(L, -2))
                            {
                                const char* path = lua_tostring(L, -2);
                                if (path && std::filesystem::exists(path))
                                {
                                    loaded_modules.push_back(path);
                                }
                            }
                            lua_pop(L, 1);
                        }
                    }
                    lua_pop(L, 1);

                    {
                        std::lock_guard<std::mutex> lock(debugger->m_loaded_modules_mutex);
                        debugger->m_loaded_modules_cache[L] = std::move(loaded_modules);
                    }
                },
                new std::pair<LuaDebugger*, lua_State*>(this, L));
    }

    // Helper to get table entries at a given path (e.g., "myTable.subTable")
    auto LuaDebugger::get_table_entries_at_path(lua_State* L, const std::string& path) -> std::vector<std::pair<std::string, LuaStackSlot>>
    {
        std::vector<std::pair<std::string, LuaStackSlot>> entries;

        if (!L || path.empty())
        {
            return entries;
        }

        // Parse the path and navigate to the table
        std::vector<std::string> parts;
        size_t start = 0;
        size_t end;
        while ((end = path.find('.', start)) != std::string::npos)
        {
            parts.push_back(path.substr(start, end - start));
            start = end + 1;
        }
        parts.push_back(path.substr(start));

        if (parts.empty())
        {
            return entries;
        }

        // Start from _G
        lua_pushglobaltable(L);

        // Navigate to the target table
        for (const auto& part : parts)
        {
            if (!lua_istable(L, -1))
            {
                lua_pop(L, 1);
                return entries;
            }

            lua_pushstring(L, part.c_str());
            lua_gettable(L, -2);
            lua_remove(L, -2); // Remove parent table
        }

        if (!lua_istable(L, -1))
        {
            lua_pop(L, 1);
            return entries;
        }

        // Iterate over table entries
        lua_pushnil(L);
        int count = 0;
        const int max_entries = 100;

        while (lua_next(L, -2) != 0 && count < max_entries)
        {
            LuaStackSlot slot;
            slot.type_name = luaL_typename(L, -1);
            slot.is_table = lua_istable(L, -1);
            slot.is_userdata = lua_isuserdata(L, -1);
            slot.value_preview = format_stack_value_safe(L, -1, MAX_STACK_PREVIEW_LENGTH);

            // Get key as string
            std::string key;
            int key_type = lua_type(L, -2);
            if (key_type == LUA_TSTRING)
            {
                key = lua_tostring(L, -2);
            }
            else if (key_type == LUA_TNUMBER)
            {
                if (lua_isinteger(L, -2))
                {
                    key = "[" + std::to_string(lua_tointeger(L, -2)) + "]";
                }
                else
                {
                    key = "[" + std::to_string(lua_tonumber(L, -2)) + "]";
                }
            }
            else
            {
                key = "[" + std::string(luaL_typename(L, -2)) + "]";
            }

            entries.emplace_back(key, slot);

            lua_pop(L, 1); // Pop value, keep key for next iteration
            ++count;
        }

        // If we exited early due to max_entries, we still have a key on the stack
        if (count >= max_entries)
        {
            lua_pop(L, 1); // Pop the remaining key
        }

        lua_pop(L, 1); // Pop the table

        return entries;
    }

    auto LuaDebugger::render_globals_view() -> void
    {
        if (!m_selected_state)
        {
            ImGui::TextDisabled("Select a Lua state from the list to view its globals");
            return;
        }

        // Filter input
        ImGui::SetNextItemWidth(scaled(200.0f));
        ImGui::InputTextWithHint("##GlobalsFilter", "Filter globals...", &m_globals_filter);

        ImGui::SameLine();

        bool can_refresh = UE4SSProgram::get_program().can_process_events() && !m_globals_refresh_requested;
        if (!can_refresh)
        {
            ImGui::BeginDisabled(true);
        }
        if (ImGui::Button(ICON_FA_SYNC " Refresh"))
        {
            request_globals_refresh();
        }
        if (!can_refresh)
        {
            ImGui::EndDisabled();
        }

        if (m_globals_refresh_requested)
        {
            ImGui::SameLine();
            ImGui::TextDisabled("(loading...)");
        }

        ImGui::Separator();

        // Check if we need to refresh for a new state
        if (m_cached_globals_state != m_selected_state && !m_globals_refresh_requested)
        {
            // Auto-refresh when selecting a new state
            if (UE4SSProgram::get_program().can_process_events())
            {
                request_globals_refresh();
            }
        }

        // Get cached globals
        std::vector<std::pair<std::string, LuaStackSlot>> globals;
        {
            std::lock_guard<std::mutex> lock(m_globals_mutex);
            if (m_cached_globals_state == m_selected_state)
            {
                globals = m_cached_globals;
            }
        }

        if (globals.empty())
        {
            if (m_globals_refresh_requested)
            {
                ImGui::TextDisabled("Loading globals...");
            }
            else if (m_cached_globals_state != m_selected_state)
            {
                ImGui::TextDisabled("Click 'Refresh' to load globals for this state");
            }
            else
            {
                ImGui::TextDisabled("No user globals found (system globals starting with _ are hidden)");
            }
            return;
        }

        // Apply filter
        std::vector<std::pair<std::string, LuaStackSlot>> filtered_globals;
        if (m_globals_filter.empty())
        {
            filtered_globals = globals;
        }
        else
        {
            for (const auto& [name, slot] : globals)
            {
                if (name.find(m_globals_filter) != std::string::npos ||
                    slot.type_name.find(m_globals_filter) != std::string::npos)
                {
                    filtered_globals.emplace_back(name, slot);
                }
            }
        }

        ImGui::Text("%zu globals shown (of %zu total)", filtered_globals.size(), globals.size());

        ImGui::SameLine();

        if (ImGui::SmallButton("Expand All"))
        {
            for (const auto& [name, slot] : filtered_globals)
            {
                if (slot.is_table || slot.is_userdata)
                {
                    m_expanded_paths.insert(name);
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Collapse All"))
        {
            m_expanded_paths.clear();
            m_globals_children_cache.clear();
        }

        ImGui::Separator();

        // Tree view for globals with expandable tables
        ImGui::BeginChild("GlobalsTree", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

        for (const auto& [name, slot] : filtered_globals)
        {
            ImVec4 type_color;
            if (slot.type_name == "nil")
                type_color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
            else if (slot.type_name == "boolean")
                type_color = ImVec4(0.6f, 0.8f, 1.0f, 1.0f);
            else if (slot.type_name == "number")
                type_color = ImVec4(0.6f, 1.0f, 0.6f, 1.0f);
            else if (slot.type_name == "string")
                type_color = ImVec4(1.0f, 0.8f, 0.5f, 1.0f);
            else if (slot.type_name == "table")
                type_color = ImVec4(0.8f, 0.6f, 1.0f, 1.0f);
            else if (slot.type_name == "function")
                type_color = ImVec4(1.0f, 0.6f, 0.6f, 1.0f);
            else if (slot.type_name == "userdata")
                type_color = ImVec4(1.0f, 1.0f, 0.6f, 1.0f);
            else
                type_color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

            // Tables and userdata are expandable
            bool is_expandable = slot.is_table || slot.is_userdata;

            if (is_expandable)
            {
                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;

                // Check if this node is expanded in our tracking set
                bool was_expanded = m_expanded_paths.count(name) > 0;
                if (was_expanded)
                {
                    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
                }

                bool node_open = ImGui::TreeNodeEx(name.c_str(), flags);

                ImGui::SameLine();
                ImGui::TextColored(type_color, "[%s]", slot.type_name.c_str());
                ImGui::SameLine();
                ImGui::TextDisabled("%s", slot.value_preview.c_str());

                // Context menu for globals
                std::string ctx_id = fmt::format("global-{}", name);
                if (ImGui::BeginPopupContextItem(ctx_id.c_str()))
                {
                    if (ImGui::MenuItem(ICON_FA_COPY " Copy Name"))
                    {
                        ImGui::SetClipboardText(name.c_str());
                    }
                    if (ImGui::MenuItem(ICON_FA_COPY " Copy Value"))
                    {
                        ImGui::SetClipboardText(slot.value_preview.c_str());
                    }
                    ImGui::EndPopup();
                }

                if (node_open)
                {
                    // Track that this is expanded
                    bool newly_expanded = m_expanded_paths.count(name) == 0;
                    if (newly_expanded)
                    {
                        m_expanded_paths.insert(name);
                    }

                    // Check if we have cached children for this global
                    auto child_it = m_globals_children_cache.find(name);
                    if (child_it != m_globals_children_cache.end())
                    {
                        // Render cached children (may be empty)
                        if (child_it->second.empty())
                        {
                            ImGui::TextDisabled("(empty)");
                        }
                        else
                        {
                            render_globals_tree_node(child_it->second, name, 1);
                        }
                    }
                    else
                    {
                        // Need to fetch children - auto-refresh if just expanded
                        if (newly_expanded && UE4SSProgram::get_program().can_process_events() && !m_globals_refresh_requested)
                        {
                            m_pending_table_expansions.insert(name);
                            request_globals_refresh();
                        }
                        else if (!m_globals_refresh_requested)
                        {
                            ImGui::TextDisabled("(expand again to load)");
                        }
                        else
                        {
                            ImGui::TextDisabled("(loading...)");
                        }
                    }

                    ImGui::TreePop();
                }
                else
                {
                    // Not expanded anymore, remove from tracking
                    m_expanded_paths.erase(name);
                }
            }
            else
            {
                // Non-expandable leaf node
                ImGui::Text("  %s", name.c_str());
                ImGui::SameLine();
                ImGui::TextColored(type_color, "[%s]", slot.type_name.c_str());
                ImGui::SameLine();
                ImGui::TextDisabled("= %s", slot.value_preview.c_str());

                // Context menu for leaf globals
                std::string ctx_id = fmt::format("global-leaf-{}", name);
                if (ImGui::BeginPopupContextItem(ctx_id.c_str()))
                {
                    if (ImGui::MenuItem(ICON_FA_COPY " Copy Name"))
                    {
                        ImGui::SetClipboardText(name.c_str());
                    }
                    if (ImGui::MenuItem(ICON_FA_COPY " Copy Value"))
                    {
                        ImGui::SetClipboardText(slot.value_preview.c_str());
                    }
                    ImGui::EndPopup();
                }
            }
        }

        ImGui::EndChild();
    }

    auto LuaDebugger::render_globals_tree_node(const std::vector<std::pair<std::string, LuaStackSlot>>& children,
                                               const std::string& parent_path,
                                               int depth) -> void
    {
        if (depth > static_cast<int>(MAX_TABLE_DEPTH))
        {
            ImGui::TextDisabled("(max depth reached)");
            return;
        }

        for (const auto& [key, slot] : children)
        {
            ImVec4 type_color;
            if (slot.type_name == "nil")
                type_color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
            else if (slot.type_name == "boolean")
                type_color = ImVec4(0.6f, 0.8f, 1.0f, 1.0f);
            else if (slot.type_name == "number")
                type_color = ImVec4(0.6f, 1.0f, 0.6f, 1.0f);
            else if (slot.type_name == "string")
                type_color = ImVec4(1.0f, 0.8f, 0.5f, 1.0f);
            else if (slot.type_name == "table")
                type_color = ImVec4(0.8f, 0.6f, 1.0f, 1.0f);
            else if (slot.type_name == "function")
                type_color = ImVec4(1.0f, 0.6f, 0.6f, 1.0f);
            else if (slot.type_name == "userdata")
                type_color = ImVec4(1.0f, 1.0f, 0.6f, 1.0f);
            else
                type_color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

            std::string full_path = parent_path + "." + key;
            bool is_expandable = slot.is_table || slot.is_userdata;

            if (is_expandable)
            {
                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;

                bool node_open = ImGui::TreeNodeEx(full_path.c_str(), flags, "%s", key.c_str());

                ImGui::SameLine();
                ImGui::TextColored(type_color, "[%s]", slot.type_name.c_str());
                ImGui::SameLine();
                ImGui::TextDisabled("%s", slot.value_preview.c_str());

                // Context menu for tree node
                std::string ctx_id = fmt::format("tree-node-{}", full_path);
                if (ImGui::BeginPopupContextItem(ctx_id.c_str()))
                {
                    if (ImGui::MenuItem(ICON_FA_COPY " Copy Key"))
                    {
                        ImGui::SetClipboardText(key.c_str());
                    }
                    if (ImGui::MenuItem(ICON_FA_COPY " Copy Path"))
                    {
                        ImGui::SetClipboardText(full_path.c_str());
                    }
                    if (ImGui::MenuItem(ICON_FA_COPY " Copy Value"))
                    {
                        ImGui::SetClipboardText(slot.value_preview.c_str());
                    }
                    ImGui::EndPopup();
                }

                if (node_open)
                {
                    // Track expansion
                    bool newly_expanded = m_expanded_paths.count(full_path) == 0;
                    if (newly_expanded)
                    {
                        m_expanded_paths.insert(full_path);
                    }

                    // Check for cached children
                    auto child_it = m_globals_children_cache.find(full_path);
                    if (child_it != m_globals_children_cache.end())
                    {
                        if (child_it->second.empty())
                        {
                            ImGui::TextDisabled("(empty)");
                        }
                        else
                        {
                            render_globals_tree_node(child_it->second, full_path, depth + 1);
                        }
                    }
                    else
                    {
                        // Auto-refresh on expand
                        if (newly_expanded && UE4SSProgram::get_program().can_process_events() && !m_globals_refresh_requested)
                        {
                            m_pending_table_expansions.insert(full_path);
                            request_globals_refresh();
                        }
                        else if (!m_globals_refresh_requested)
                        {
                            ImGui::TextDisabled("(expand again to load)");
                        }
                        else
                        {
                            ImGui::TextDisabled("(loading...)");
                        }
                    }

                    ImGui::TreePop();
                }
                else
                {
                    // Track collapse
                    m_expanded_paths.erase(full_path);
                }
            }
            else
            {
                // Leaf node
                ImGui::Text("  %s", key.c_str());
                ImGui::SameLine();
                ImGui::TextColored(type_color, "[%s]", slot.type_name.c_str());
                ImGui::SameLine();
                ImGui::TextDisabled("= %s", slot.value_preview.c_str());

                // Context menu for leaf node
                std::string ctx_id = fmt::format("tree-leaf-{}", full_path);
                if (ImGui::BeginPopupContextItem(ctx_id.c_str()))
                {
                    if (ImGui::MenuItem(ICON_FA_COPY " Copy Key"))
                    {
                        ImGui::SetClipboardText(key.c_str());
                    }
                    if (ImGui::MenuItem(ICON_FA_COPY " Copy Path"))
                    {
                        ImGui::SetClipboardText(full_path.c_str());
                    }
                    if (ImGui::MenuItem(ICON_FA_COPY " Copy Value"))
                    {
                        ImGui::SetClipboardText(slot.value_preview.c_str());
                    }
                    ImGui::EndPopup();
                }
            }
        }
    }

    auto LuaDebugger::render_error_log() -> void
    {
        // Filter input
        ImGui::SetNextItemWidth(scaled(200.0f));
        ImGui::InputTextWithHint("##ErrorFilter", "Filter...", &m_error_filter);

        ImGui::SameLine();
        ImGui::TextDisabled("(%zu errors)", get_error_count());

        ImGui::Separator();

        // Error list (use minimum height to ensure scrollbar appears)
        float available_height = ImGui::GetContentRegionAvail().y;
        float list_height = std::max(scaled(100.0f), available_height * 0.4f);
        ImGui::BeginChild("ErrorList", ImVec2(0, list_height), true, ImGuiWindowFlags_HorizontalScrollbar);

        std::lock_guard<std::mutex> lock(m_errors_mutex);

        int display_index = 0;
        for (size_t i = 0; i < m_error_history.size(); ++i)
        {
            const auto& error = m_error_history[i];

            // Apply filter
            if (!m_error_filter.empty())
            {
                bool match = error.error_message.find(m_error_filter) != std::string::npos || error.mod_name.find(m_error_filter) != std::string::npos;
                if (!match)
                {
                    continue;
                }
            }

            // Format timestamp
            auto time = std::chrono::system_clock::to_time_t(error.timestamp);
            std::tm tm_buf;
            localtime_s(&tm_buf, &time);
            char time_str[32];
            strftime(time_str, sizeof(time_str), "%H:%M:%S", &tm_buf);

            // Create selectable item
            bool is_selected = (m_selected_error_index == static_cast<int>(i));
            std::string label = std::string(time_str) + " [" + error.mod_name + "] " + error.error_message.substr(0, 50);
            if (error.error_message.length() > 50)
            {
                label += "...";
            }

            if (ImGui::Selectable(label.c_str(), is_selected))
            {
                m_selected_error_index = static_cast<int>(i);
            }

            // Context menu for error list items
            std::string context_id = fmt::format("error-item-{}", i);
            if (ImGui::BeginPopupContextItem(context_id.c_str()))
            {
                if (ImGui::MenuItem(ICON_FA_COPY " Copy Error Message"))
                {
                    ImGui::SetClipboardText(error.error_message.c_str());
                }
                if (ImGui::MenuItem(ICON_FA_COPY " Copy Traceback"))
                {
                    ImGui::SetClipboardText(error.traceback.c_str());
                }
                if (ImGui::MenuItem(ICON_FA_COPY " Copy Full Report"))
                {
                    auto err_time = std::chrono::system_clock::to_time_t(error.timestamp);
                    std::tm err_tm_buf;
                    localtime_s(&err_tm_buf, &err_time);
                    char err_time_str[64];
                    strftime(err_time_str, sizeof(err_time_str), "%Y-%m-%d %H:%M:%S", &err_tm_buf);

                    std::ostringstream oss;
                    oss << "=== Lua Error Report ===\n";
                    oss << "Time: " << err_time_str << "\n";
                    oss << "Mod: " << error.mod_name << "\n\n";
                    oss << "Error:\n" << error.error_message << "\n\n";
                    oss << "Traceback:\n" << error.traceback << "\n";
                    ImGui::SetClipboardText(oss.str().c_str());
                }
                ImGui::EndPopup();
            }

            // Tooltip
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("%s", error.error_message.c_str());
                ImGui::EndTooltip();
            }

            ++display_index;
        }

        // Auto-scroll
        if (m_auto_scroll_errors && !m_error_history.empty())
        {
            ImGui::SetScrollHereY(1.0f);
        }

        ImGui::EndChild();

        // Error details panel
        ImGui::Separator();
        ImGui::BeginChild("ErrorDetails", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

        if (m_selected_error_index >= 0 && m_selected_error_index < static_cast<int>(m_error_history.size()))
        {
            render_error_details(m_error_history[m_selected_error_index]);
        }
        else
        {
            ImGui::TextDisabled("Select an error to view details");
        }

        ImGui::EndChild();
    }

    auto LuaDebugger::render_error_details(const LuaErrorRecord& error) -> void
    {
        // Timestamp and mod
        auto time = std::chrono::system_clock::to_time_t(error.timestamp);
        std::tm tm_buf;
        localtime_s(&tm_buf, &time);
        char time_str[64];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &tm_buf);

        ImGui::Text("Time: %s", time_str);
        ImGui::Text("Mod: %s", error.mod_name.c_str());
        ImGui::Separator();

        // Error message
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Error:");
        ImGui::TextWrapped("%s", error.error_message.c_str());
        if (ImGui::BeginPopupContextItem("error-message-context"))
        {
            if (ImGui::MenuItem(ICON_FA_COPY " Copy Error Message"))
            {
                ImGui::SetClipboardText(error.error_message.c_str());
            }
            ImGui::EndPopup();
        }
        ImGui::Separator();

        // Collapsible sections
        if (ImGui::CollapsingHeader("Call Stack", ImGuiTreeNodeFlags_DefaultOpen))
        {
            render_call_stack(error.call_stack);
        }

        if (ImGui::CollapsingHeader("Traceback"))
        {
            ImGui::TextWrapped("%s", error.traceback.c_str());
            if (ImGui::BeginPopupContextItem("traceback-context"))
            {
                if (ImGui::MenuItem(ICON_FA_COPY " Copy Traceback"))
                {
                    ImGui::SetClipboardText(error.traceback.c_str());
                }
                ImGui::EndPopup();
            }
        }

        if (ImGui::CollapsingHeader("Stack Snapshot"))
        {
            if (error.stack_snapshot.empty())
            {
                ImGui::TextDisabled("Stack was empty at error time");
            }
            else
            {
                for (size_t i = 0; i < error.stack_snapshot.size(); ++i)
                {
                    const auto& slot = error.stack_snapshot[i];
                    ImGui::BulletText("[%d] %s: %s", slot.index, slot.type_name.c_str(), slot.value_preview.c_str());

                    std::string ctx_id = fmt::format("snapshot-slot-{}", i);
                    if (ImGui::BeginPopupContextItem(ctx_id.c_str()))
                    {
                        if (ImGui::MenuItem(ICON_FA_COPY " Copy Value"))
                        {
                            ImGui::SetClipboardText(slot.value_preview.c_str());
                        }
                        if (ImGui::MenuItem(ICON_FA_COPY " Copy Type"))
                        {
                            ImGui::SetClipboardText(slot.type_name.c_str());
                        }
                        ImGui::EndPopup();
                    }
                }
            }
        }

        // Copy button
        ImGui::Separator();
        if (ImGui::Button(ICON_FA_COPY " Copy to Clipboard"))
        {
            std::ostringstream oss;
            oss << "=== Lua Error Report ===\n";
            oss << "Time: " << time_str << "\n";
            oss << "Mod: " << error.mod_name << "\n\n";
            oss << "Error:\n" << error.error_message << "\n\n";
            oss << "Traceback:\n" << error.traceback << "\n";
            ImGui::SetClipboardText(oss.str().c_str());
        }
    }

    auto LuaDebugger::render_call_stack(const std::vector<LuaCallStackEntry>& call_stack) -> void
    {
        if (call_stack.empty())
        {
            ImGui::TextDisabled("No call stack available");
            return;
        }

        if (ImGui::BeginTable("CallStackTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, scaled(30.0f));
            ImGui::TableSetupColumn("Function", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Source", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Line", ImGuiTableColumnFlags_WidthFixed, scaled(60.0f));
            ImGui::TableHeadersRow();

            for (size_t i = 0; i < call_stack.size(); ++i)
            {
                const auto& entry = call_stack[i];

                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::Text("%zu", i);

                ImGui::TableNextColumn();
                if (entry.name_what.empty())
                {
                    ImGui::Text("%s", entry.function_name.c_str());
                }
                else
                {
                    ImGui::Text("%s (%s)", entry.function_name.c_str(), entry.name_what.c_str());
                }

                // Context menu for function name
                std::string func_ctx_id = fmt::format("callstack-func-{}", i);
                if (ImGui::BeginPopupContextItem(func_ctx_id.c_str()))
                {
                    if (ImGui::MenuItem(ICON_FA_COPY " Copy Function Name"))
                    {
                        ImGui::SetClipboardText(entry.function_name.c_str());
                    }
                    if (ImGui::MenuItem(ICON_FA_COPY " Copy Source Path"))
                    {
                        ImGui::SetClipboardText(entry.source.c_str());
                    }
                    if (ImGui::MenuItem(ICON_FA_COPY " Copy Source:Line"))
                    {
                        std::string loc = entry.source + ":" + std::to_string(entry.current_line > 0 ? entry.current_line : entry.line_number);
                        ImGui::SetClipboardText(loc.c_str());
                    }
                    ImGui::EndPopup();
                }

                ImGui::TableNextColumn();
                // Show full path in display, no truncation for copy purposes
                std::string source_display = entry.source;
                if (source_display.length() > 40)
                {
                    source_display = "..." + source_display.substr(source_display.length() - 37);
                }
                ImGui::TextDisabled("%s", source_display.c_str());

                // Tooltip with full path
                if (ImGui::IsItemHovered() && entry.source.length() > 40)
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("%s", entry.source.c_str());
                    ImGui::EndTooltip();
                }

                // Context menu for source
                std::string src_ctx_id = fmt::format("callstack-src-{}", i);
                if (ImGui::BeginPopupContextItem(src_ctx_id.c_str()))
                {
                    if (ImGui::MenuItem(ICON_FA_COPY " Copy Source Path"))
                    {
                        ImGui::SetClipboardText(entry.source.c_str());
                    }
                    ImGui::EndPopup();
                }

                ImGui::TableNextColumn();
                if (entry.current_line > 0)
                {
                    ImGui::Text("%d", entry.current_line);
                }
                else if (entry.line_number > 0)
                {
                    ImGui::TextDisabled("%d", entry.line_number);
                }
                else
                {
                    ImGui::TextDisabled("-");
                }
            }

            ImGui::EndTable();
        }
    }

    auto LuaDebugger::render_debug_controls() -> void
    {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.3f, 0.15f, 0.15f, 1.0f));
        ImGui::BeginChild("DebugControls", ImVec2(0, 70), true);

        // First row: status
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), ICON_FA_PAUSE " PAUSED");
        ImGui::SameLine();

        std::string pause_info = "at line " + std::to_string(m_paused_line);
        if (!m_paused_source.empty())
        {
            std::string src = m_paused_source;
            if (!src.empty() && src[0] == '@')
                src = src.substr(1);
            if (src.length() > 50)
                src = "..." + src.substr(src.length() - 47);
            pause_info += " in " + src;
        }
        ImGui::Text("%s", pause_info.c_str());

        // Second row: buttons (always on new line for visibility)
        if (ImGui::Button(ICON_FA_PLAY " Continue"))
        {
            continue_execution();
        }
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_ARROW_DOWN " Into"))
        {
            step_into();
        }
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_ARROW_RIGHT " Over"))
        {
            step_over();
        }
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_ARROW_UP " Out"))
        {
            step_out();
        }

        ImGui::EndChild();
        ImGui::PopStyleColor();
    }

    auto LuaDebugger::render_breakpoints_panel() -> void
    {
        std::lock_guard<std::mutex> lock(m_breakpoints_mutex);

        if (m_breakpoints.empty())
        {
            ImGui::TextDisabled("No breakpoints set");
            ImGui::TextDisabled("Click line numbers in");
            ImGui::TextDisabled("Script view to add");
            return;
        }

        if (ImGui::Button(ICON_FA_TRASH " Clear All"))
        {
            // Can't call clear_all_breakpoints here due to mutex, just clear directly
            m_breakpoints.clear();
        }

        ImGui::Separator();

        for (size_t i = 0; i < m_breakpoints.size(); ++i)
        {
            auto& bp = m_breakpoints[i];

            ImGui::PushID(static_cast<int>(i));

            // Checkbox to enable/disable
            ImGui::Checkbox("##enabled", &bp.enabled);
            ImGui::SameLine();

            // Shorten source path
            std::string short_source = bp.source_file;
            if (short_source.length() > 20)
            {
                short_source = "..." + short_source.substr(short_source.length() - 17);
            }

            ImGui::Text("%s:%d", short_source.c_str(), bp.line);

            // Show hit count
            if (bp.hit_count > 0)
            {
                ImGui::SameLine();
                ImGui::TextDisabled("(%d hits)", bp.hit_count);
            }

            // Delete button
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 20);
            if (ImGui::SmallButton(ICON_FA_TIMES))
            {
                m_breakpoints.erase(m_breakpoints.begin() + i);
                ImGui::PopID();
                break;
            }

            ImGui::PopID();
        }
    }

    auto LuaDebugger::render_debug_view() -> void
    {
        if (!m_selected_state)
        {
            ImGui::TextDisabled("Select a Lua state to debug");
            return;
        }

        // Debug controls at top
        std::string mod_name = find_mod_name_for_state(m_selected_state);
        bool debug_enabled = has_debug_hook(m_selected_state);

        if (ImGui::Checkbox("Enable Debug", &debug_enabled))
        {
            if (debug_enabled)
            {
                install_debug_hook(m_selected_state);
                m_debug_enabled_mods.insert(mod_name);
            }
            else
            {
                uninstall_debug_hook(m_selected_state);
                m_debug_enabled_mods.erase(mod_name);
            }
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Enable to allow breakpoints to pause execution");
        }

        ImGui::Separator();

        // Split: script view on left, stack on right
        float script_width = ImGui::GetContentRegionAvail().x * 0.65f;

        // Left side: Script view
        ImGui::BeginChild("DebugScriptPanel", ImVec2(script_width, -1.0f), true, ImGuiWindowFlags_HorizontalScrollbar);

        // Script selection
        auto scripts = get_mod_scripts(m_selected_state);
        if (!scripts.empty())
        {
            std::string current_display = m_current_script_path;
            if (!current_display.empty())
            {
                size_t last_slash = current_display.find_last_of("\\/");
                if (last_slash != std::string::npos)
                    current_display = current_display.substr(last_slash + 1);
            }

            ImGui::SetNextItemWidth(scaled(200.0f));
            if (ImGui::BeginCombo("##DebugScriptSelect", m_current_script_path.empty() ? "Select script..." : current_display.c_str()))
            {
                for (const auto& path : scripts)
                {
                    std::string name = path;
                    size_t pos = name.find_last_of("\\/");
                    if (pos != std::string::npos)
                        name = name.substr(pos + 1);

                    if (ImGui::Selectable(name.c_str(), m_current_script_path == path))
                    {
                        m_current_script_path = path;
                    }
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("%s", path.c_str());
                    }
                }
                ImGui::EndCombo();
            }

            // Display script with line numbers and breakpoint indicators
            if (!m_current_script_path.empty())
            {
                const LuaScriptFile* script = load_script(m_current_script_path);
                if (script && script->loaded)
                {
                    ImGui::Separator();

                    ImGui::BeginChild("ScriptContent", ImVec2(-1.0f, -1.0f), false, ImGuiWindowFlags_HorizontalScrollbar);

                    float line_height = ImGui::GetTextLineHeight();
                    float circle_radius = line_height * 0.3f;

                    float line_number_width = ImGui::CalcTextSize("9999").x;
                    float circle_area_width = line_height;
                    float total_margin_width = circle_area_width + line_number_width;

                    for (size_t i = 0; i < script->lines.size(); ++i)
                    {
                        int line_num = static_cast<int>(i + 1);
                        bool has_bp = has_breakpoint(m_current_script_path, line_num);
                        bool is_current = m_is_paused.load() && m_paused_line == line_num &&
                                          (m_paused_source.find(m_current_script_path) != std::string::npos ||
                                           m_current_script_path.find(m_paused_source) != std::string::npos);

                        ImGui::PushID(line_num);

                        ImDrawList* draw_list = ImGui::GetWindowDrawList();

                        // Make the entire margin (circle area + line number) clickable
                        ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
                        ImVec2 circle_center = ImVec2(cursor_pos.x + circle_area_width * 0.5f, cursor_pos.y + line_height * 0.5f);

                        if (ImGui::InvisibleButton("##bp", ImVec2(total_margin_width, line_height)))
                        {
                            toggle_breakpoint(m_current_script_path, line_num);
                        }

                        bool hovered = ImGui::IsItemHovered();

                        // Draw breakpoint circle
                        if (has_bp)
                        {
                            draw_list->AddCircleFilled(circle_center, circle_radius, IM_COL32(220, 50, 50, 255));
                        }
                        else if (hovered)
                        {
                            draw_list->AddCircle(circle_center, circle_radius, IM_COL32(150, 150, 150, 180), 0, 1.5f);
                        }

                        // Draw line number on top of the invisible button
                        char line_num_str[16];
                        snprintf(line_num_str, sizeof(line_num_str), "%4d", line_num);
                        ImVec2 text_pos = ImVec2(cursor_pos.x + circle_area_width, cursor_pos.y);
                        draw_list->AddText(text_pos, ImGui::GetColorU32(ImGuiCol_TextDisabled), line_num_str);

                        ImGui::SameLine();

                        // Line content with highlight if current
                        if (is_current)
                        {
                            ImVec2 line_start = ImGui::GetCursorScreenPos();
                            ImVec2 line_end = ImVec2(line_start.x + ImGui::GetContentRegionAvail().x, line_start.y + line_height);
                            draw_list->AddRectFilled(line_start, line_end, IM_COL32(80, 80, 0, 100));
                            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", script->lines[i].c_str());
                        }
                        else
                        {
                            ImGui::TextUnformatted(script->lines[i].c_str());
                        }

                        ImGui::PopID();
                    }

                    // Auto-scroll to paused line
                    if (m_script_scroll_to_line > 0 && m_script_scroll_to_line <= static_cast<int>(script->lines.size()))
                    {
                        float line_height = ImGui::GetTextLineHeightWithSpacing();
                        ImGui::SetScrollY((m_script_scroll_to_line - 5) * line_height);
                        m_script_scroll_to_line = -1;
                    }

                    ImGui::EndChild();
                }
            }
        }
        else
        {
            ImGui::TextDisabled("No scripts found");
        }

        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("DebugStackPanel", ImVec2(-1.0f, -1.0f), true, ImGuiWindowFlags_HorizontalScrollbar);
        render_stack_view();
        ImGui::EndChild();
    }

    auto LuaDebugger::render_script_editor() -> void
    {
        // Get scripts list if we have a selected state
        std::vector<std::string> scripts;
        if (m_selected_state)
        {
            scripts = get_mod_scripts(m_selected_state);
        }

        // Allow editing if we have a file loaded, even without a selected state
        bool has_loaded_file = !m_script_edit_path.empty() && !m_script_original_content.empty();

        if (!m_selected_state && !has_loaded_file)
        {
            ImGui::TextDisabled("Select a Lua state or open a script from the Mods tab");
            return;
        }

        if (m_selected_state && scripts.empty() && !has_loaded_file)
        {
            ImGui::TextDisabled("No scripts found for this mod");
            return;
        }

        // Script selection
        std::string current_display = m_script_edit_path;
        if (!current_display.empty())
        {
            size_t last_slash = current_display.find_last_of("\\/");
            if (last_slash != std::string::npos)
                current_display = current_display.substr(last_slash + 1);
        }

        // If no state selected but we have a loaded file, scan for other scripts in the same mod folder
        if (scripts.empty() && has_loaded_file)
        {
            std::filesystem::path loaded_path(m_script_edit_path);
            std::filesystem::path scripts_dir = loaded_path.parent_path();
            if (std::filesystem::exists(scripts_dir))
            {
                for (const auto& entry : std::filesystem::recursive_directory_iterator(scripts_dir))
                {
                    if (entry.is_regular_file() && entry.path().extension() == ".lua")
                    {
                        scripts.push_back(entry.path().string());
                    }
                }
                std::sort(scripts.begin(), scripts.end());
            }
        }

        ImGui::SetNextItemWidth(scaled(300.0f));
        if (ImGui::BeginCombo("##EditorScriptSelect", m_script_edit_path.empty() ? "Select script..." : current_display.c_str()))
        {
            for (const auto& path : scripts)
            {
                std::string name = path;
                size_t pos = name.find_last_of("\\/");
                if (pos != std::string::npos)
                    name = name.substr(pos + 1);

                if (ImGui::Selectable(name.c_str(), m_script_edit_path == path))
                {
                    if (m_script_is_dirty && m_script_edit_path != path)
                    {
                        Output::send<LogLevel::Warning>(STR("Discarding unsaved changes\n"));
                    }

                    const LuaScriptFile* script = load_script(path);
                    if (script && script->loaded)
                    {
                        m_script_editor.SetText(script->content);
                        m_script_original_content = m_script_editor.GetText();
                        m_script_edit_path = path;
                        m_script_is_dirty = false;
                    }
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s", path.c_str());
                }
            }
            ImGui::EndCombo();
        }

        ImGui::SameLine();

        // Save button
        ImGui::BeginDisabled(!m_script_is_dirty);
        if (ImGui::Button(ICON_FA_SAVE " Save"))
        {
            std::string content = m_script_editor.GetText();
            if (save_script(m_script_edit_path, content))
            {
                m_script_original_content = content;
                m_script_is_dirty = false;
            }
        }
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            ImGui::SetTooltip("Save changes (Ctrl+S)");
        }

        if (m_script_is_dirty)
        {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), ICON_FA_EXCLAMATION_TRIANGLE " Unsaved");
        }

        ImGui::Separator();

        if (!m_script_edit_path.empty())
        {
            // Track dirty state
            std::string current_text = m_script_editor.GetText();
            m_script_is_dirty = (current_text != m_script_original_content);

            // Ctrl+S to save
            if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S) && m_script_is_dirty)
            {
                if (save_script(m_script_edit_path, current_text))
                {
                    m_script_original_content = current_text;
                    m_script_is_dirty = false;
                }
            }

            m_script_editor.Render("ScriptEditorMain", {-1.0f, scaled(-4.0f)});
        }
    }

    auto LuaDebugger::render_repl() -> void
    {
        if (!m_selected_state)
        {
            ImGui::TextDisabled("Select a Lua state to use the REPL");
            return;
        }

        ImGui::Text("Execute Lua code in the context of: %s", find_mod_name_for_state(m_selected_state).c_str());

        // Input area first (at the top for visibility)
        ImGui::Separator();
        ImGui::Text("Enter Lua code:");

        // Use a char buffer for the input instead of std::string to ensure visibility
        static char repl_input_buffer[4096] = "";

        // Copy current input to buffer if not matching
        if (m_repl_input != repl_input_buffer)
        {
            strncpy_s(repl_input_buffer, sizeof(repl_input_buffer), m_repl_input.c_str(), _TRUNCATE);
        }

        float input_width = ImGui::GetContentRegionAvail().x - 80;
        ImGui::SetNextItemWidth(input_width);

        bool execute = false;
        if (ImGui::InputText("##ReplInput", repl_input_buffer, sizeof(repl_input_buffer), ImGuiInputTextFlags_EnterReturnsTrue))
        {
            execute = true;
        }

        // Sync buffer back to string
        m_repl_input = repl_input_buffer;

        ImGui::SameLine();

        bool has_input = strlen(repl_input_buffer) > 0;
        bool can_execute = !m_repl_pending && has_input && UE4SSProgram::get_program().can_process_events();

        if (!can_execute)
        {
            ImGui::BeginDisabled(true);
        }

        if (ImGui::Button(ICON_FA_PLAY " Run") || (execute && can_execute))
        {
            if (can_execute)
            {
                // Store in history
                m_repl_input_history.push_back(m_repl_input);
                m_repl_history_index = -1;

                // Execute
                execute_repl(m_selected_state, m_repl_input);
                m_repl_input.clear();
                repl_input_buffer[0] = '\0';

                // Set focus back to input
                ImGui::SetKeyboardFocusHere(-1);
            }
        }

        if (!can_execute)
        {
            ImGui::EndDisabled();
        }

        // History display below the input
        ImGui::Separator();
        ImGui::Text("History:");

        ImGui::BeginChild("ReplHistory", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

        {
            std::lock_guard<std::mutex> lock(m_repl_mutex);

            if (m_repl_history.empty())
            {
                ImGui::TextDisabled("No commands executed yet.");
                ImGui::TextDisabled("Type Lua code above and press Enter or click Run.");
            }
            else
            {
                int entry_idx = 0;
                for (const auto& entry : m_repl_history)
                {
                    ImGui::PushID(entry_idx++);

                    // Input - clickable to re-use command
                    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "> %s", entry.input.c_str());
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                        ImGui::SetTooltip("Click to copy to input");
                    }
                    if (ImGui::IsItemClicked())
                    {
                        m_repl_input = entry.input;
                    }

                    // Output
                    if (entry.is_error)
                    {
                        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "  %s", entry.output.c_str());
                    }
                    else
                    {
                        ImGui::TextColored(ImVec4(0.6f, 1.0f, 0.6f, 1.0f), "  %s", entry.output.c_str());
                    }

                    ImGui::PopID();
                    ImGui::Separator();
                }
            }

            if (m_repl_pending)
            {
                ImGui::TextDisabled("Executing...");
            }
        }

        // Auto-scroll
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
        {
            ImGui::SetScrollHereY(1.0f);
        }

        ImGui::EndChild();
    }

    auto LuaDebugger::render_value_tree(std::vector<LuaValueNode>& nodes) -> void
    {
        for (auto& node : nodes)
        {
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
            if (!node.is_expandable)
            {
                flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            }

            // Color by type
            ImVec4 type_color;
            if (node.type_name == "nil")
                type_color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
            else if (node.type_name == "boolean")
                type_color = ImVec4(0.6f, 0.8f, 1.0f, 1.0f);
            else if (node.type_name == "number")
                type_color = ImVec4(0.6f, 1.0f, 0.6f, 1.0f);
            else if (node.type_name == "string")
                type_color = ImVec4(1.0f, 0.8f, 0.5f, 1.0f);
            else if (node.type_name == "table")
                type_color = ImVec4(0.8f, 0.6f, 1.0f, 1.0f);
            else if (node.type_name == "function")
                type_color = ImVec4(1.0f, 0.6f, 0.6f, 1.0f);
            else if (node.type_name == "userdata")
                type_color = ImVec4(1.0f, 1.0f, 0.6f, 1.0f);
            else
                type_color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

            // Build label
            std::string label = node.key + " : ";

            bool node_open = ImGui::TreeNodeEx(node.path.c_str(), flags, "%s", node.key.c_str());

            // Type and value on same line
            ImGui::SameLine();
            ImGui::TextColored(type_color, "[%s]", node.type_name.c_str());
            ImGui::SameLine();
            ImGui::TextDisabled("%s", node.value_preview.c_str());

            if (node_open && node.is_expandable)
            {
                if (m_expanded_paths.count(node.path))
                {
                    if (node.children.empty())
                    {
                        ImGui::TextDisabled("Loading...");
                        m_tree_refresh_requested = true;
                    }
                    else
                    {
                        render_value_tree(node.children);
                    }
                }
                else
                {
                    m_expanded_paths.insert(node.path);
                    m_tree_refresh_requested = true;
                }

                ImGui::TreePop();
            }
        }
    }

    auto LuaDebugger::refresh_mods_list() -> void
    {
        m_discovered_mods.clear();

        auto& program = UE4SSProgram::get_program();
        auto& mods_dirs = program.get_mods_directories();
        auto mods_txt_entries = program.get_mods_txt_entries();

        std::set<std::string> running_mod_names;
        for (const auto& mod : program.m_mods)
        {
            if (auto* lua_mod = dynamic_cast<LuaMod*>(mod.get()))
            {
                if (lua_mod->is_started())
                {
                    running_mod_names.insert(to_string(lua_mod->get_name()));
                }
            }
        }

        std::set<std::string> seen_mods;

        for (const auto& mods_dir : mods_dirs)
        {
            if (!std::filesystem::exists(mods_dir))
            {
                continue;
            }

            std::error_code ec;
            for (const auto& entry : std::filesystem::directory_iterator(mods_dir, ec))
            {
                if (!entry.is_directory(ec) || ec)
                {
                    continue;
                }

                std::string mod_name = entry.path().stem().string();

                if (seen_mods.count(mod_name))
                {
                    continue;
                }
                seen_mods.insert(mod_name);

                bool has_main_lua = std::filesystem::exists(entry.path() / "Scripts" / "main.lua", ec);
                if (!has_main_lua)
                {
                    continue;
                }

                ModInfo info;
                info.name = mod_name;
                info.path = entry.path();
                info.enabled_via_txt = std::filesystem::exists(entry.path() / "enabled.txt", ec);
                info.is_running = running_mod_names.count(mod_name) > 0;

                auto it = mods_txt_entries.find(mod_name);
                if (it != mods_txt_entries.end())
                {
                    info.enabled_via_mods_txt = it->second;
                }

                m_discovered_mods.push_back(std::move(info));
            }
        }

        std::sort(m_discovered_mods.begin(),
                  m_discovered_mods.end(),
                  [](const auto& a, const auto& b) {
                      return a.name < b.name;
                  });

        m_mods_list_dirty = false;
    }

    auto LuaDebugger::set_mod_enabled(const std::filesystem::path& mod_path, bool enabled) -> void
    {
        std::filesystem::path enabled_file = mod_path / "enabled.txt";
        std::error_code ec;

        if (enabled)
        {
            std::ofstream file(enabled_file);
            file.close();
        }
        else
        {
            std::filesystem::remove(enabled_file, ec);
        }

        m_mods_list_dirty = true;
    }

    auto LuaDebugger::restart_mod_by_name(const std::string& mod_name) -> void
    {
        auto& program = UE4SSProgram::get_program();

        // Pass mod name string to the event, not a pointer (which could become stale)
        auto name_copy = std::make_shared<std::string>(mod_name);

        program.queue_event(
                [](void* data) {
                    auto* name_ptr = static_cast<std::shared_ptr<std::string>*>(data);
                    UE4SSProgram::get_program().reinstall_mod_by_name(**name_ptr);
                    delete name_ptr;
                    // Mark the mods list as dirty after the operation completes
                    if (LuaDebugger::has_instance())
                    {
                        LuaDebugger::get().m_mods_list_dirty = true;
                    }
                },
                new std::shared_ptr<std::string>(name_copy));
    }

    auto LuaDebugger::uninstall_mod_by_name(const std::string& mod_name) -> void
    {
        auto& program = UE4SSProgram::get_program();

        // Pass mod name string to the event, not a pointer (which could become stale)
        auto name_copy = std::make_shared<std::string>(mod_name);

        program.queue_event(
                [](void* data) {
                    auto* name_ptr = static_cast<std::shared_ptr<std::string>*>(data);
                    UE4SSProgram::get_program().uninstall_mod_by_name(**name_ptr);
                    delete name_ptr;
                    // Mark the mods list as dirty after the operation completes
                    if (LuaDebugger::has_instance())
                    {
                        LuaDebugger::get().m_mods_list_dirty = true;
                    }
                },
                new std::shared_ptr<std::string>(name_copy));
    }

    auto LuaDebugger::start_mod_by_path(const std::filesystem::path& mod_path) -> void
    {
        auto& program = UE4SSProgram::get_program();

        // Create a copy of the path for the lambda capture
        auto path_copy = std::make_shared<std::filesystem::path>(mod_path);

        program.queue_event(
                [](void* data) {
                    auto* path_ptr = static_cast<std::shared_ptr<std::filesystem::path>*>(data);
                    UE4SSProgram::get_program().start_lua_mod_by_path(**path_ptr);
                    delete path_ptr;
                    // Mark the mods list as dirty after the mod is started
                    if (LuaDebugger::has_instance())
                    {
                        LuaDebugger::get().m_mods_list_dirty = true;
                    }
                },
                new std::shared_ptr<std::filesystem::path>(path_copy));
    }

    auto LuaDebugger::create_new_mod(const std::string& name) -> bool
    {
        if (name.empty())
        {
            return false;
        }

        auto& program = UE4SSProgram::get_program();
        auto& mods_dirs = program.get_mods_directories();

        if (mods_dirs.empty())
        {
            return false;
        }

        std::filesystem::path mod_path = mods_dirs[0] / name;
        std::filesystem::path scripts_path = mod_path / "Scripts";
        std::filesystem::path main_lua_path = scripts_path / "main.lua";

        std::error_code ec;

        if (std::filesystem::exists(mod_path, ec))
        {
            Output::send<LogLevel::Warning>(STR("Mod '{}' already exists\n"), to_generic_string(name));
            return false;
        }

        std::filesystem::create_directories(scripts_path, ec);
        if (ec)
        {
            Output::send<LogLevel::Error>(STR("Failed to create mod directory: {}\n"), to_generic_string(ec.message()));
            return false;
        }

        std::ofstream main_file(main_lua_path);
        if (!main_file.is_open())
        {
            Output::send<LogLevel::Error>(STR("Failed to create main.lua\n"));
            return false;
        }

        main_file << "-- " << name << " mod\n";
        main_file << "-- Created by UE4SS Lua Debugger\n\n";
        main_file << "print(\"[" << name << "] Mod loaded!\")\n";
        main_file.close();

        std::ofstream enabled_file(mod_path / "enabled.txt");
        enabled_file.close();

        m_mods_list_dirty = true;

        // New mod is not running, clear selected state so dropdown uses filesystem scan
        m_selected_state = nullptr;

        m_script_edit_path = main_lua_path.string();
        const LuaScriptFile* script = load_script(m_script_edit_path);
        if (script && script->loaded)
        {
            m_script_editor.SetText(script->content);
            m_script_original_content = m_script_editor.GetText();
            m_script_is_dirty = false;
        }

        m_pending_editor_tab_switch = 1;

        Output::send(STR("Created new mod '{}'\n"), to_generic_string(name));
        return true;
    }

    auto LuaDebugger::create_new_file(const std::string& mod_path, const std::string& filename, bool add_require_to_main) -> bool
    {
        if (filename.empty() || mod_path.empty())
        {
            return false;
        }

        std::filesystem::path scripts_path = std::filesystem::path(mod_path) / "Scripts";
        std::filesystem::path file_path = scripts_path / filename;

        if (!file_path.has_extension())
        {
            file_path += ".lua";
        }

        std::error_code ec;
        if (std::filesystem::exists(file_path, ec))
        {
            Output::send<LogLevel::Warning>(STR("File '{}' already exists\n"), to_generic_string(filename));
            return false;
        }

        std::ofstream file(file_path);
        if (!file.is_open())
        {
            Output::send<LogLevel::Error>(STR("Failed to create file '{}'\n"), to_generic_string(filename));
            return false;
        }

        std::string module_name = file_path.stem().string();
        file << "-- " << module_name << " module\n\n";
        file << "local M = {}\n\n";
        file << "return M\n";
        file.close();

        // Add require() to main.lua if requested
        if (add_require_to_main)
        {
            std::filesystem::path main_lua_path = scripts_path / "main.lua";
            if (std::filesystem::exists(main_lua_path))
            {
                std::ifstream main_file_read(main_lua_path);
                std::string main_content((std::istreambuf_iterator<char>(main_file_read)), std::istreambuf_iterator<char>());
                main_file_read.close();

                std::string require_line = "local " + module_name + " = require(\"" + module_name + "\")\n";

                // Check if require already exists
                if (main_content.find("require(\"" + module_name + "\")") == std::string::npos)
                {
                    // Find the best place to insert - after existing requires or at the top
                    size_t insert_pos = 0;
                    size_t last_require_pos = main_content.rfind("require(");
                    if (last_require_pos != std::string::npos)
                    {
                        // Find end of line after last require
                        size_t newline_pos = main_content.find('\n', last_require_pos);
                        if (newline_pos != std::string::npos)
                        {
                            insert_pos = newline_pos + 1;
                        }
                    }
                    else
                    {
                        // No existing requires, find end of any initial comments
                        size_t pos = 0;
                        while (pos < main_content.size())
                        {
                            // Skip whitespace
                            while (pos < main_content.size() && (main_content[pos] == ' ' || main_content[pos] == '\t' || main_content[pos] == '\n' || main_content[pos] == '\r'))
                                pos++;

                            if (pos < main_content.size() && main_content[pos] == '-' && pos + 1 < main_content.size() && main_content[pos + 1] == '-')
                            {
                                // Skip comment line
                                size_t newline = main_content.find('\n', pos);
                                if (newline != std::string::npos)
                                    pos = newline + 1;
                                else
                                    break;
                            }
                            else
                            {
                                break;
                            }
                        }
                        insert_pos = pos;
                    }

                    main_content.insert(insert_pos, require_line);

                    std::ofstream main_file_write(main_lua_path);
                    main_file_write << main_content;
                    main_file_write.close();

                    // Clear cache for main.lua so it reloads
                    {
                        std::lock_guard<std::mutex> lock(m_scripts_mutex);
                        m_script_cache.erase(main_lua_path.string());
                    }

                    Output::send(STR("Added require() for '{}' to main.lua\n"), to_generic_string(module_name));
                }
            }
        }

        {
            std::lock_guard<std::mutex> lock(m_scripts_mutex);
            m_script_cache.erase(file_path.string());
        }

        // Check if mod is running; if not, clear selected state so dropdown uses filesystem scan
        bool mod_is_running = false;
        std::string mod_name = std::filesystem::path(mod_path).stem().string();
        for (const auto& mod : UE4SSProgram::get_program().m_mods)
        {
            auto* lua_mod = dynamic_cast<LuaMod*>(mod.get());
            if (lua_mod && to_string(lua_mod->get_name()) == mod_name && lua_mod->is_started())
            {
                mod_is_running = true;
                break;
            }
        }
        if (!mod_is_running)
        {
            m_selected_state = nullptr;
        }

        m_script_edit_path = file_path.string();
        const LuaScriptFile* script = load_script(m_script_edit_path);
        if (script && script->loaded)
        {
            m_script_editor.SetText(script->content);
            m_script_original_content = m_script_editor.GetText();
            m_script_is_dirty = false;
        }

        m_pending_editor_tab_switch = 1;

        Output::send(STR("Created new file '{}'\n"), to_generic_string(file_path.string()));
        return true;
    }

    auto LuaDebugger::render_mods_tab() -> void
    {
        if (m_mods_list_dirty)
        {
            refresh_mods_list();
        }

        if (ImGui::Button(ICON_FA_SYNC " Refresh"))
        {
            m_mods_list_dirty = true;
        }

        ImGui::SameLine();

        if (ImGui::Button(ICON_FA_PLUS " New Mod"))
        {
            m_show_create_mod_popup = true;
            m_new_mod_name.clear();
        }

        ImGui::SameLine();

        auto& gui = UE4SSProgram::get_program().get_debugging_ui();
        bool event_busy = gui.m_event_thread_busy || !UE4SSProgram::get_program().can_process_events();
        if (event_busy)
        {
            ImGui::BeginDisabled(true);
        }
        if (ImGui::Button(ICON_FA_REDO " Restart All Mods"))
        {
            gui.m_event_thread_busy = true;
            UE4SSProgram::get_program().queue_event(
                    [](void* data) {
                        UE4SSProgram::get_program().reinstall_mods();
                        static_cast<GUI::DebuggingGUI*>(data)->m_event_thread_busy = false;
                    },
                    &gui);
            m_mods_list_dirty = true;
        }
        if (event_busy)
        {
            ImGui::EndDisabled();
        }

        ImGui::SameLine();
        ImGui::TextDisabled("(%zu mods)", m_discovered_mods.size());

        ImGui::Separator();

        ImGui::BeginChild("ModsList", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

        for (size_t i = 0; i < m_discovered_mods.size(); ++i)
        {
            auto& mod = m_discovered_mods[i];

            ImGui::PushID(static_cast<int>(i));

            bool enabled = mod.is_enabled();
            if (ImGui::Checkbox("##enabled", &enabled))
            {
                set_mod_enabled(mod.path, enabled);
                mod.enabled_via_txt = enabled;
            }

            if (ImGui::IsItemHovered())
            {
                std::string tooltip = "Enable/disable on next reload";
                if (mod.enabled_via_mods_txt && !mod.enabled_via_txt)
                {
                    tooltip += "\n(Currently enabled via mods.txt)";
                }
                ImGui::SetTooltip("%s", tooltip.c_str());
            }

            ImGui::SameLine();

            if (mod.is_running)
            {
                ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), ICON_FA_PLAY);
                ImGui::SameLine();
            }

            if (mod.enabled_via_mods_txt && !mod.enabled_via_txt)
            {
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f), "%s", mod.name.c_str());
            }
            else
            {
                ImGui::Text("%s", mod.name.c_str());
            }

            // Use consistent button width for alignment (widest case: Open + New + Restart + Uninstall)
            float button_width = scaled(320.0f);
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - button_width);

            if (ImGui::SmallButton(ICON_FA_FOLDER_OPEN " Open"))
            {
                std::filesystem::path scripts_path = mod.path / "Scripts";
                std::filesystem::path main_lua = scripts_path / "main.lua";
                if (std::filesystem::exists(main_lua))
                {
                    // Clear selected state if mod isn't running so dropdown uses filesystem scan
                    if (!mod.is_running)
                    {
                        m_selected_state = nullptr;
                    }
                    m_script_edit_path = main_lua.string();
                    const LuaScriptFile* script = load_script(m_script_edit_path);
                    if (script && script->loaded)
                    {
                        m_script_editor.SetText(script->content);
                        m_script_original_content = m_script_editor.GetText();
                        m_script_is_dirty = false;
                        m_pending_editor_tab_switch = 1;
                    }
                }
            }

            ImGui::SameLine();

            if (ImGui::SmallButton(ICON_FA_FILE_ALT " New"))
            {
                m_show_create_file_popup = true;
                m_new_file_name.clear();
                m_add_require_to_main = true;
                m_create_file_mod_path = mod.path;
            }

            // Show different buttons based on whether mod is running
            if (mod.is_running)
            {
                ImGui::SameLine();

                if (event_busy)
                {
                    ImGui::BeginDisabled(true);
                }

                if (ImGui::SmallButton(ICON_FA_SYNC " Restart"))
                {
                    restart_mod_by_name(mod.name);
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Uninstall and reinstall this mod");
                }

                ImGui::SameLine();

                if (ImGui::SmallButton(ICON_FA_STOP " Uninstall"))
                {
                    uninstall_mod_by_name(mod.name);
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Uninstall this mod (will not run until next full reload)");
                }

                if (event_busy)
                {
                    ImGui::EndDisabled();
                }
            }
            else
            {
                // Show Start button for mods that are not running
                ImGui::SameLine();

                if (event_busy)
                {
                    ImGui::BeginDisabled(true);
                }

                if (ImGui::SmallButton(ICON_FA_PLAY " Start"))
                {
                    start_mod_by_path(mod.path);
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Start this mod without reloading all mods");
                }

                if (event_busy)
                {
                    ImGui::EndDisabled();
                }
            }

            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Path: %s", mod.path.string().c_str());
                ImGui::EndTooltip();
            }

            ImGui::PopID();
        }

        ImGui::EndChild();

        if (m_show_create_mod_popup)
        {
            ImGui::OpenPopup("Create New Mod");
            m_show_create_mod_popup = false;
        }

        if (m_show_create_file_popup)
        {
            ImGui::OpenPopup("Create New File");
            m_show_create_file_popup = false;
        }

        if (ImGui::BeginPopupModal("Create New Mod", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Enter name for new mod:");
            ImGui::Separator();

            ImGui::SetNextItemWidth(scaled(300.0f));
            bool enter_pressed = ImGui::InputText("##newmodname", &m_new_mod_name, ImGuiInputTextFlags_EnterReturnsTrue);

            if ((ImGui::Button("Create", ImVec2(scaled(120.0f), 0)) || enter_pressed) && !m_new_mod_name.empty())
            {
                if (create_new_mod(m_new_mod_name))
                {
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::SameLine();

            if (ImGui::Button("Cancel", ImVec2(scaled(120.0f), 0)))
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        if (ImGui::BeginPopupModal("Create New File", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            std::string mod_name = m_create_file_mod_path.stem().string();
            ImGui::Text("Create new file in %s:", mod_name.c_str());
            ImGui::Separator();

            ImGui::SetNextItemWidth(scaled(300.0f));
            bool enter_pressed = ImGui::InputText("##newfilename", &m_new_file_name, ImGuiInputTextFlags_EnterReturnsTrue);

            if (!m_new_file_name.empty() && m_new_file_name.find(".lua") == std::string::npos)
            {
                ImGui::SameLine();
                ImGui::TextDisabled(".lua");
            }

            ImGui::Checkbox("Add require() to main.lua", &m_add_require_to_main);

            if ((ImGui::Button("Create", ImVec2(scaled(120.0f), 0)) || enter_pressed) && !m_new_file_name.empty())
            {
                if (create_new_file(m_create_file_mod_path.string(), m_new_file_name, m_add_require_to_main))
                {
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::SameLine();

            if (ImGui::Button("Cancel", ImVec2(scaled(120.0f), 0)))
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

} // namespace RC::GUI