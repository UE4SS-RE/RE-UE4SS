#pragma once

#include <LuaMadeSimple/LuaMadeSimple.hpp>
#include <string>

namespace RC::LuaMadeSimple::Type
{
    enum class IsFinal
    {
        Yes,
        No
    };

    enum class Operation
    {
        Get,                // Return to Lua some value that it can work with
        GetNonTrivialLocal, // Return to Lua a non-trivial type that's been made trivial (example: StructProperty as a Lua table)
        Set,                // Take from Lua some value
        GetParam,           // Return to Lua a container of a value that's supposed to be used as a Lua function param
    };

    // Container for MetaMethods after the object has been fully constructed.
    // Safe to use only after construction, and can be used without casting to proper type.
    struct RC_LMS_API MetaMethodContainer
    {
        LuaMadeSimple::Lua::MetaMethods metamethods;
    };

    class RC_LMS_API BaseObject
    {
      private:
        const char* m_object_name;

        // Storage for metamethods during object construction.
        // Is available & usable after construction only if cast to the proper type.
        LuaMadeSimple::Lua::MetaMethods metamethods_before_construction;

      protected:
        explicit BaseObject(const char* object_name) : m_object_name(object_name)
        {
        }

      public:
        auto get_object_name() const -> const char*
        {
            return m_object_name;
        }

        auto get_metamethods() const -> const Lua::MetaMethods&
        {
            return metamethods_before_construction;
        }

        auto get_metamethods() -> Lua::MetaMethods&
        {
            return metamethods_before_construction;
        }
    };

    // Only used for non-local types
    // The object gets copied into here
    template <typename ObjectType>
    class LocalObject : public BaseObject
    {
      private:
        ObjectType m_local_storage;

      protected:
        LocalObject(const char* object_name, ObjectType&& object) : BaseObject(object_name), m_local_storage(object)
        {
        }

      public:
        auto get_local_cpp_object() -> ObjectType&
        {
            return m_local_storage;
        }
    };

    // Only used for remote types (data stored elsewhere, only pointer is stored locally to this object)
    template <typename ObjectType>
    class RemoteObject : public BaseObject
    {
      private:
        ObjectType* m_cpp_object;

      protected:
        RemoteObject(const char* object_name, ObjectType* object) : BaseObject(object_name), m_cpp_object(object)
        {
        }

      public:
        RemoteObject() = delete;

        // For constructing a RemoteObject with a trivial templated type
        // This function cannot be used if you want to inherit from RemoteObject
        auto static construct(const LuaMadeSimple::Lua& lua, ObjectType* object_ptr) -> const LuaMadeSimple::Lua::Table
        {
            RemoteObject<ObjectType> lua_object{object_ptr};

            auto metatable_name = "TrivialObject";

            LuaMadeSimple::Lua::Table table = lua.get_metatable(metatable_name);
            if (lua.is_nil(-1))
            {
                lua.discard_value(-1);
                lua.prepare_new_table();
                setup_member_functions<LuaMadeSimple::Type::IsFinal::Yes>(table);
                lua.new_metatable<RemoteObject<ObjectType>>(metatable_name, table.get_metamethods());
            }

            // Create object & surrender ownership to Lua
            lua.transfer_stack_object(std::move(lua_object), metatable_name, table.get_metamethods());

            return table;
        }

        // For constructing a RemoteObject with a non-trivial templated type that can have overrides
        // The non-trivial type must place a call to 'table.make_global'
        // The unused "construct_to" param can be used if you want to do something with meta tables potentially so the param stays for now
        auto static construct(const LuaMadeSimple::Lua& lua, [[maybe_unused]] BaseObject& construct_to) -> const LuaMadeSimple::Lua::Table
        {
            LuaMadeSimple::Lua::Table table = lua.prepare_new_table();

            // Setup functions that can be called on this object
            setup_member_functions<IsFinal::No>(table);

            return table;
        }

      public:
        template <IsFinal is_final>
        auto static setup_member_functions(LuaMadeSimple::Lua::Table& table) -> void
        {
            table.add_pair("GetAddress", [](const LuaMadeSimple::Lua& lua) -> int {
                lua_getiuservalue(lua.get_lua_state(), 1, 5);
                if (lua_toboolean(lua.get_lua_state(), -1))
                {
                    lua.throw_error("Call to RemoteObject:GetAddress on polymorphic type is not allowed, please override GetAddress.");
                }

                const RemoteObject& lua_object = lua.get_userdata<RemoteObject>();
                lua.set_integer(reinterpret_cast<uintptr_t>(lua_object.m_cpp_object));
                return 1;
            });

            table.add_pair("IsValid", [](const LuaMadeSimple::Lua& lua) -> int {
                lua_getiuservalue(lua.get_lua_state(), 1, 5);
                if (lua_toboolean(lua.get_lua_state(), -1))
                {
                    lua.throw_error("Call to RemoteObject:IsValid on polymorphic type is not allowed, please override IsValid.");
                }

                const RemoteObject& lua_object = lua.get_userdata<RemoteObject>();
                if (lua_object.get_remote_cpp_object())
                {
                    lua.set_bool(true);
                }
                else
                {
                    lua.set_bool(false);
                }
                return 1;
            });

            // Add things that are intended to be overridden later here
            // These will then be set only if this is the final object (nothing overrides it later)
            if constexpr (is_final == LuaMadeSimple::Type::IsFinal::Yes)
            {
                table.get_metamethods().create(LuaMadeSimple::Lua::MetaMethod::Index, [](const LuaMadeSimple::Lua& lua) -> int {
                    RemoteObject<ObjectType>& lua_object = lua.get_userdata<RemoteObject<ObjectType>>();
                    // lua_object.get_pusher_callable()(lua, lua_object.get_remote_cpp_object()); commented out, not implemented
                    return 1;
                });

                table.get_metamethods().create(LuaMadeSimple::Lua::MetaMethod::NewIndex, [](const LuaMadeSimple::Lua& lua) -> int {
                    return 0;
                });
            }
        }

        auto get_remote_cpp_object() const -> ObjectType*
        {
            return m_cpp_object;
        }
    };
} // namespace RC::LuaMadeSimple::Type
