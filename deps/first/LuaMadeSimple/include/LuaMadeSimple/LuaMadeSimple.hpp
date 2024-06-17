#pragma once

#include <format>
#include <functional>
#include <optional>

#include <LuaMadeSimple/Common.hpp>
#include <lua.hpp>
#include <fmt/core.h>

namespace RC::LuaMadeSimple
{
    // To be used in the future to decide whether it's safe to perform any Lua actions
    enum class GlobalState
    {
        Ready,
        Halt
    };

    // Dump the current state of the Lua stack
    auto RC_LMS_API dump_stack(lua_State*, const char* message = "") -> void;

    // Internal functions
    // All Lua <-> C++ calls go through these two C++ functions
    auto RC_LMS_API process_lua_function(lua_State* lua_state) -> int;

    /**
     * @tparam StackIndex Valid values are -1 for table key and -2 for table value
     */
    template <int32_t StackIndex>
    struct RC_LMS_API LuaTableData
    {
        const class Lua* lua{};

        [[nodiscard]] auto is_nil() -> bool
        {
            return lua_isnil(lua->get_lua_state(), StackIndex);
        }

        [[nodiscard]] auto is_string() const -> bool
        {
            return lua_type(lua->get_lua_state(), StackIndex) == LUA_TSTRING;
        }
        [[nodiscard]] auto get_string() const -> std::string_view
        {
            return lua_tostring(lua->get_lua_state(), StackIndex);
        }

        [[nodiscard]] auto is_number() const -> bool
        {
            return lua_isnumber(lua->get_lua_state(), StackIndex);
        }
        [[nodiscard]] auto get_number() const -> double
        {
            return lua_tonumber(lua->get_lua_state(), StackIndex);
        }
        [[nodiscard]] auto get_float(int32_t force_index = 1) -> float // Safe to use after a call to is_number
        {
            return static_cast<float>(lua_tonumber(lua->get_lua_state(), force_index));
        }

        [[nodiscard]] auto is_integer() const -> bool
        {
            return lua_isinteger(lua->get_lua_state(), StackIndex);
        }
        [[nodiscard]] auto get_integer() const -> int64_t
        {
            return lua_tointeger(lua->get_lua_state(), StackIndex);
        }

        [[nodiscard]] auto is_bool() const -> bool
        {
            return lua_isboolean(lua->get_lua_state(), StackIndex);
        }
        [[nodiscard]] auto get_bool() const -> bool
        {
            return lua_toboolean(lua->get_lua_state(), StackIndex);
        }

        [[nodiscard]] auto is_table() const -> bool
        {
            return lua_istable(lua->get_lua_state(), StackIndex);
        }
    };

    /**
     * Helper for dealing with Lua tables passed from Lua to C++
     */
    struct RC_LMS_API LuaTableReference
    {
        LuaTableData<-1> key;
        LuaTableData<-2> value;
    };

    using PostFunctionProcessCallback = void (*)(const Lua&);

    /**
     * Main helper for Lua
     * Use new_state() to start using the LuaMadeSimple system
     */
    class Lua
    {
        friend RC_LMS_API auto process_lua_function(lua_State* lua_state) -> int;

      private:
        lua_State* m_lua_state;

        std::vector<PostFunctionProcessCallback> m_post_function_process_callbacks;

        // The name of the current metatable that's being worked on
        mutable std::string_view m_working_metatable_name{};

        // Holds how many tables deep a nested table iteration operation currently is
        mutable int32_t m_num_tables_being_iterated{};

      public:
        RC_LMS_API Lua(lua_State* lua_state);
        RC_LMS_API ~Lua();

      public:
        /**
         * External definition of a Lua function
         */
        using LuaFunction = int (*)(const Lua&);

        /**
         * Internal definition of a Lua function
         */
        using LuaFunctionInternal = int (*)(lua_State*);

        /**
         * Type of Lua <-> C++ call interaction
         * Current unused but it's being kept during development until everything's been decided
         */
        enum class LuaFunctionType : int64_t
        {
            // Global function
            Global,

            // Function is a member of a table
            Table,

            // Locally scoped function
            // For example, a function in a table or a metamethod in a metatable
            Local,
        };

