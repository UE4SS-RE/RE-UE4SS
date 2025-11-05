#pragma once

#include <LuaType/LuaUObject.hpp>
#include <Helpers/String.hpp>
#include <Unreal/Core/Containers/FString.hpp>
#include <Unreal/Core/Containers/FUtf8String.hpp>
#include <Unreal/Core/Containers/FAnsiString.hpp>

namespace RC::LuaType
{
    // Base template for all string types
    template<typename UnrealStringType, typename StringNameType>
    class TLuaStringBase : public LocalObjectBase<UnrealStringType, StringNameType>
    {
        using BaseClass = LocalObjectBase<UnrealStringType, StringNameType>;
        
    protected:
        explicit TLuaStringBase(UnrealStringType* object) 
            : BaseClass(std::move(UnrealStringType(*object)))
        {
        }

    public:
        TLuaStringBase() = delete;

        static auto construct(const LuaMadeSimple::Lua& lua, UnrealStringType* unreal_object) -> const LuaMadeSimple::Lua::Table
        {
            TLuaStringBase lua_object{unreal_object};

            auto metatable_name = StringNameType::ToString();

            LuaMadeSimple::Lua::Table table = lua.get_metatable(metatable_name);
            if (lua.is_nil(-1))
            {
                lua.discard_value(-1);
                lua.prepare_new_table();
                setup_metamethods(lua_object);
                setup_member_functions<LuaMadeSimple::Type::IsFinal::Yes>(lua, table);
                lua.new_metatable<TLuaStringBase>(metatable_name, lua_object.get_metamethods());
            }

            // Transfer the object & its ownership fully to Lua
            lua.transfer_stack_object(std::move(lua_object), metatable_name, lua_object.get_metamethods());

            return table;
        }

        static auto construct(const LuaMadeSimple::Lua& lua, BaseClass& construct_to) -> const LuaMadeSimple::Lua::Table
        {
            LuaMadeSimple::Lua::Table table = LuaMadeSimple::Type::RemoteObject<UnrealStringType>::construct(lua, construct_to);

            setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(lua, table);

            setup_metamethods(construct_to);

            return table;
        }

    private:
        static auto setup_metamethods(BaseClass& base_object) -> void
        {

        }

