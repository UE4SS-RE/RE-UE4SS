#include <algorithm>
#include <mutex>
#include <stdexcept>
#include <vector>

#include <LuaMadeSimple/LuaMadeSimple.hpp>
#include <LuaMadeSimple/LuaObject.hpp>

#include <Helpers/String.hpp>

namespace RC::LuaMadeSimple
{
    // All lua instances, lua_State* are stored in the Lua class
    static std::unordered_map<lua_State*, std::shared_ptr<Lua>> lua_instances;

    static std::vector<std::optional<Lua::LuaFunction>> lua_functions;

    // Current errors for all lua states
    static std::unordered_map<lua_State*, std::string> lua_state_errors;

    // Error callbacks for external error handling/logging
    static std::vector<LuaErrorCallback> lua_error_callbacks;
    static std::mutex lua_error_callbacks_mutex;

    auto register_error_callback(LuaErrorCallback callback) -> void
    {
        std::lock_guard<std::mutex> lock(lua_error_callbacks_mutex);
        lua_error_callbacks.push_back(callback);
    }

    auto unregister_error_callback(LuaErrorCallback callback) -> void
    {
        std::lock_guard<std::mutex> lock(lua_error_callbacks_mutex);
        lua_error_callbacks.erase(
            std::remove(lua_error_callbacks.begin(), lua_error_callbacks.end(), callback),
            lua_error_callbacks.end());
    }

    static auto notify_error_callbacks(lua_State* L, const std::string& error_message, const std::string& traceback) -> void
    {
        std::lock_guard<std::mutex> lock(lua_error_callbacks_mutex);
        for (auto& callback : lua_error_callbacks)
        {
            if (callback)
            {
                callback(L, error_message, traceback);
            }
        }
    }

    // Message handler for lua_pcall that captures the stack while it's still intact
    // This is called BEFORE the stack unwinds, so we can get a proper traceback
    static int pcall_error_handler(lua_State* L)
    {
        // Get the error message from the top of the stack
        const char* msg = lua_tostring(L, 1);
        std::string error_message = msg ? msg : "unknown error";

        // Generate traceback while stack is intact (starting at level 1 to skip this handler)
        luaL_traceback(L, L, error_message.c_str(), 1);
        std::string traceback = lua_tostring(L, -1);

        // Notify callbacks while stack is still intact
        notify_error_callbacks(L, error_message, traceback);

        // Return the traceback as the error message
        return 1;
    }

    // Helper to push the error handler and return its stack index
    static int push_error_handler(lua_State* L)
    {
        lua_pushcfunction(L, pcall_error_handler);
        return lua_gettop(L);
    }

    template <>
    auto LuaTableData<-1>::is_nil() -> bool
    {
        return lua_isnil(lua->get_lua_state(), -1);
    }
    template <>
    auto LuaTableData<-1>::is_string() const -> bool
    {
        return lua_type(lua->get_lua_state(), -1) == LUA_TSTRING;
    }
    template <>
    auto LuaTableData<-1>::get_string() const -> std::string_view
    {
        return lua_tostring(lua->get_lua_state(), -1);
    }
    template <>
    auto LuaTableData<-1>::is_number() const -> bool
    {
        return lua_isnumber(lua->get_lua_state(), -1);
    }
    template <>
    auto LuaTableData<-1>::get_number() const -> double
    {
        return lua_tonumber(lua->get_lua_state(), -1);
    }
    template <>
    auto LuaTableData<-1>::get_float(int32_t force_index) -> float // Safe to use after a call to is_number
    {
        return static_cast<float>(lua_tonumber(lua->get_lua_state(), force_index));
    }
    template <>
    auto LuaTableData<-1>::is_integer() const -> bool
    {
        return lua_isinteger(lua->get_lua_state(), -1);
    }
    template <>
    auto LuaTableData<-1>::get_integer() const -> int64_t
    {
        return lua_tointeger(lua->get_lua_state(), -1);
    }
    template <>
    auto LuaTableData<-1>::is_bool() const -> bool
    {
        return lua_isboolean(lua->get_lua_state(), -1);
    }
    template <>
    auto LuaTableData<-1>::get_bool() const -> bool
    {
        return lua_toboolean(lua->get_lua_state(), -1);
    }
    template <>
    auto LuaTableData<-1>::is_table() const -> bool
    {
        return lua_istable(lua->get_lua_state(), -1);
    }