        /**
         * All metamethods
         * TODO: Add all of the metamethods... very few are currently supported
         */
        enum class MetaMethod
        {
            Gc,
            Index,
            NewIndex,
            Call,
            Equal,
            Length,
        };

        /**
         * Helper for setting up metamethods
         */
        struct MetaMethods
        {
            std::optional<LuaFunction> gc = std::nullopt;
            std::optional<LuaFunction> index = std::nullopt;
            std::optional<LuaFunction> new_index = std::nullopt;
            std::optional<LuaFunction> call = std::nullopt;
            std::optional<LuaFunction> equal = std::nullopt;
            std::optional<LuaFunction> length = std::nullopt;

            template <typename LuaCallable>
            auto create(MetaMethod metamethod, LuaCallable lua_callable) -> void
            {
                switch (metamethod)
                {
                case MetaMethod::Gc:
                    gc = lua_callable;
                    return;
                case MetaMethod::Index:
                    index = lua_callable;
                    return;
                case MetaMethod::NewIndex:
                    new_index = lua_callable;
                    return;
                case MetaMethod::Call:
                    call = lua_callable;
                    return;
                case MetaMethod::Equal:
                    equal = lua_callable;
                    return;
                case MetaMethod::Length:
                    length = lua_callable;
                }

                // TODO: use throw_error() here
                // Can't do it until there's a reference/ptr to the Lua class available here
            }
        };
        using OptionalMetaMethods = std::optional<MetaMethods>;

        // Current set of metamethods that are being worked on
        mutable OptionalMetaMethods m_working_metamethods{std::nullopt};

        class RC_LMS_API Registry
        {
          private:
            const Lua& m_lua_instance;

          public:
            Registry(const Lua& lua);

          public:
            auto get_lua_instance() const -> const Lua&;
            auto make_ref() const -> int32_t;
            auto get_string_ref(int32_t registry_index) const -> void;
            auto get_number_ref(int32_t registry_index) const -> void;
            auto get_integer_ref(int32_t registry_index) const -> void;
            auto get_bool_ref(int32_t registry_index) const -> void;
            auto get_function_ref(int32_t registry_index) const -> void;
            auto get_table_ref(int32_t registry_index) const -> void;
        };

      private:
        const Registry m_registry;

      public:
        class RC_LMS_API Table
        {
          private:
            const Lua& m_lua_instance;
            MetaMethods m_metamethods{};
            bool m_has_table_marked_for_pop{false};
            bool m_has_userdata{true};

          public:
            explicit Table(const Lua& lua_instance);

          private:
            /**
             * (internal)
             * Adds a function to a table as a value
             * @param function C++ function pointer (can be lambda)
             */
            auto add_function_value_internal(LuaFunction function) const -> void;
            auto push_field_name_and_value(std::string_view field_name) -> int32_t;

          public:
            auto get_lua_instance() const -> const Lua&;
            auto get_metamethods() -> MetaMethods&;

            /**
             * Manually adds a key to a table
             * Must call fuse_pair() when using add_key() & add_value()
             * @param key
             */
            template <typename KeyType>
            auto add_key(KeyType key) const -> void
            {
                if constexpr (std::is_same_v<KeyType, const char*> || std::is_same_v<KeyType, char*>)
                {
                    lua_pushstring(get_lua_instance().get_lua_state(), key);
                }
                else if constexpr (std::is_same_v<KeyType, int> || std::is_same_v<KeyType, size_t>)
                {
                    lua_pushinteger(get_lua_instance().get_lua_state(), key);
                }
                else if constexpr (std::is_same_v<KeyType, unsigned int>)
                {
                    lua_pushinteger(get_lua_instance().get_lua_state(), static_cast<int>(key));
                }
                else
                {
                    throw std::runtime_error{"Unsupported type for 'KeyType'"};
                }
            }