        template <LuaMadeSimple::Type::IsFinal is_final>
        static auto setup_member_functions(const LuaMadeSimple::Lua& lua, const LuaMadeSimple::Lua::Table& table) -> void
        {
            table.add_pair("ToString", [](const LuaMadeSimple::Lua& lua) -> int {
                auto& lua_object = lua.get_userdata<TLuaStringBase>();
                push_string_to_lua(lua, lua_object.get_local_cpp_object());
                return 1;
            });

            table.add_pair("Empty", [](const LuaMadeSimple::Lua& lua) -> int {
                auto& lua_object = lua.get_userdata<TLuaStringBase>();
                lua_object.get_local_cpp_object().Empty();
                return 0;
            });

            table.add_pair("Clear", [](const LuaMadeSimple::Lua& lua) -> int {
                auto& lua_object = lua.get_userdata<TLuaStringBase>();
                lua_object.get_local_cpp_object().Empty();
                return 0;
            });

            table.add_pair("Len", [](const LuaMadeSimple::Lua& lua) -> int {
                auto& lua_object = lua.get_userdata<TLuaStringBase>();
                lua.set_integer(lua_object.get_local_cpp_object().Len());
                return 1;
            });

            table.add_pair("IsEmpty", [](const LuaMadeSimple::Lua& lua) -> int {
                auto& lua_object = lua.get_userdata<TLuaStringBase>();
                lua.set_bool(lua_object.get_local_cpp_object().IsEmpty());
                return 1;
            });

            table.add_pair("Append", [](const LuaMadeSimple::Lua& lua) -> int {
                auto& lua_object = lua.get_userdata<TLuaStringBase>(1, true); // preserve_stack = true

                if (lua.is_string(2))
                {
                    std::string_view str_view = lua.get_string(2);
                    append_string(lua_object.get_local_cpp_object(), str_view);
                }
                else if (lua.is_userdata(2))
                {
                    auto& other = lua.get_userdata<TLuaStringBase>(2, true); // preserve_stack = true
                    append_from_object(lua_object.get_local_cpp_object(), other.get_local_cpp_object());
                }
                else
                {
                    lua.throw_error("Append requires a string argument");
                }

                return 0;
            });

            table.add_pair("Find", [](const LuaMadeSimple::Lua& lua) -> int {
                auto& lua_object = lua.get_userdata<TLuaStringBase>(1, true); // preserve_stack = true

                if (!lua.is_string(2))
                {
                    lua.throw_error("Find requires a string argument");
                }

                std::string_view search = lua.get_string(2);
                int32_t result = find_substring(lua_object.get_local_cpp_object(), search);

                if (result == Unreal::INDEX_NONE)
                {
                    lua.set_nil();
                }
                else
                {
                    lua.set_integer(result + 1); // Convert to 1-based Lua indexing
                }

                return 1;
            });

            table.add_pair("StartsWith", [](const LuaMadeSimple::Lua& lua) -> int {
                auto& lua_object = lua.get_userdata<TLuaStringBase>(1, true); // preserve_stack = true

                if (!lua.is_string(2))
                {
                    lua.throw_error("StartsWith requires a string argument");
                }

                std::string_view prefix = lua.get_string(2);
                bool result = starts_with(lua_object.get_local_cpp_object(), prefix);
                lua.set_bool(result);

                return 1;
            });

            table.add_pair("EndsWith", [](const LuaMadeSimple::Lua& lua) -> int {
                auto& lua_object = lua.get_userdata<TLuaStringBase>(1, true); // preserve_stack = true

                if (!lua.is_string(2))
                {
                    lua.throw_error("EndsWith requires a string argument");
                }

                std::string_view suffix = lua.get_string(2);
                bool result = ends_with(lua_object.get_local_cpp_object(), suffix);
                lua.set_bool(result);

                return 1;
            });

            table.add_pair("ToUpper", [](const LuaMadeSimple::Lua& lua) -> int {
                auto& lua_object = lua.get_userdata<TLuaStringBase>();
                auto upper = to_upper(lua_object.get_local_cpp_object());
                construct(lua, &upper);
                return 1;
            });

            table.add_pair("ToLower", [](const LuaMadeSimple::Lua& lua) -> int {
                auto& lua_object = lua.get_userdata<TLuaStringBase>();
                auto lower = to_lower(lua_object.get_local_cpp_object());
                construct(lua, &lower);
                return 1;
            });

            if constexpr (is_final == LuaMadeSimple::Type::IsFinal::Yes)
            {
                table.add_pair("type", [](const LuaMadeSimple::Lua& lua) -> int {
                    lua.set_string(StringNameType::ToString());
                    return 1;
                });
            }
        }

        // Specialized string operations for different string types
        
        // FString specializations
        static void push_string_to_lua(const LuaMadeSimple::Lua& lua, const Unreal::FString& str)
        {
            const TCHAR* string_data = *str;
            if (string_data)
            {
                std::string utf8_string = to_utf8_string(string_data);
                lua.set_string(utf8_string.data(), utf8_string.length());
            }
            else
            {
                lua.set_string("", 0);
            }
        }

        static std::string get_string_from_object(const Unreal::FString& str)
        {
            return to_utf8_string(*str);
        }

        static void append_string(Unreal::FString& str, std::string_view append)
        {
            auto converted = ensure_str(std::string(append));
            str.Append(converted.c_str());
        }

        static void append_from_object(Unreal::FString& str, const Unreal::FString& other)
        {
            str.Append(other);
        }

        static int32_t find_substring(const Unreal::FString& str, std::string_view search)
        {
            auto converted = ensure_str(std::string(search));
            return str.Find(converted.c_str());
        }

        static bool starts_with(const Unreal::FString& str, std::string_view prefix)
        {
            auto converted = ensure_str(std::string(prefix));
            return str.StartsWith(converted.c_str());
        }

        static bool ends_with(const Unreal::FString& str, std::string_view suffix)
        {
            auto converted = ensure_str(std::string(suffix));
            return str.EndsWith(converted.c_str());
        }

        static Unreal::FString to_upper(const Unreal::FString& str)
        {
            return str.ToUpper();
        }

        static Unreal::FString to_lower(const Unreal::FString& str)
        {
            return str.ToLower();
        }

        // FUtf8String specializations
        static void push_string_to_lua(const LuaMadeSimple::Lua& lua, const Unreal::FUtf8String& str)
        {
            const Unreal::UTF8CHAR* string_data = *str;
            if (string_data)
            {
                lua.set_string(reinterpret_cast<const char*>(string_data), str.Len());
            }
            else
            {
                lua.set_string("", 0);
            }
        }