    template <>
    auto LuaTableData<-2>::is_nil() -> bool
    {
        return lua_isnil(lua->get_lua_state(), -2);
    }
    template <>
    auto LuaTableData<-2>::is_string() const -> bool
    {
        return lua_type(lua->get_lua_state(), -2) == LUA_TSTRING;
    }
    template <>
    auto LuaTableData<-2>::get_string() const -> std::string_view
    {
        return lua_tostring(lua->get_lua_state(), -2);
    }
    template <>
    auto LuaTableData<-2>::is_number() const -> bool
    {
        return lua_isnumber(lua->get_lua_state(), -2);
    }
    template <>
    auto LuaTableData<-2>::get_number() const -> double
    {
        return lua_tonumber(lua->get_lua_state(), -2);
    }
    template <>
    auto LuaTableData<-2>::get_float(int32_t force_index) -> float // Safe to use after a call to is_number
    {
        return static_cast<float>(lua_tonumber(lua->get_lua_state(), force_index));
    }
    template <>
    auto LuaTableData<-2>::is_integer() const -> bool
    {
        return lua_isinteger(lua->get_lua_state(), -2);
    }
    template <>
    auto LuaTableData<-2>::get_integer() const -> int64_t
    {
        return lua_tointeger(lua->get_lua_state(), -2);
    }
    template <>
    auto LuaTableData<-2>::is_bool() const -> bool
    {
        return lua_isboolean(lua->get_lua_state(), -2);
    }
    template <>
    auto LuaTableData<-2>::get_bool() const -> bool
    {
        return lua_toboolean(lua->get_lua_state(), -2);
    }
    template <>
    auto LuaTableData<-2>::is_table() const -> bool
    {
        return lua_istable(lua->get_lua_state(), -2);
    }

    Lua::Lua(lua_State* lua_state) : m_lua_state(lua_state), m_registry(*this)
    {
    }

    Lua::~Lua()
    {
        // Why is calling lua_close() here causing a crash
        // lua_close(get_lua_state());
    }

    Lua::Registry::Registry(const Lua& lua) : m_lua_instance(lua)
    {
    }

    auto Lua::Registry::get_lua_instance() const -> const Lua&
    {
        return m_lua_instance;
    }

    auto Lua::Registry::make_ref() const -> int32_t
    {
        return luaL_ref(get_lua_instance().get_lua_state(), LUA_REGISTRYINDEX);
    }

    auto Lua::Registry::get_string_ref(int32_t registry_index) const -> void
    {
        if (get_lua_instance().rawgeti(LUA_REGISTRYINDEX, registry_index) != LUA_TSTRING)
        {
            get_lua_instance().discard_value();
            get_lua_instance().throw_error("[Lua::Registry::get_string_ref] Ref was not string");
        }
    }

    auto Lua::Registry::get_number_ref(int32_t registry_index) const -> void
    {
        get_lua_instance().rawgeti(LUA_REGISTRYINDEX, registry_index);
        if (!get_lua_instance().is_number(-1))
        {
            get_lua_instance().discard_value();
            get_lua_instance().throw_error("[Lua::Registry::get_number_ref] Ref was not number");
        }
    }

    auto Lua::Registry::get_integer_ref(int32_t registry_index) const -> void
    {
        get_lua_instance().rawgeti(LUA_REGISTRYINDEX, registry_index);
        if (!get_lua_instance().is_integer(-1))
        {
            get_lua_instance().discard_value();
            get_lua_instance().throw_error("[Lua::Registry::get_integer_ref] Ref was not integer");
        }
    }

    auto Lua::Registry::get_bool_ref(int32_t registry_index) const -> void
    {
        get_lua_instance().rawgeti(LUA_REGISTRYINDEX, registry_index);
        if (!get_lua_instance().is_bool(-1))
        {
            get_lua_instance().discard_value();
            get_lua_instance().throw_error("[Lua::Registry::get_bool_ref] Ref was not bool");
        }
    }

    auto Lua::Registry::get_function_ref(int32_t registry_index) const -> void
    {
        if (get_lua_instance().rawgeti(LUA_REGISTRYINDEX, registry_index) != LUA_TFUNCTION)
        {
            get_lua_instance().discard_value();
            get_lua_instance().throw_error("[Lua::Registry::get_function_ref] Ref was not function");
        }
    }

    auto Lua::Registry::get_table_ref(int32_t registry_index) const -> void
    {
        if (get_lua_instance().rawgeti(LUA_REGISTRYINDEX, registry_index) != LUA_TTABLE)
        {
            get_lua_instance().discard_value();
            get_lua_instance().throw_error("[Lua::Registry::get_table_ref] Ref was not table");
        }
    }

    Lua::Table::Table(const Lua& lua_instance) : m_lua_instance(lua_instance)
    {
        // printf_s("Constructed Table\n");
    }

    auto Lua::Table::add_function_value_internal(Lua::LuaFunction function) const -> void
    {
        lua_functions.emplace_back(LuaFunction{function});

        // Upvalues for process_lua_function
        // Upvalue #1: Function id
        lua_pushinteger(get_lua_instance().get_lua_state(), lua_functions.size() - 1);

        // Upvalue #2: Function type
        lua_pushinteger(get_lua_instance().get_lua_state(), static_cast<lua_Integer>(m_has_userdata ? LuaFunctionType::Local : LuaFunctionType::Table));

        lua_pushcclosure(get_lua_instance().get_lua_state(), &process_lua_function, 2);
    }

    auto Lua::Table::get_lua_instance() const -> const Lua&
    {
        return m_lua_instance;
    }

    auto Lua::Table::get_metamethods() -> Lua::MetaMethods&
    {
        return m_metamethods;
    }