            /**
             * Manually adds a value to a table
             * Must call fuse_pair() when using add_key() & add_value()
             * @param value
             */
            template <typename ValueType>
            auto add_value(ValueType value) const -> void
            {
                if constexpr (std::is_same_v<ValueType, const char*> || std::is_same_v<ValueType, char*>)
                {
                    lua_pushstring(get_lua_instance().get_lua_state(), value);
                }
                else if constexpr (std::is_same_v<ValueType, bool>)
                {
                    lua_pushboolean(get_lua_instance().get_lua_state(), value);
                }
                else if constexpr (std::is_same_v<ValueType, int> || std::is_same_v<ValueType, long long>)
                {
                    lua_pushinteger(get_lua_instance().get_lua_state(), value);
                }
                else if constexpr (std::is_same_v<ValueType, unsigned int>)
                {
                    lua_pushinteger(get_lua_instance().get_lua_state(), static_cast<int>(value));
                }
                else if constexpr (std::is_same_v<ValueType, float> || std::is_same_v<ValueType, double>)
                {
                    lua_pushnumber(get_lua_instance().get_lua_state(), value);
                }
                else if constexpr (std::is_convertible_v<ValueType, LuaFunctionInternal>)
                {
                    lua_pushcfunction(get_lua_instance().get_lua_state(), value);
                }
                else if constexpr (std::is_convertible_v<ValueType, LuaFunction>)
                {
                    add_function_value_internal(value);
                }
                else if constexpr (std::is_same_v<ValueType, Userdata<typename ValueType::InnerType>>)
                {
                    get_lua_instance().transfer_stack_object<typename ValueType::InnerType>(std::move(value.inner_object), std::nullopt, value.table_metamethods);
                }
                else if constexpr (std::is_same_v<ValueType, SharedUserdata<typename ValueType::InnerType>>)
                {
                    get_lua_instance().share_heap_object(value.inner_object, value.table_metamethods);
                }
                else
                {
                    throw std::runtime_error{"Unsupported type for 'ValueType'"};
                }
            }

            /**
             * Adds a key/value pair to a table
             * @param key
             * @param value
             */
            template <typename KeyType, typename ValueType>
            auto add_pair(KeyType key, ValueType value) const -> void
            {
                // Key
                add_key(key);

                // Value
                add_value(value);

                lua_rawset(get_lua_instance().get_lua_state(), -3);
            }

            /**
             * Call this only if you manually created a pair for the table
             * A pair can be manually created either by
             * A. Calling add_key() / add_value()
             * or
             * B. Manually manipulating the stack
             *
             * Using add_pair() will automatically fuse the key/value
             */
            auto fuse_pair() -> void;

            /**
             * Converts a std::vector to a Lua table
             * @param convert_from_vector
             */
            template <typename VectorInnerType, typename VectorAllocator>
            auto vector_to_table(const std::vector<VectorInnerType, VectorAllocator>& convert_from_vector) -> void
            {
                for (size_t i = 0; i < convert_from_vector.size(); ++i)
                {
                    // Lua scripts expects table to be one-index so add 1 to the vector index
                    add_pair(i + 1, convert_from_vector[i]);
                }
            }

            /**
             * Make a table local
             * Does whatever is needed to make a local table
             * Must be called when you're done manipulatingthe table (manually or otherwise)
             */
            auto make_local() -> void;

            /**
             * Make a table global
             * Does whatever is needed to make a global table
             * Must be called when you're done manipulating the table (manually or otherwise)
             * @param table_name
             */
            auto make_global(std::string_view table_name) const -> void;

          public:
            // Finds a value corresponding to the key at the top of the Lua stack
            // Use the Lua::get_<type>() functions to retrieve the value (i.e: lua.get_integer())
            auto find_value() -> void;
            auto has_userdata() const -> bool;
            auto set_has_userdata(bool) -> void;

            // Simplified table field getters
            auto does_field_exist(std::string_view field_name) -> bool;
            auto get_string_field(std::string_view field_name, bool* has_error = nullptr) -> std::string_view;
            auto get_int_field(std::string_view field_name, bool* has_error = nullptr) -> int64_t;
            auto get_float_field(std::string_view field_name, bool* has_error = nullptr) -> float;
            auto get_table_field(std::string_view field_name, bool* has_error = nullptr) -> Lua::Table&;

            template <typename ObjectType>
            auto get_userdata_field(std::string_view field_name, bool* has_error = nullptr) -> ObjectType&
            {
                int pushed_type = push_field_name_and_value(field_name);
                if (pushed_type != LUA_TUSERDATA)
                {
                    if (has_error)
                    {
                        *has_error = true;
                    }
                    else
                    {
                        get_lua_instance().throw_error(fmt::format("[get_userdata_field] Attempted to get field: '{}' but the type was '{}'",
                                                                   field_name,
                                                                   lua_typename(get_lua_instance().get_lua_state(), pushed_type)));
                    }
                }

                return get_lua_instance().get_userdata<ObjectType>(-1);
            }
        };