        static std::string get_string_from_object(const Unreal::FUtf8String& str)
        {
            return std::string(reinterpret_cast<const char*>(*str), str.Len());
        }

        static void append_string(Unreal::FUtf8String& str, std::string_view append)
        {
            std::string temp(append);
            str.Append(reinterpret_cast<const Unreal::UTF8CHAR*>(temp.c_str()));
        }

        static void append_from_object(Unreal::FUtf8String& str, const Unreal::FUtf8String& other)
        {
            str.Append(other);
        }

        static int32_t find_substring(const Unreal::FUtf8String& str, std::string_view search)
        {
            std::string temp(search);
            return str.Find(reinterpret_cast<const Unreal::UTF8CHAR*>(temp.c_str()));
        }

        static bool starts_with(const Unreal::FUtf8String& str, std::string_view prefix)
        {
            std::string temp(prefix);
            return str.StartsWith(reinterpret_cast<const Unreal::UTF8CHAR*>(temp.c_str()));
        }

        static bool ends_with(const Unreal::FUtf8String& str, std::string_view suffix)
        {
            std::string temp(suffix);
            return str.EndsWith(reinterpret_cast<const Unreal::UTF8CHAR*>(temp.c_str()));
        }

        static Unreal::FUtf8String to_upper(const Unreal::FUtf8String& str)
        {
            return str.ToUpper();
        }

        static Unreal::FUtf8String to_lower(const Unreal::FUtf8String& str)
        {
            return str.ToLower();
        }

        // FAnsiString specializations
        static void push_string_to_lua(const LuaMadeSimple::Lua& lua, const Unreal::FAnsiString& str)
        {
            const Unreal::ANSICHAR* string_data = *str;
            if (string_data)
            {
                lua.set_string(string_data, str.Len());
            }
            else
            {
                lua.set_string("", 0);
            }
        }

        static std::string get_string_from_object(const Unreal::FAnsiString& str)
        {
            return std::string(*str, str.Len());
        }

        static void append_string(Unreal::FAnsiString& str, std::string_view append)
        {
            std::string temp(append);
            str.Append(temp.c_str());
        }

        static void append_from_object(Unreal::FAnsiString& str, const Unreal::FAnsiString& other)
        {
            str.Append(other);
        }

        static int32_t find_substring(const Unreal::FAnsiString& str, std::string_view search)
        {
            std::string temp(search);
            return str.Find(temp.c_str());
        }

        static bool starts_with(const Unreal::FAnsiString& str, std::string_view prefix)
        {
            std::string temp(prefix);
            return str.StartsWith(temp.c_str());
        }

        static bool ends_with(const Unreal::FAnsiString& str, std::string_view suffix)
        {
            std::string temp(suffix);
            return str.EndsWith(temp.c_str());
        }

        static Unreal::FAnsiString to_upper(const Unreal::FAnsiString& str)
        {
            return str.ToUpper();
        }

        static Unreal::FAnsiString to_lower(const Unreal::FAnsiString& str)
        {
            return str.ToLower();
        }
    };

    // FString
    struct FStringName
    {
        constexpr static const char* ToString() { return "FString"; }
    };
    
    class FString : public TLuaStringBase<Unreal::FString, FStringName>
    {
        friend class TLuaStringBase<Unreal::FString, FStringName>;
        
        explicit FString(Unreal::FString* object) : TLuaStringBase(object) {}
        
    public:
        FString() = delete;
        using TLuaStringBase::construct;
    };

    // FUtf8String
    struct FUtf8StringName
    {
        constexpr static const char* ToString() { return "FUtf8String"; }
    };
    
    class FUtf8String : public TLuaStringBase<Unreal::FUtf8String, FUtf8StringName>
    {
        friend class TLuaStringBase<Unreal::FUtf8String, FUtf8StringName>;
        
        explicit FUtf8String(Unreal::FUtf8String* object) : TLuaStringBase(object) {}
        
    public:
        FUtf8String() = delete;
        using TLuaStringBase::construct;
    };

    // FAnsiString
    struct FAnsiStringName
    {
        constexpr static const char* ToString() { return "FAnsiString"; }
    };
    
    class FAnsiString : public TLuaStringBase<Unreal::FAnsiString, FAnsiStringName>
    {
        friend class TLuaStringBase<Unreal::FAnsiString, FAnsiStringName>;
        
        explicit FAnsiString(Unreal::FAnsiString* object) : TLuaStringBase(object) {}
        
    public:
        FAnsiString() = delete;
        using TLuaStringBase::construct;
    };
}