    auto Lua::Table::fuse_pair() -> void
    {
        lua_rawset(get_lua_instance().get_lua_state(), -3);
    }

    // Nop, this function is just to clarify when you make a local table
    auto Lua::Table::make_local() -> void
    {
    }

    auto Lua::Table::make_global(std::string_view table_name) const -> void
    {
        lua_setglobal(get_lua_instance().get_lua_state(), table_name.data());
    }

    auto Lua::Table::find_value() -> void
    {
        lua_rawget(get_lua_instance().get_lua_state(), -2);

        // Push the bottom of the stack to the top (table)
        lua_pushvalue(get_lua_instance().get_lua_state(), 1);

        // Remove the bottom of the stack, this puts the table value at the bottom
        lua_remove(get_lua_instance().get_lua_state(), 1);
    }

    auto Lua::Table::has_userdata() const -> bool
    {
        return m_has_userdata;
    }

    auto Lua::Table::set_has_userdata(bool new_value) -> void
    {
        m_has_userdata = new_value;
    }

    auto Lua::Table::push_field_name_and_value(std::string_view field_name) -> int32_t
    {
        // Push the field name to the top of the stack
        lua_pushstring(get_lua_instance().get_lua_state(), field_name.data());

        // Push the value of the field to the top of the stack
        // Table is at -2 (idx param)
        // Field name is at -1
        int32_t table_value_type = lua_rawget(get_lua_instance().get_lua_state(), -2);

        // Table is now at -3 (if: has_table_marked_for_pop == true, otherwise nothing/unrelated)
        // Table is now at -2
        // Value is now at -1

        if (m_has_table_marked_for_pop)
        {
            lua_remove(get_lua_instance().get_lua_state(), -2);
            m_has_table_marked_for_pop = false;
        }

        return table_value_type;
    }

    auto Lua::Table::does_field_exist(std::string_view field_name) -> bool
    {
        // Push the field name to the top of the stack
        lua_pushstring(get_lua_instance().get_lua_state(), field_name.data());

        // Push the value of the field to the top of the stack
        int32_t table_value_type = lua_rawget(get_lua_instance().get_lua_state(), -2);

        // Pop the value of the stack as we don't need it here, we're just checking if it exists
        lua_pop(get_lua_instance().get_lua_state(), 1);

        return table_value_type != LUA_TNIL;
    }

    auto Lua::Table::get_string_field(std::string_view field_name, bool* has_error) -> std::string_view
    {
        int pushed_type = push_field_name_and_value(field_name);
        if (pushed_type != LUA_TSTRING)
        {
            if (has_error)
            {
                *has_error = true;
            }
            else
            {
                get_lua_instance().throw_error(fmt::format("[get_string_field] Attempted to get field: '{}' but the type was '{}'",
                                                           field_name,
                                                           lua_typename(get_lua_instance().get_lua_state(), pushed_type)));
            }
        }

        // Pop the value of the field from the stack
        std::string_view str = lua_tostring(get_lua_instance().get_lua_state(), -1);
        lua_pop(get_lua_instance().get_lua_state(), 1);
        return str;
    }

    auto Lua::Table::get_int_field(std::string_view field_name, bool* has_error) -> int64_t
    {
        push_field_name_and_value(field_name);
        if (!get_lua_instance().is_integer(-1))
        {
            if (has_error)
            {
                *has_error = true;
            }
            else
            {
                get_lua_instance().throw_error(fmt::format("[get_int_field] Attempted to get field: '{}' but the type was '{}'",
                                                           field_name,
                                                           lua_typename(get_lua_instance().get_lua_state(), lua_type(get_lua_instance().get_lua_state(), -1))));
            }
        }

        // Pop the value of the field from the stack
        int64_t integer = lua_tointeger(get_lua_instance().get_lua_state(), -1);
        lua_pop(get_lua_instance().get_lua_state(), 1);
        return integer;
    }

    auto Lua::Table::get_float_field(std::string_view field_name, bool* has_error) -> float
    {
        push_field_name_and_value(field_name);
        if (!get_lua_instance().is_number(-1))
        {
            if (has_error)
            {
                *has_error = true;
            }
            else
            {
                get_lua_instance().throw_error(fmt::format("[get_float_field] Attempted to get field: '{}' but the type was '{}'",
                                                           field_name,
                                                           lua_typename(get_lua_instance().get_lua_state(), lua_type(get_lua_instance().get_lua_state(), -1))));
            }
        }

        // Pop the value of the field from the stack
        float number = static_cast<float>(lua_tonumber(get_lua_instance().get_lua_state(), -1));
        lua_pop(get_lua_instance().get_lua_state(), 1);
        return number;
    }

    auto Lua::Table::get_table_field(std::string_view field_name, bool* has_error) -> Lua::Table&
    {
        int pushed_type = push_field_name_and_value(field_name);
        if (pushed_type != LUA_TTABLE)
        {
            if (has_error)
            {
                *has_error = true;
            }
            else
            {
                get_lua_instance().throw_error(fmt::format("[get_table_field] Attempted to get field: '{}' but the type was '{}'",
                                                           field_name,
                                                           lua_typename(get_lua_instance().get_lua_state(), pushed_type)));
            }
        }

        m_has_table_marked_for_pop = true;

        // There's now another table on the top of the stack
        // Return any Lua::Table object to enable chaining of a function
        // The Lua::Table object doesn't store any table specific stuff, so we can reuse the existing one
        return *this;
    }