      private:
        /**
         * Internal representation of how Lua should treat a C++ object
         */
        enum UserdataInternalType : int64_t
        {
            LuaOwnedStackObject,
            SharedHeapObject,
        };

      public:
        /**
         * Used as the 'value' param for LuaMadeSimple::Lua::Table::add_pair()
         */
        template <typename ObjectType>
        struct Userdata
        {
          public:
            // Used to retrieve the type when doing std::is_same_v with a Userdata type
            using InnerType = ObjectType;

            // Copy of object that you wish to turn into Lua userdata
            InnerType inner_object;

            // Metamethods that the userdata should contain
            const OptionalMetaMethods table_metamethods{std::nullopt};

          public:
            Userdata(InnerType&& object) : inner_object(std::move(object))
            {
            }
            Userdata(InnerType&& object, const MetaMethods metamethods) : inner_object(std::move(object)), table_metamethods(metamethods)
            {
            }
        };

        template <typename ObjectType>
        struct SharedUserdata
        {
          public:
            // Used to retrieve the type when doing std::is_same_v with a Userdata type
            using InnerType = ObjectType;

            // Copy of object that you wish to turn into Lua userdata
            // Reference of a shared_ptr (doesn't increase ref count)
            // The ref count is increased when it's passed to Lua
            // Does the ref count need to be increased here ?
            // I would say no because the original cannot exit scope until a copy has been passed to Lua (increasing the ref count)
            // So lets just save some performance here
            const std::shared_ptr<InnerType>& inner_object;

            // Metamethods that the userdata should contain
            const OptionalMetaMethods table_metamethods{std::nullopt};

          public:
            SharedUserdata(const std::shared_ptr<InnerType>& object) : inner_object(object)
            {
            }
            SharedUserdata(const std::shared_ptr<InnerType>& object, const MetaMethods metamethods) : inner_object(object), table_metamethods(metamethods)
            {
            }
        };

      public:
        RC_LMS_API auto status_to_string(int status) const -> std::string_view;
        RC_LMS_API auto resolve_status_message(int status, bool from_lua_only = false) const -> std::string;

      private:
        // Returns whether the '__gc' metamethod should be automatically created
        RC_LMS_API auto add_metamethods(const MetaMethods&, const Table& metatable) const -> bool;

      public:
        // Debug function
        RC_LMS_API auto dump_stack(const char* message = "") const -> void;

        RC_LMS_API auto handle_error(const std::string&) const -> const std::string;
        RC_LMS_API auto throw_error(const std::string&) const -> void;

        template <typename CodeToTry>
        auto constexpr TRY(CodeToTry code_to_try) const -> void
        {
            try
            {
                code_to_try();
            }
            catch (std::exception& e)
            {
                throw_error(e.what());
            }
        }

        template <typename CodeToTry>
        auto constexpr TRY(CodeToTry code_to_try) -> void
        {
            TRY(code_to_try);
        }

        RC_LMS_API auto execute_file(std::string_view) const -> void;
        RC_LMS_API auto execute_file(std::wstring_view) const -> void;
        RC_LMS_API auto execute_string(std::string_view) const -> void;
        RC_LMS_API auto execute_string(std::wstring_view) const -> void;
        RC_LMS_API auto open_all_libs() const -> void;
        RC_LMS_API auto set_hook(lua_Hook, int32_t mask, int32_t count) const -> void;

        RC_LMS_API auto registry() const -> const Registry&;
        RC_LMS_API auto rawgeti(int32_t idx, lua_Integer n) const -> int32_t;

        RC_LMS_API auto register_post_function_process_callback(PostFunctionProcessCallback) -> void;
        RC_LMS_API auto register_function(const std::string& name, const LuaFunction&) const -> void;

        RC_LMS_API auto get_lua_state() const -> lua_State*;
        RC_LMS_API auto get_stack_size() const -> int32_t;
        RC_LMS_API auto get_type_string() const -> std::string_view;