    auto Lua::dump_stack(const char* message) const -> void
    {
        ::RC::LuaMadeSimple::dump_stack(get_lua_state(), message);
    }

    auto Lua::get_stack_dump(const char* message) const -> std::string
    {
        return ::RC::LuaMadeSimple::get_stack_dump(get_lua_state(), message);
    }

    auto Lua::handle_error(const std::string& error_message) const -> const std::string
    {
        return ::RC::LuaMadeSimple::handle_error(get_lua_state(), error_message);
    }

    auto Lua::throw_error(const std::string& error_message) const -> void
    {
        ::RC::LuaMadeSimple::throw_error(get_lua_state(), error_message);
    }

    auto Lua::status_to_string(int status) const -> std::string_view
    {
        switch (status)
        {
        case LUA_YIELD:
            return std::string_view("LUA_YIELD");
        case LUA_ERRRUN:
            return std::string_view("LUA_ERRRUN");
        case LUA_ERRSYNTAX:
            return std::string_view("LUA_ERRSYNTAX");
        case LUA_ERRMEM:
            return std::string_view("LUA_ERRMEM");
        case LUA_ERRERR:
            return std::string_view("LUA_ERRERR");
        case LUA_ERRFILE:
            return std::string_view("LUA_ERRFILE");
        }

        return std::string_view("Unknown error");
    }

    auto Lua::resolve_status_message(int status, bool from_lua_only) const -> std::string
    {
        if (!from_lua_only && lua_state_errors.contains(get_lua_state()))
        {
            return lua_state_errors[get_lua_state()];
        }
        else if (lua_isstring(get_lua_state(), -1))
        {
            const char* status_message = lua_tostring(get_lua_state(), -1);
            lua_pop(get_lua_state(), 1);
            return fmt::format("{} => {}", status_to_string(status), status_message);
        }
        else
        {
            return fmt::format("{} => No message", status_to_string(status));
        }
    }

    auto Lua::add_metamethods(const MetaMethods& metamethods, const Table& metatable) const -> bool
    {
        auto create = [&](const char* method_name, std::optional<LuaFunction> user_callable) {
            if (user_callable.has_value())
            {
                metatable.add_pair(method_name, user_callable.value());
            }
        };

        // Hide metatable from lua scripts
        metatable.add_pair("__metatable", false);

        // Create the '__index' metamethod
        // This one will always exist but the user supplied callable might not
        metatable.add_pair("__index", [](const LuaMadeSimple::Lua& lua) -> int {
            if (lua.is_userdata(-2))
            {
                // Get the global table that corresponds to this metatable
                if (lua_getiuservalue(lua.get_lua_state(), -2, 3) == LUA_TNIL)
                {
                    lua_pop(lua.get_lua_state(), 1);
                    luaL_traceback(lua.get_lua_state(), lua.get_lua_state(), nullptr, 0);
                    throw std::runtime_error{"System: Global for __index doesn't exist."};
                }

                // Push the member_name to the top of the stack for lua_raw_get
                lua_pushvalue(lua.get_lua_state(), -2);
                if (lua_rawget(lua.get_lua_state(), -2) != LUA_TNIL)
                {
                    return 1;
                }
                else
                {
                    // Pop the table from lua_getglobal and the nil from lua_rawget
                    lua_pop(lua.get_lua_state(), 2);

                    // Duplicate the userdata so that we can work with it without deleting the original from the stack
                    lua_pushvalue(lua.get_lua_state(), -2);

                    // Push onto the stack, the userdata corresponding to the MetaMethodContainer for this userdata
                    lua_getiuservalue(lua.get_lua_state(), -1, 4);
                    const auto& lua_object = lua.get_userdata<Type::MetaMethodContainer>(-1);

                    const MetaMethods& user_callables = lua_object.metamethods;

                    if (user_callables.index.has_value())
                    {
                        user_callables.index.value()(lua);
                    }
                }
            }
            return 1;
        });

        create("__newindex", metamethods.new_index);

        bool custom_gc_method{};
        if (metamethods.gc.has_value())
        {
            create("__gc", metamethods.gc);
            custom_gc_method = true;
        }

        create("__call", metamethods.call);
        create("__eq", metamethods.equal);
        create("__len", metamethods.length);

        return custom_gc_method;
    };

    auto Lua::register_post_function_process_callback(PostFunctionProcessCallback callback) -> void
    {
        m_post_function_process_callbacks.emplace_back(callback);
    }