        // Figure out what stack index to use
        // Using -1 (top of stack) doesn't work well for function params because calling any get_<type>() function will then return the params backwards
        // Using 1 (bottom of stack) doesn't work well for Lua tables if it's not the last param (or something, I don't remember what the actual problem was)
        // Answer: Probably use index 1 and duplicate the table entry (putting it at the top of the stack) and use -1 for tables specifically
        // Currently using what's described in "Answer"
        // Might need to implement some of the UE4SS functions before knowing for sure if this will work

        // Use discard_value() if you have values on the stack that you require to be gone
        RC_LMS_API auto discard_value(int32_t force_index = 1) const -> void;

        // Copy value from from_index to force_index, shifting values on stack to top
        RC_LMS_API auto insert_value(int32_t from_index = -1, int32_t force_index = 1) const -> void;

        [[nodiscard]] RC_LMS_API auto is_nil(int32_t force_index = 1) const -> bool;
        RC_LMS_API auto set_nil() const -> void;

        [[nodiscard]] RC_LMS_API auto is_string(int32_t force_index = 1) const -> bool;
        [[nodiscard]] RC_LMS_API auto get_string(int32_t force_index = 1) const -> std::string_view;
        RC_LMS_API auto set_string(std::string_view) const -> void;

        // is_number == lua_isnumber, which returns true if the value is a number or a string convertible to a number
        [[nodiscard]] RC_LMS_API auto is_number(int32_t force_index = 1) const -> bool;
        [[nodiscard]] RC_LMS_API auto get_number(int32_t force_index = 1) const -> double;
        RC_LMS_API auto set_number(double number) const -> void;
        [[nodiscard]] RC_LMS_API auto get_float(int32_t force_index = 1) const -> float; // Safe to use after a call to is_number
        RC_LMS_API auto set_float(float number) const -> void;

        [[nodiscard]] RC_LMS_API auto is_integer(int32_t force_index = 1) const -> bool;
        [[nodiscard]] RC_LMS_API auto get_integer(int32_t force_index = 1) const -> int64_t;
        RC_LMS_API auto set_integer(int64_t number) const -> void;

        [[nodiscard]] RC_LMS_API auto is_bool(int32_t force_index = 1) const -> bool;
        [[nodiscard]] RC_LMS_API auto get_bool(int32_t force_index = 1) const -> bool;
        RC_LMS_API auto set_bool(bool bool_value) const -> void;

        [[nodiscard]] RC_LMS_API auto is_table(int32_t force_index = 1) const -> bool;
        [[nodiscard]] RC_LMS_API auto get_table() const -> Table;

        using ForEachInTableCallable = std::function<bool(LuaTableReference)>;

        // If you use a lambda then make absolutely sure to capture the 'LuaMadeSimple::Lua' object by reference
        // If you don't then it will be improperly copied and things, including nested for_each calls, will break
        RC_LMS_API auto for_each_in_table(const ForEachInTableCallable& callable) const -> void;

        [[nodiscard]] RC_LMS_API auto is_function() const -> bool;
        [[nodiscard]] RC_LMS_API auto is_global_function(std::string_view global_function_name) const -> bool;
        // Use the get_<type>() functions to retrieve return values
        RC_LMS_API auto prepare_function_call(std::string_view global_function_name) const -> void;
        RC_LMS_API auto call_function(int32_t num_params, int32_t num_return_values) const -> void;
        // Convenience function for calling less complicated Lua functions
        RC_LMS_API auto call_function(std::string_view global_function_name, int32_t num_params, int32_t num_return_values) const -> void;

        RC_LMS_API auto prepare_new_table(int32_t preallocate_sequential_elements = 0, int32_t preallocate_other_elements = 0) const -> Table;
        RC_LMS_API auto prepare_new_metatable(const char* metatable_name) const -> Table;
        RC_LMS_API auto get_metatable(const char* metatable_name) const -> Table;

        RC_LMS_API auto new_thread() const -> Lua&;

      private:
        RC_LMS_API auto construct_metamethods_object(const OptionalMetaMethods&, std::optional<std::string_view> metatable_name = std::nullopt) const -> void;