    auto Lua::execute_file(std::string_view file_name_and_path) const -> void
    {
        // Push error handler first
        int err_handler_idx = push_error_handler(get_lua_state());

        if (int status = luaL_loadfile(get_lua_state(), file_name_and_path.data()); status != LUA_OK)
        {
            lua_remove(get_lua_state(), err_handler_idx); // Clean up error handler
            throw std::runtime_error{fmt::format("[Lua::execute_file] luaL_loadfile returned {}", resolve_status_message(status))};
        }

        // Use our error handler (err_handler_idx) for pcall
        if (int status = lua_pcall(get_lua_state(), 0, LUA_MULTRET, err_handler_idx); status != LUA_OK)
        {
            // Error message already includes traceback from our handler
            std::string error_msg = lua_tostring(get_lua_state(), -1);
            lua_pop(get_lua_state(), 1); // Pop error message
            lua_remove(get_lua_state(), err_handler_idx); // Clean up error handler
            throw std::runtime_error{fmt::format("[Lua::execute_file] lua_pcall returned {}: {}", resolve_status_message(status), error_msg)};
        }

        lua_remove(get_lua_state(), err_handler_idx); // Clean up error handler
    }

    auto Lua::execute_file(std::wstring_view file_name_and_path) const -> void
    {
        std::string file_name_and_path_utf8 = to_string(file_name_and_path);
        execute_file(file_name_and_path_utf8);
    }

    auto Lua::execute_file(std::u16string_view file_name_and_path) const -> void
    {
        std::string file_name_and_path_utf8 = to_string(file_name_and_path);
        execute_file(file_name_and_path_utf8);
    }

    auto Lua::execute_string(std::string_view code) const -> void
    {
        // Push error handler first
        int err_handler_idx = push_error_handler(get_lua_state());

        if (int status = luaL_loadstring(get_lua_state(), code.data()); status != LUA_OK)
        {
            lua_remove(get_lua_state(), err_handler_idx); // Clean up error handler
            throw_error(fmt::format("[Lua::execute_string] luaL_loadstring returned {}", resolve_status_message(status)));
        }

        // Use our error handler for pcall
        if (int status = lua_pcall(get_lua_state(), 0, LUA_MULTRET, err_handler_idx); status != LUA_OK)
        {
            // Error message already includes traceback from our handler, just clean up
            lua_pop(get_lua_state(), 1); // Pop error message (already processed in handler)
            lua_remove(get_lua_state(), err_handler_idx); // Clean up error handler
            throw_error(fmt::format("[Lua::execute_string] lua_pcall returned {}", resolve_status_message(status)));
        }

        lua_remove(get_lua_state(), err_handler_idx); // Clean up error handler
    }

    auto Lua::execute_string(std::wstring_view code) const -> void
    {
        std::string code_utf8 = to_string(code);
        execute_string(code_utf8);
    }

    auto Lua::execute_string(std::u16string_view code) const -> void
    {
        std::string code_utf8 = to_string(code);
        execute_string(code_utf8);
    }

    auto Lua::open_all_libs() const -> void
    {
        luaL_openlibs(get_lua_state());
    }

    auto Lua::set_hook(lua_Hook func, int32_t mask, int32_t count) const -> void
    {
        lua_sethook(get_lua_state(), func, mask, count);
    }

    auto Lua::registry() const -> const Registry&
    {
        return m_registry;
    }

    auto Lua::rawgeti(int32_t idx, lua_Integer n) const -> int32_t
    {
        return lua_rawgeti(get_lua_state(), idx, n);
    }

    auto Lua::register_function(const std::string& name, const LuaFunction& function) const -> void
    {
        lua_functions.emplace_back(LuaFunction{function});

        // Upvalue for process_lua_function
        lua_pushinteger(get_lua_state(), lua_functions.size() - 1);
        lua_pushinteger(get_lua_state(), static_cast<lua_Integer>(LuaFunctionType::Global));

        lua_pushcclosure(get_lua_state(), &process_lua_function, 2);
        lua_setglobal(get_lua_state(), name.c_str());
    }

    auto Lua::get_lua_state() const -> lua_State*
    {
        return m_lua_state;
    }

    auto Lua::get_stack_size() const -> int32_t
    {
        return lua_gettop(get_lua_state());
    }

    auto Lua::get_type_string() const -> std::string_view
    {
        return lua_typename(get_lua_state(), lua_type(get_lua_state(), 1));
    }

    auto Lua::discard_value(int32_t force_index) const -> void
    {
        lua_remove(get_lua_state(), force_index);
    }

    auto Lua::insert_value(int32_t from_index, int32_t force_index) const -> void
    {
        lua_pushvalue(get_lua_state(), from_index);
        lua_insert(get_lua_state(), force_index);
    }

    auto Lua::is_nil(int32_t force_index) const -> bool
    {
        return lua_isnil(get_lua_state(), force_index);
    }

    auto Lua::set_nil() const -> void
    {
        lua_pushnil(get_lua_state());
    }

    auto Lua::is_string(int32_t force_index) const -> bool
    {
        // Not using lua_isstring because I don't want to return true if the type is non-string but convertible to string (example: number)
        return lua_type(get_lua_state(), force_index) == LUA_TSTRING;
    }

    auto Lua::get_string(int32_t force_index) const -> std::string_view
    {
        std::string_view string = lua_tostring(get_lua_state(), force_index);
        lua_remove(get_lua_state(), force_index);
        return string;
    }

    auto Lua::set_string(const char* str, size_t len) const -> void
    {
        lua_pushlstring(get_lua_state(), str, len);
    }

    auto Lua::set_string(const uint8_t* str, size_t len) const -> void
    {
        lua_pushlstring(get_lua_state(), reinterpret_cast<const char*>(str), len);
    }

    auto Lua::set_string(std::string_view str) const -> void
    {
        lua_pushlstring(get_lua_state(), str.data(), str.length());
    }

    auto Lua::is_number(int32_t force_index) const -> bool
    {
        return lua_isnumber(get_lua_state(), force_index);
    }

    auto Lua::get_number(int32_t force_index) const -> double
    {
        double number = lua_tonumber(get_lua_state(), force_index);
        lua_remove(get_lua_state(), force_index);
        return number;
    }

    auto Lua::set_number(double number) const -> void
    {
        lua_pushnumber(get_lua_state(), number);
    }

    auto Lua::get_float(int32_t force_index) const -> float
    {
        float number = static_cast<float>(lua_tonumber(get_lua_state(), force_index));
        lua_remove(get_lua_state(), force_index);
        return number;
    }

    auto Lua::set_float(float number) const -> void
    {
        lua_pushnumber(get_lua_state(), static_cast<double>(number));
    }

    auto Lua::is_integer(int32_t force_index) const -> bool
    {
        return lua_isinteger(get_lua_state(), force_index);
    }

    auto Lua::get_integer(int32_t force_index) const -> int64_t
    {
        int64_t integer = lua_tointeger(get_lua_state(), force_index);
        lua_remove(get_lua_state(), force_index);
        return integer;
    }

    auto Lua::set_integer(int64_t integer) const -> void
    {
        lua_pushinteger(get_lua_state(), integer);
    }

    auto Lua::is_bool(int32_t force_index) const -> bool
    {
        return lua_isboolean(get_lua_state(), force_index);
    }

    auto Lua::get_bool(int32_t force_index) const -> bool
    {
        int64_t integer = lua_toboolean(get_lua_state(), force_index);
        lua_remove(get_lua_state(), force_index);
        return integer;
    }

    auto Lua::set_bool(bool bool_value) const -> void
    {
        lua_pushboolean(get_lua_state(), bool_value);
    }

    auto Lua::is_table(int32_t force_index) const -> bool
    {
        return lua_istable(get_lua_state(), force_index);
    }

    auto Lua::get_table() const -> Table
    {
        return Table{*this};
    }

    auto Lua::for_each_in_table(const Lua::ForEachInTableCallable& callable) const -> void
    {
        ++m_num_tables_being_iterated;

        // If there are other values on the stack after the table then everything break
        // So lets make sure the table is at the top of the stack
        // 1: table (original table)

        // If we're already iterating at able then the table will be at index -2 instead of at index 1
        lua_pushvalue(get_lua_state(), m_num_tables_being_iterated > 1 ? -2 : 1);
        // 1: table (original table)
        // 2: table  (original table, dupe)

        // Put nil on the top of the stack for lua_next
        lua_pushnil(get_lua_state());
        // 1: table (original table)
        // 2: table (original table, dupe)
        // 3: nil (key)

        // Stack index -2 = table
        while (lua_next(get_lua_state(), -2) != 0)
        {
            // 1: table (original table)
            // 2: table  (original table, dupe)
            // 3: number (key)
            // 4: table (value)

            // Put the table key at the top of the stack
            // This is to prevent lua_next from getting confused if the key is a number
            lua_pushvalue(get_lua_state(), -2);
            // 1: table (original table)
            // 2: table  (original table, dupe)
            // 3: number (key)
            // 4: table (value)
            // 5: number (key)

            // No we have access to the table key at index -1 and the table value at -2
            // It's up to the 'callable' to fetch the key/value from the stack
            // This makes dealing with different types easier
            // TODO: Make it so you don't need to pass the 'this' pointer to both the key & value struct
            LuaTableReference table{.key = {this}, .value = {this}};
            if (callable(table))
            {
                lua_pop(get_lua_state(), 3);
                // 1: table (original table)
                // 2: table  (original table, dupe)
                break;
            }
            else
            {
                lua_pop(get_lua_state(), 2);
                // 1: table (original table)
                // 2: table  (original table, dupe)
                // 3: number (key)
            }
        }

        lua_pop(get_lua_state(), 1);
        // 1: table (original table)
        if (m_num_tables_being_iterated <= 1)
        {
            lua_remove(get_lua_state(), 1);
        }

        // 1: stack empty

        --m_num_tables_being_iterated;
    }

    auto Lua::is_function() const -> bool
    {
        return lua_isfunction(get_lua_state(), 1);
    }

    auto Lua::is_global_function(std::string_view global_function_name) const -> bool
    {
        // Push on to the stack the value of global_function_name
        // lua_getglobal also returns the type
        int type_of_global = lua_getglobal(get_lua_state(), global_function_name.data());

        // Clean the stack by popping the global of the stack
        lua_pop(get_lua_state(), 1);

        return type_of_global == LUA_TFUNCTION;
    }