      public:
        // Create a new metatable and put references to member functions table in it
        // A metatable is automatically attached with a __gc metamethod that calls the ObjectType destructor
        // More metamethods can be supplied via the 'metamethods' parameter (see the 'MetaMethods' struct)
        template <typename ObjectType>
        auto new_metatable(std::optional<std::string_view> metatable_name = std::nullopt, OptionalMetaMethods metamethods = std::nullopt) const -> void
        {
            auto create_auto_gc_metamethod = [](Table& metatable) {
                metatable.add_pair("__gc", [](lua_State* lua_state) -> int {
                    auto* userdata = static_cast<ObjectType*>(lua_touserdata(lua_state, 1));
                    userdata->~ObjectType();

                    return 0;
                });
            };

            bool custom_gc_method{};

            if (metatable_name.has_value() && metamethods.has_value())
            {
                // At least some metamethods might have been provided

                Table metatable = prepare_new_metatable(metatable_name.value().data());

                custom_gc_method = add_metamethods(metamethods.value(), metatable);
                if (!custom_gc_method)
                {
                    // There may have been some metamethods provided but '__gc' wasn't one of them so we can safely attach our own
                    create_auto_gc_metamethod(metatable);
                }
            }
            else
            {
                auto no_gc_metatable = metatable_name.has_value() ? metatable_name.value().data() : "AutoGCMetatable";
                // No metamethods of any kind was provided
                Table metatable = get_metatable(no_gc_metatable);
                if (is_nil(-1))
                {
                    discard_value(-1);
                    prepare_new_metatable(no_gc_metatable);
                    // We know that the user didn't provide their own '__gc' metamethod so we can safely attach our own
                    create_auto_gc_metamethod(metatable);
                }
            }
            if (is_table(-2)) // if called from construct_metamethods_object this will be false
            {
                lua_rotate(get_lua_state(), -2, 1);
                lua_rawseti(get_lua_state(), -2, 1);
            }
        }

        // Transfer ownership of a C++ object stored on the stack to Lua via userdata
        // More metamethods can be supplied via the 'metamethods' parameter (see the 'MetaMethods' struct)
        template <typename ObjectType>
        auto transfer_stack_object(ObjectType&& object,
                                   std::optional<std::string_view> metatable_name = std::nullopt,
                                   OptionalMetaMethods metamethods = std::nullopt,
                                   bool is_metamethod_container = false) const -> void
        {
            auto* userdata = static_cast<ObjectType*>(lua_newuserdatauv(get_lua_state(), sizeof(ObjectType), 5));

            // Set a user value that tells 'get_userdata()' that this userdata is a stack object owned by lua
            set_integer(UserdataInternalType::LuaOwnedStackObject);
            lua_setiuservalue(get_lua_state(), -2, 2);

            // Attach metatable to useradata for reference
            if (lua_rawgeti(get_lua_state(), -2, 1) == LUA_TTABLE)
            {
                lua_setiuservalue(get_lua_state(), -2, 3);
            }
            else
            {
                discard_value(-1);
            }

            if (!is_metamethod_container)
            {
                // Set a user value that contains the metamethod handlers
                if (lua_rawgeti(get_lua_state(), -2, 2) == LUA_TNIL)
                {
                    discard_value(-1);
                    construct_metamethods_object(metamethods, "MetaMethodContainer");
                    lua_pushvalue(get_lua_state(), -1);
                    lua_rawseti(get_lua_state(), -4, 2);
                }
                lua_setiuservalue(get_lua_state(), -2, 4);
            }

            // Set a user value that determines whether this is a polymorphic type.
            // This is used to determine if 'GetAddress' and 'GetValid' needs to be overridden by the derived type.
            if constexpr (std::is_polymorphic_v<ObjectType>)
            {
                set_bool(true);
            }
            else
            {
                set_bool(false);
            }
            lua_setiuservalue(get_lua_state(), -2, 5);

            new (userdata) ObjectType(std::move(object));

            // Remove dangling table
            lua_remove(get_lua_state(), -2);
            luaL_getmetatable(get_lua_state(), metatable_name.has_value() ? metatable_name.value().data() : "AutoGCMetatable");
            lua_setmetatable(get_lua_state(), -2);
        }