    auto Lua::prepare_function_call(std::string_view global_function_name) const -> void
    {
        // Put global on the stack & verify that it's a function and if it's not then simply do some stack cleanup
        if (int status = lua_getglobal(get_lua_state(), global_function_name.data()); status != LUA_TFUNCTION)
        {
            // Intentionally only cleaning the stack if the type wasn't TFUNCTION
            // This is because the TFUNCTION is needed on the stack later
            lua_pop(get_lua_state(), 1);
            throw std::runtime_error{fmt::format("[Lua::prepare_function_call] lua_getglobal returned !LUA_TFUNCTION, type returned: {}",
                                                 lua_typename(get_lua_state(), status))};
        }
    }

    auto Lua::call_function(int32_t num_params, int32_t num_return_values) const -> void
    {
        lua_State* L = get_lua_state();

        // Insert error handler below the function and its arguments
        // Stack before: [... func arg1 arg2 ... argN] (top = N items)
        // We need: [... err_handler func arg1 arg2 ... argN]

        // Get absolute index of where the function currently is
        int top_before = lua_gettop(L);
        int func_abs_idx = top_before - num_params;  // func is at this absolute position

        // Push error handler, then insert it below the function
        lua_pushcfunction(L, pcall_error_handler);
        // Now stack is: [... func arg1 ... argN err_handler]
        // Insert at func position to shift everything up
        lua_insert(L, func_abs_idx);
        // Now stack is: [... err_handler func arg1 ... argN]

        // Error handler is at func_abs_idx (absolute)
        int err_handler_abs = func_abs_idx;

        if (int status = lua_pcall(L, num_params, num_return_values, err_handler_abs); status != LUA_OK)
        {
            // Error message (with traceback) is on stack, error handler already notified callbacks
            const char* msg = lua_tostring(L, -1);
            std::string error_msg = msg ? msg : "(no error message)";
            lua_pop(L, 1);  // Pop error message
            lua_remove(L, err_handler_abs);  // Remove error handler
            // Don't use resolve_status_message here - it would pop the stack again
            throw std::runtime_error{fmt::format("[Lua::call_function] lua_pcall returned {} => {}", status_to_string(status), error_msg)};
        }

        // Remove error handler (it's below any return values)
        // Stack after pcall success: [... err_handler ret1 ret2 ... retN]
        lua_remove(L, err_handler_abs);
    }

    auto Lua::call_function(std::string_view global_function_name, int32_t num_params, int32_t num_return_values) const -> void
    {
        prepare_function_call(global_function_name);
        call_function(num_params, num_return_values);
    }

    auto Lua::prepare_new_table(int32_t preallocate_sequential_elements, int32_t preallocate_other_elements) const -> Table
    {
        lua_createtable(get_lua_state(), preallocate_sequential_elements, preallocate_other_elements);
        Table table{*this};
        return table;
    }

    auto Lua::prepare_new_metatable(const char* metatable_name) const -> Table
    {
        luaL_newmetatable(get_lua_state(), metatable_name);
        Table table{*this};
        return table;
    }

    auto Lua::get_metatable(const char* metatable_name) const -> Table
    {
        luaL_getmetatable(get_lua_state(), metatable_name);
        Table table{*this};
        return table;
    }

    auto Lua::new_thread() const -> Lua&
    {
        auto new_lua_thread = lua_newthread(get_lua_state());
        return *lua_instances.emplace(new_lua_thread, std::make_unique<Lua>(new_lua_thread)).first->second;
    }

    auto Lua::construct_metamethods_object(const OptionalMetaMethods& metamethods, std::optional<std::string_view> metatable_name) const -> void
    {
        if (metamethods.has_value())
        {
            Type::MetaMethodContainer c{
                    metamethods->gc,
                    metamethods->index,
                    metamethods->new_index,
                    metamethods->call,
                    metamethods->equal,
                    metamethods->length,
            };
            new_metatable<Type::MetaMethodContainer>(metatable_name, std::nullopt);
            transfer_stack_object(std::move(c), metatable_name, std::nullopt, true);
        }
        else
        {
            Type::MetaMethodContainer c{};
            new_metatable<Type::MetaMethodContainer>(metatable_name, std::nullopt);
            transfer_stack_object(std::move(c), metatable_name, std::nullopt, true);
        }
    }

    auto Lua::is_userdata(int32_t force_index) const -> bool
    {
        return lua_isuserdata(get_lua_state(), force_index);
    }

    auto RC_LMS_API handle_error(lua_State* lua_state, const std::string& error_message) -> const std::string
    {
        luaL_traceback(lua_state, lua_state, error_message.c_str(), 0);

        std::string final_message{};
        if (lua_isstring(lua_state, -1))
        {
            final_message = lua_tostring(lua_state, -1);
        }

        if (final_message.empty() || final_message == error_message + "\nstack traceback:")
        {
            final_message = fmt::format("{}\nNo traceback", error_message);
        }

        // Notify error callbacks before clearing the stack so they can inspect state
        notify_error_callbacks(lua_state, error_message, final_message);

        // Clearing the Lua stack because we're treating errors as non-fatal, but we still need to clean everything up
        lua_settop(lua_state, 0);
        return final_message;
    }

    auto throw_error(lua_State* lua_state, const std::string& error_message) -> void
    {
        auto final_message = handle_error(lua_state, error_message);

        lua_state_errors.emplace(lua_state, final_message);
        luaL_error(lua_state, final_message.c_str());
    }

    auto new_state() -> Lua&
    {
        auto new_lua_state = luaL_newstate();
        return *lua_instances.emplace(new_lua_state, std::make_unique<Lua>(new_lua_state)).first->second;
    }

    auto RC_LMS_API push_pcall_error_handler(lua_State* L) -> int
    {
        return push_error_handler(L);
    }

    // dumpstack function from: https://stackoverflow.com/questions/59091462/from-c-how-can-i-print-the-contents-of-the-lua-stack/59097940#59097940
    // it's been modified slightly
    auto dump_stack(lua_State* lua_state, const char* message) -> void
    {
        printf_s("\n\nLUA Stack dump -> START------------------------------\n%s\n", message);
        int top = lua_gettop(lua_state);
        for (int i = 1; i <= top; i++)
        {
            printf_s("%d\t%s\t", i, luaL_typename(lua_state, i));
            switch (lua_type(lua_state, i))
            {
            case LUA_TNUMBER:
                printf_s("%g", lua_tonumber(lua_state, i));
                break;
            case LUA_TSTRING:
                printf_s("%s", lua_tostring(lua_state, i));
                break;
            case LUA_TBOOLEAN:
                printf_s("%s", (lua_toboolean(lua_state, i) ? "true" : "false"));
                break;
            case LUA_TNIL:
                printf_s("nil");
                break;
            case LUA_TFUNCTION:
                printf_s("function");
                break;
            default:
                printf_s("%p", lua_topointer(lua_state, i));
                break;
            }
            printf_s("\n");
        }
        printf_s("\nLUA Stack dump -> END----------------------------\n\n");
    }

    // https://stackoverflow.com/questions/59091462/from-c-how-can-i-print-the-contents-of-the-lua-stack/59097940#59097940
    auto get_stack_dump(lua_State* lua_state, const char* message) -> std::string
    {
        auto out_message = fmt::format("\n\nLUA Stack dump -> START------------------------------\n{}\n", message);
        int top = lua_gettop(lua_state);
        for (int i = 1; i <= top; i++)
        {
            out_message.append(fmt::format("{}\t{}\t", i, luaL_typename(lua_state, i)));
            switch (lua_type(lua_state, i))
            {
            case LUA_TNUMBER:
                out_message.append(fmt::format("{}", lua_tonumber(lua_state, i)));
                break;
            case LUA_TSTRING:
                out_message.append(fmt::format("{}", lua_tostring(lua_state, i)));
                break;
            case LUA_TBOOLEAN:
                out_message.append(fmt::format("{}", (lua_toboolean(lua_state, i) ? "true" : "false")));
                break;
            case LUA_TNIL:
                out_message.append("nil");
                break;
            case LUA_TFUNCTION:
                out_message.append("function");
                break;
            default:
                out_message.append(fmt::format("{}", lua_topointer(lua_state, i)));
                break;
            }
            out_message.append("\n");
        }
        out_message.append("\nLUA Stack dump -> END----------------------------\n\n");
        return out_message;
    }

    auto process_lua_function(lua_State* lua_state) -> int
    {
        const size_t func_id = lua_tointeger(lua_state, lua_upvalueindex(1));
        const Lua::LuaFunctionType func_type = static_cast<Lua::LuaFunctionType>(lua_tointeger(lua_state, lua_upvalueindex(2)));

        if (func_type == Lua::LuaFunctionType::Local && !lua_isuserdata(lua_state, 1))
        {
            throw_error(lua_state, fmt::format("[process_lua_function] A function requiring userdata as param #1 was called without userdata at param #1"));
        }

        if (!lua_instances.contains(lua_state))
        {
            throw_error(lua_state, fmt::format("[process_lua_function] The lua state '{}' has no instance inside lua_instances unordered map", (void*)lua_state));
        }

        Lua& data_owner = *lua_instances.find(lua_state)->second;

        if (func_id > lua_functions.size())
        {
            throw_error(lua_state, fmt::format("[process_lua_function] There was no global function with the id '{}' inside the lua_functions vector", func_id));
        }

        if (!lua_functions[func_id].has_value())
        {
            throw_error(lua_state, "[process_lua_function] The optional was empty");
        }

        return TRY(lua_state, [&] {
            auto return_value = lua_functions[func_id].value()(data_owner);
            for (const auto& post_process_callback : data_owner.m_post_function_process_callbacks)
            {
                post_process_callback(data_owner);
            }
            return return_value;
        });
    }
} // namespace RC::LuaMadeSimple