        // Share a heap object (std::shared_ptr) with Lua via userdata
        // A metatable is automatically attached with a __gc metamethod that calls the ObjectType destructor
        // More metamethods can be supplied via the 'metamethods' parameter (see the 'MetaMethods' struct)
        template <typename ObjectType>
        auto share_heap_object(const std::shared_ptr<ObjectType>& object,
                               std::optional<std::string_view> metatable_name = std::nullopt,
                               OptionalMetaMethods metamethods = std::nullopt) const -> void
        {
            auto create_auto_gc_metamethod = [](Table& metatable) {
                metatable.add_pair("__gc", [](lua_State* lua_state) -> int {
                    auto* userdata = static_cast<std::shared_ptr<ObjectType>*>(lua_touserdata(lua_state, 1));
                    userdata->~shared_ptr<ObjectType>();

                    return 0;
                });
            };

            bool custom_gc_method{};

            if (metatable_name.has_value() && metamethods.has_value())
            {
                // At least some metamethods might have been provided

                m_working_metatable_name = metatable_name.value();
                m_working_metamethods = metamethods;

                Table metatable = prepare_new_metatable(m_working_metatable_name.data());

                custom_gc_method = add_metamethods(metamethods.value(), metatable);
                if (!custom_gc_method)
                {
                    // There may have been some metamethods provided but '__gc' wasn't one of them so we can safely attach our own
                    create_auto_gc_metamethod(metatable);
                }
            }
            else
            {
                // No metamethods of any kind was provided

                Table metatable = prepare_new_metatable("AutoGCMetatable");

                // We know that the user didn't provide their own '__gc' metamethod so we can safely attach our own
                create_auto_gc_metamethod(metatable);
            }

            auto* userdata = static_cast<std::shared_ptr<ObjectType>*>(lua_newuserdatauv(get_lua_state(), sizeof(std::shared_ptr<ObjectType>), 2));

            // Set a user value that tells 'get_userdata()' that this userdata is a shared heap object
            set_integer(UserdataInternalType::SharedHeapObject);
            lua_setiuservalue(get_lua_state(), -2, 2);

            new (userdata) std::shared_ptr<ObjectType>(object);

            // Remove dangling table
            lua_remove(get_lua_state(), -2);
            luaL_getmetatable(get_lua_state(), metatable_name.has_value() ? metatable_name.value().data() : "AutoGCMetatable");
            lua_setmetatable(get_lua_state(), -2);
        }

        [[nodiscard]] RC_LMS_API auto is_userdata(int32_t force_index = 1) const -> bool;

        template <typename ObjectType>
        [[nodiscard]] auto get_userdata(int32_t force_index = 1, bool preserve_stack = false) const -> ObjectType&
        {
            lua_getiuservalue(get_lua_state(), force_index, 2);
            int64_t userdata_internal_type = get_integer(-1);

            if (userdata_internal_type == UserdataInternalType::LuaOwnedStackObject)
            {
                // Stack object owned by Lua
                auto& object = *static_cast<ObjectType*>(lua_touserdata(get_lua_state(), force_index));
                if (!preserve_stack)
                {
                    lua_remove(get_lua_state(), force_index);
                }
                return object;
            }
            else if (userdata_internal_type == UserdataInternalType::SharedHeapObject)
            {
                // Shared object (shared_ptr)
                auto& object = **static_cast<std::shared_ptr<ObjectType>*>(lua_touserdata(get_lua_state(), force_index));
                if (!preserve_stack)
                {
                    lua_remove(get_lua_state(), force_index);
                }
                return object;
            }
            else [[unlikely]]
            {
                luaL_traceback(get_lua_state(),
                               get_lua_state(),
                               "[get_userdata] The user value 'userdata_internal_type' corresponding to this userdata was invalid\"",
                               0);
                throw std::runtime_error{"See traceback"};
            }
        }
    };

    RC_LMS_API auto handle_error(lua_State*, const std::string&) -> const std::string;
    RC_LMS_API auto throw_error(lua_State*, const std::string&) -> void;
    [[nodiscard]] RC_LMS_API auto new_state() -> Lua&;

    template <typename CodeToTry>
    auto constexpr TRY(lua_State* lua_state, CodeToTry code_to_try) -> int
    {
        try
        {
            return code_to_try();
        }
        catch (std::exception& e)
        {
            throw_error(lua_state, e.what());
            return 0;
        }
    }
} // namespace RC::LuaMadeSimple
