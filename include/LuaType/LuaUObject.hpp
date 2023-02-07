#ifndef UE4SS_REWRITTEN_LUAUOBJECT_HPP
#define UE4SS_REWRITTEN_LUAUOBJECT_HPP

#define NOMINMAX

#include <format>
#include <functional>

#include <LuaMadeSimple/LuaObject.hpp>
#include <Helpers/String.hpp>
#include <DynamicOutput/DynamicOutput.hpp>
#include <LuaType/LuaCustomProperty.hpp>
#pragma warning(disable: 4005)
#include <Unreal/UObject.hpp>
#include <Unreal/UClass.hpp>
#include <Unreal/UStruct.hpp>
#include <Unreal/World.hpp>
#include <Unreal/FProperty.hpp>
#include <Unreal/NameTypes.hpp>
#include <Unreal/VersionedContainer/Container.hpp>
#pragma warning(default: 4005)

template<typename SupposedIntegralType>
concept IsConvertableToLuaInteger = std::is_integral_v<SupposedIntegralType>;

namespace RC::LuaType
{
    auto call_ufunction_from_lua(const LuaMadeSimple::Lua& lua) -> int;

    using Operation = LuaMadeSimple::Type::Operation;

    template<typename ObjectType, template<typename T> typename LuaObjectBase, typename ObjectName>
    class ObjectBase : public LuaObjectBase<ObjectType>
    {
    protected:
        using SelfType = ObjectBase<ObjectType, LuaObjectBase, ObjectName>;
        using Super = SelfType;
        using ClassName = ObjectName;

    public:
        ObjectBase(ObjectType* object) : LuaObjectBase<ObjectType>(ObjectName::ToString(), object) {}
        ObjectBase(ObjectType&& object) : LuaObjectBase<ObjectType>(ObjectName::ToString(), std::move(object)) {}
        ObjectBase() = delete;
        virtual ~ObjectBase() = default;

    public:
        // Whether this object is a specific type or inherits from a specific type
        virtual auto derives_from_actor()           -> bool { return false; }
        virtual auto derives_from_name()            -> bool { return false; }
        virtual auto derives_from_string()          -> bool { return false; }
        virtual auto derives_from_text()            -> bool { return false; }
        virtual auto derives_from_weak_object_ptr() -> bool { return false; }
        virtual auto derives_from_mod()             -> bool { return false; }
        virtual auto derives_from_array()           -> bool { return false; }
        virtual auto derives_from_class()           -> bool { return false; }
        virtual auto derives_from_enum()            -> bool { return false; }
        virtual auto derives_from_function()        -> bool { return false; }
        virtual auto derives_from_object()          -> bool { return false; }
        virtual auto derives_from_script_struct()   -> bool { return false; }
        virtual auto derives_from_world()           -> bool { return false; }
        virtual auto derives_from_property()        -> bool { return false; }
        virtual auto derives_from_property_class()  -> bool { return false; }
        virtual auto derives_from_ufunction()       -> bool { return false; }

    public:
        // Constructor for RemoteObjectBase
        auto static construct(const LuaMadeSimple::Lua& lua, Unreal::UObject* unreal_object) -> const LuaMadeSimple::Lua::Table
        {
            SelfType lua_object{unreal_object};

            auto metatable_name = lua_object.get_object_name();

            LuaMadeSimple::Lua::Table table = lua.get_metatable(metatable_name);
            if (lua.is_nil(-1))
            {
                lua.discard_value(-1);
                LuaMadeSimple::Type::RemoteObject<ObjectType>::construct(lua, lua_object);
                setup_member_functions<LuaMadeSimple::Type::IsFinal::Yes>(table);
                lua.new_metatable<SelfType>(metatable_name, lua_object.get_metamethods());
            }

            // Create object & surrender ownership to Lua
            lua.transfer_stack_object(std::move(lua_object), metatable_name, lua_object.get_metamethods());

            return table;
        }

        // Constructor for objects that inherit from RemoteObjectBase
        auto static construct(const LuaMadeSimple::Lua& lua, LuaMadeSimple::Type::BaseObject& construct_to) -> const LuaMadeSimple::Lua::Table
        {
            const LuaMadeSimple::Lua::Table table = LuaMadeSimple::Type::RemoteObject<ObjectType>::construct(lua, construct_to);

            // Setup functions that can be called on this object
            setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(table);

            // Creation & transferring the object & its ownership fully to Lua is the responsibility of the overriding object

            return table;
        }

        template<LuaMadeSimple::Type::IsFinal is_final>
        auto static setup_member_functions(const LuaMadeSimple::Lua::Table& table) -> void
        {
            // Overridden functions from 'Lua::RemoteObject'.
            table.add_pair("GetAddress", [](const LuaMadeSimple::Lua& lua) -> int {
                const auto& lua_object = lua.get_userdata<SelfType>();
                lua.set_integer(reinterpret_cast<uintptr_t>(lua_object.get_remote_cpp_object()));
                return 1;
            });

            table.add_pair("IsValid", [](const LuaMadeSimple::Lua& lua) -> int {
                const auto& lua_object = lua.get_userdata<SelfType>();
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
                table.add_pair("type", [](const LuaMadeSimple::Lua& lua) -> int {
                    lua.set_string("RemoteObjectBase");
                    return 1;
                });

                // If this is the final object then we also want to finalize creating the table
                // If not then it's the responsibility of the overriding object to call 'make_global()
                //table.make_global("RemoteObjectBase");
            }
        }
    };

    struct UE4SSBaseObjectName { constexpr static const char* ToString() { return "UE4SSBaseObject"; } };

    template<typename DerivedType, typename ObjectName>
    using RemoteObjectBase = ObjectBase<DerivedType, LuaMadeSimple::Type::RemoteObject, ObjectName>;

    template<typename DerivedType, typename ObjectName>
    using LocalObjectBase = ObjectBase<DerivedType, LuaMadeSimple::Type::LocalObject, ObjectName>;

    using UE4SSBaseObject = ObjectBase<uint8_t, LuaMadeSimple::Type::RemoteObject, UE4SSBaseObjectName>;

    struct PusherParams
    {
        const Operation operation;
        const LuaMadeSimple::Lua& lua;
        Unreal::UObject* base;
        void* data;
        Unreal::FProperty* property;

        // Where in the Lua stack the data is at
        // Default is 1 because that's the default for LuaMadeSimple because that's how the system works
        int32_t stored_at_index = 1;

        // Whether to create a new Lua item (example: table) or use an existing one on the top of the stack
        bool create_new_if_get_non_trivial_local{true};
    };

    struct FunctionPusherParams
    {
        const LuaMadeSimple::Lua& lua;
        Unreal::UObject* base;
        Unreal::UFunction* function;
    };

    struct StaticState
    {
        using PropertyValuePusherCallable = std::function<void(const PusherParams&)>;
        static inline std::unordered_map<int32_t, PropertyValuePusherCallable> m_property_value_pushers;
    };

    auto auto_construct_object(const LuaMadeSimple::Lua&, Unreal::UObject*) -> void;

    // Helpers to construct an object that we cannot have access to in this header
    auto construct_fname(const LuaMadeSimple::Lua&) -> void;
    auto construct_uclass(const LuaMadeSimple::Lua&) -> void;
    auto construct_xproperty(const LuaMadeSimple::Lua&, Unreal::FProperty* property) -> void;

    // Push to Lua -> START
    auto push_unhandledproperty(const PusherParams&) -> void;
    auto push_objectproperty(const PusherParams&) -> void;
    auto push_classproperty(const PusherParams&) -> void;
    auto push_int8property(const PusherParams&) -> void;
    auto push_int16property(const PusherParams&) -> void;
    auto push_intproperty(const PusherParams&) -> void;
    auto push_int64property(const PusherParams&) -> void;
    auto push_byteproperty(const PusherParams&) -> void; // Unreal Engine calls uint8 a "byte"
    auto push_uint16property(const PusherParams&) -> void;
    auto push_uint32property(const PusherParams&) -> void;
    auto push_uint64property(const PusherParams&) -> void;
    auto push_structproperty(const PusherParams&) -> void;
    auto push_arrayproperty(const PusherParams&) -> void;
    auto push_floatproperty(const PusherParams&) -> void;
    auto push_doubleproperty(const PusherParams&) -> void;
    auto push_boolproperty(const PusherParams&) -> void;
    auto push_enumproperty(const PusherParams&) -> void;
    auto push_weakobjectproperty(const PusherParams&) -> void;
    auto push_nameproperty(const PusherParams&) -> void;
    auto push_textproperty(const PusherParams&) -> void;
    auto push_strproperty(const PusherParams&) -> void;
    auto push_softclassproperty(const PusherParams&) -> void;
    auto push_interfaceproperty(const PusherParams&) -> void;

    auto push_functionproperty(const FunctionPusherParams&) -> void;
    // Push to Lua -> END

    auto handle_unreal_property_value(const Operation operation, const LuaMadeSimple::Lua&, Unreal::UObject* base, Unreal::FName property_name, Unreal::FField* field) -> void;

    auto is_a_implementation(const LuaMadeSimple::Lua& lua) -> int;

    template<typename DerivedType, typename ObjectName>
    class UObjectBase;

    struct UObjectName { constexpr static const char* ToString() { return "UObject"; } };
    using UObject = UObjectBase<Unreal::UObject, UObjectName>;
    
    template<typename DerivedType, typename ObjectName>
    class UObjectBase : public RemoteObjectBase<DerivedType, ObjectName>
    {
    protected:
        using Super = RemoteObjectBase<DerivedType, ObjectName>;
        using SelfType = Super;

    public:
        explicit UObjectBase(DerivedType* object) : Super(object) {}

    public:
        auto derives_from_object() -> bool override { return true; }

    public:
        UObjectBase() = delete;
        virtual ~UObjectBase() = default;

        // Constructor for UObject
        auto static construct(const LuaMadeSimple::Lua& lua, DerivedType* unreal_object) -> const LuaMadeSimple::Lua::Table
        {
            SelfType lua_object{unreal_object};

            auto metatable_name = ObjectName::ToString();

            LuaMadeSimple::Lua::Table table = lua.get_metatable(metatable_name);
            if (lua.is_nil(-1))
            {
                lua.discard_value(-1);
                Super::construct(lua, lua_object);
                setup_metamethods(lua_object);
                setup_member_functions<LuaMadeSimple::Type::IsFinal::Yes>(table);
                lua.new_metatable<SelfType>(metatable_name, lua_object.get_metamethods());
            }

            // Create object & surrender ownership to Lua
            lua.transfer_stack_object(std::move(lua_object), metatable_name, lua_object.get_metamethods());

            return table;
        }

        // Constructor for objects that inherit from UObject
        auto static construct(const LuaMadeSimple::Lua& lua, LuaMadeSimple::Type::BaseObject& construct_to) -> const LuaMadeSimple::Lua::Table
        {
            LuaMadeSimple::Lua::Table table = Super::construct(lua, construct_to);

            // Setup functions that can be called on this object
            setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(table);

            setup_metamethods(construct_to);

            // Creation & transferring the object & its ownership fully to Lua is the responsibility of the overriding object

            return table;
        }

    private:
        auto static setup_metamethods(LuaMadeSimple::Type::BaseObject& base_object) -> void
        {
            // Another object can override these metamethods by calling the 'create()' function again
            base_object.get_metamethods().create(LuaMadeSimple::Lua::MetaMethod::Index, [](const LuaMadeSimple::Lua& lua) -> int {
                prepare_to_handle(Operation::Get, lua);
                return 1;
            });

            base_object.get_metamethods().create(LuaMadeSimple::Lua::MetaMethod::NewIndex, [](const LuaMadeSimple::Lua& lua) -> int {
                prepare_to_handle(Operation::Set, lua);
                return 0;
            });

            base_object.get_metamethods().create(LuaMadeSimple::Lua::MetaMethod::Call, [](const LuaMadeSimple::Lua& lua) -> int {
                const auto& lua_object = lua.get_userdata<SelfType>();

                if (!lua_object.get_remote_cpp_object())
                {
                    lua.throw_error("Tried calling a member function but the UObject instance is nullptr\n");
                }
                return 0;
            });
        }

    public:
        template<LuaMadeSimple::Type::IsFinal is_final>
        auto static setup_member_functions(const LuaMadeSimple::Lua::Table& table) -> void
        {
            Super::setup_member_functions<LuaMadeSimple::Type::IsFinal::No>(table);

            // Add functions that are not intended to be overridden later here
            table.add_pair("GetFullName", [](const LuaMadeSimple::Lua& lua) -> int {
                // Get the userdata from the Lua stack
                // We're making an assumption here about the type
                // I know of no way to get around this assumption
                const auto& lua_object = lua.get_userdata<SelfType>();

                if (lua_object.get_remote_cpp_object())
                {
                    // Set the return value to the ansi version of the full name
                    lua.set_string(to_string(lua_object.get_remote_cpp_object()->GetFullName()).c_str());
                }
                else
                {
                    // We have a nullptr, lets return 'nil' for easy object verification in Lua
                    lua.set_nil();
                }

                return 1;
            });

            table.add_pair("GetFName", [](const LuaMadeSimple::Lua& lua) -> int {
                construct_fname(lua);
                return 1;
            });

            table.add_pair("GetClass", [](const LuaMadeSimple::Lua& lua) -> int {
                construct_uclass(lua);
                return 1;
            });

            table.add_pair("GetOuter", [](const LuaMadeSimple::Lua& lua) -> int {
                const auto& lua_object = lua.get_userdata<SelfType>();

                UObject::construct(lua, lua_object.get_remote_cpp_object()->GetOuterPrivate());

                return 1;
            });

            table.add_pair("IsAnyClass", [](const LuaMadeSimple::Lua& lua) -> int {
                const auto& lua_object = lua.get_userdata<SelfType>();

                lua.set_bool(lua_object.get_remote_cpp_object()->IsA<Unreal::UClass>());

                return 1;
            });

            table.add_pair("Reflection", [](const LuaMadeSimple::Lua& lua) -> int {
                const auto& lua_object = lua.get_userdata<SelfType>();

                auto reflection_table = lua.prepare_new_table();
                reflection_table.set_has_userdata(false);
                reflection_table.add_key("ReflectedObject");
                SelfType::construct(lua, lua_object.get_remote_cpp_object());
                reflection_table.fuse_pair();

                reflection_table.add_pair("GetProperty", [](const LuaMadeSimple::Lua& lua) -> int {
                    // Stack
                    // 1 (-2): ReflectionTable
                    // 2 (-1): Param #1 (PropertyName)

                    if (!lua.is_string(2))
                    {
                        lua.throw_error("Function 'GetProperty' requires a string as the first parameter");
                    }
                    std::wstring property_name = to_wstring(lua.get_string(2));

                    auto reflection_table = lua.get_table();
                    const auto& reflected_object = reflection_table.get_userdata_field<SelfType>("ReflectedObject").get_remote_cpp_object();

                    if (!reflected_object)
                    {
                        construct_xproperty(lua, nullptr);
                        return 1;
                    }

                    auto* obj_as_struct = Unreal::Cast<Unreal::UStruct>(reflected_object);
                    if (!obj_as_struct) { obj_as_struct = reflected_object->GetClassPrivate(); }
                    auto* property = obj_as_struct->FindProperty(Unreal::FName(property_name));

                    construct_xproperty(lua, property);
                    return 1;
                });
                return 1;
            });

            table.add_pair("GetPropertyValue", [](const LuaMadeSimple::Lua& lua) -> int {
                prepare_to_handle(Operation::Get, lua);
                return 1;
            });

            table.add_pair("SetPropertyValue", [](const LuaMadeSimple::Lua& lua) -> int {
                prepare_to_handle(Operation::Set, lua);
                return 1;
            });

            table.add_pair("IsClass", [](const LuaMadeSimple::Lua& lua) -> int {
                // TODO: Refactor this function into "IsA", but leave the original "IsClass" for compatibility reasons
                //       Creating an "IsA" function will require that we expose some identifier of every class to Lua
                //       We could use strings or FNames, those could be calculated on the Lua side and then
                //       we wouldn't need to preemptively expose anything
                const auto& lua_object = lua.get_userdata<SelfType>();
                lua.set_bool(lua_object.get_remote_cpp_object()->template IsA<Unreal::UClass>());
                return 1;
            });

            table.add_pair("GetWorld", [](const LuaMadeSimple::Lua& lua) -> int {
                const auto& lua_object = lua.get_userdata<SelfType>();
                auto_construct_object(lua, lua_object.get_remote_cpp_object()->GetWorld());
                return 1;
            });

            table.add_pair("CallFunction", [](const LuaMadeSimple::Lua& lua) -> int {
                return call_ufunction_from_lua(lua);
            });

            table.add_pair("IsA", [](const LuaMadeSimple::Lua& lua) -> int {
                return is_a_implementation(lua);
            });

            table.add_pair("HasAllFlags", [](const LuaMadeSimple::Lua& lua) -> int {
                std::string error_overload_not_found{R"(
No overload found for function 'UObject.HasAllFlags'.
Overloads:
#1: HasAllFlags(EObjectFlags ObjectFlags))"};

                const auto& lua_object = lua.get_userdata<SelfType>();

                if (!lua.is_integer())
                {
                    lua.throw_error(error_overload_not_found);
                }

                Unreal::EObjectFlags object_flags = static_cast<Unreal::EObjectFlags>(lua.get_integer());
                lua.set_bool(lua_object.get_remote_cpp_object()->HasAllFlags(object_flags));
                return 1;
            });

            table.add_pair("HasAnyFlags", [](const LuaMadeSimple::Lua& lua) -> int {
                std::string error_overload_not_found{R"(
No overload found for function 'UObject.HasAnyFlags'.
Overloads:
#1: HasAnyFlags(EObjectFlags ObjectFlags))"};

                const auto& lua_object = lua.get_userdata<SelfType>();

                if (!lua.is_integer())
                {
                    lua.throw_error(error_overload_not_found);
                }

                Unreal::EObjectFlags object_flags = static_cast<Unreal::EObjectFlags>(lua.get_integer());
                lua.set_bool(lua_object.get_remote_cpp_object()->HasAnyFlags(object_flags));
                return 1;
            });

            table.add_pair("HasAnyInternalFlags", [](const LuaMadeSimple::Lua& lua) -> int {
                std::string error_overload_not_found{R"(
No overload found for function 'UObject.HasAnyInternalFlags'.
Overloads:
#1: HasAnyInternalFlags(EInternalObjectFlags InternalObjectFlags))"};

                const auto& lua_object = lua.get_userdata<SelfType>();

                if (!lua.is_integer())
                {
                    lua.throw_error(error_overload_not_found);
                }

                Unreal::EInternalObjectFlags object_internal_flags = static_cast<Unreal::EInternalObjectFlags>(lua.get_integer());
                lua.set_bool(lua_object.get_remote_cpp_object()->HasAnyInternalFlags(object_internal_flags));
                return 1;
            });

            table.add_pair("IsValid", [](const LuaMadeSimple::Lua& lua) -> int {
                const auto& lua_object = lua.get_userdata<SelfType>();
                if (lua_object.get_remote_cpp_object() && !lua_object.get_remote_cpp_object()->IsUnreachable())
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
                table.add_pair("type", [](const LuaMadeSimple::Lua& lua) -> int {
                    lua.set_string("UObject");
                    return 1;
                });

                // If this is the final object then we also want to finalize creating the table
                // If not then it's the responsibility of the overriding object to call 'make_global()
                //table.make_global("UObject");
            }
        }

    private:
        auto static prepare_to_handle(const Operation operation, const LuaMadeSimple::Lua& lua) -> void
        {
            auto& lua_object = lua.get_userdata<SelfType>();

            std::wstring member_name = to_wstring(lua.get_string());

            // If nullptr then we assume the UObject wasn't found so lets return an invalid UObject to Lua
            // This allows the safe chaining of "__index" as long as the Lua script checks ":IsValid()" before using the object
            if (!lua_object.get_remote_cpp_object())
            {
                // If the operation is not "Get" then this isn't "__index" and we want to do nothing in this case
                switch (operation)
                {
                    case Operation::Get:
                    case Operation::GetParam:
                        // Construct an empty object to allow for safe chaining with a validity check at the end
                        SelfType::construct(lua, static_cast<DerivedType*>(nullptr));
                        break;
                    case Operation::Set:
                        Output::send(STR("[Lua][Error] Tried setting member variable '{}' but UObject instance is nullptr\n"), member_name);
                        break;
                    default:
                        Output::send(STR("[Lua][Error] The UObject instance is nullptr & operation type was invalid\n"));
                        break;
                }

                return;
            }

            Unreal::FName property_name = Unreal::FName(member_name);
            Unreal::FField* field = LuaCustomProperty::StaticStorage::property_list.find_or_nullptr(lua_object.get_remote_cpp_object(), member_name);

            if (!field)
            {
                auto* object = lua_object.get_remote_cpp_object();
                auto* obj_as_struct = Unreal::Cast<Unreal::UStruct>(object);
                if (!obj_as_struct) { obj_as_struct = object->GetClassPrivate(); }
                field = obj_as_struct->FindProperty(property_name);
            }

            handle_unreal_property_value(operation, lua, lua_object.get_remote_cpp_object(), property_name, field);
        }
    };

    template<typename ParamType>
    class LocalUnrealParam : public LuaMadeSimple::Type::LocalObject<ParamType>
    {
    private:
        Unreal::FProperty* m_property{};
        Unreal::UObject* m_base{};

    public:
        LocalUnrealParam(ParamType object, Unreal::UObject* base, Unreal::FProperty* property) :
                LuaMadeSimple::Type::LocalObject<ParamType>("LocalParam", std::move(object)),
                m_property(property),
                m_base(base)
        {
        }

        // Constructor for local param
        auto static construct(const LuaMadeSimple::Lua& lua, ParamType& object, Unreal::UObject* base, Unreal::FProperty* property) -> const LuaMadeSimple::Lua::Table
        {
            LuaType::LocalUnrealParam lua_object{object, base, property};

            auto metatable_name = "LocalUnrealParam";

            LuaMadeSimple::Lua::Table table = lua.get_metatable(metatable_name);
            if (lua.is_nil(-1))
            {
                lua.discard_value(-1);
                lua.prepare_new_table();
                setup_metamethods(lua_object);
                setup_member_functions(table);
                lua.new_metatable<LuaType::LocalUnrealParam<ParamType>>(metatable_name, lua_object.get_metamethods());
            }

            // Create object & surrender ownership to Lua
            lua.transfer_stack_object(std::move(lua_object), metatable_name, lua_object.get_metamethods());

            return table;
        }

    private:
        auto static setup_metamethods(LuaMadeSimple::Type::BaseObject& base_object) -> void
        {
            base_object.get_metamethods().create(LuaMadeSimple::Lua::MetaMethod::Index, [](const LuaMadeSimple::Lua& lua) -> int {
                prepare_to_handle(Operation::Get, lua);
                return 1;
            });

            base_object.get_metamethods().create(LuaMadeSimple::Lua::MetaMethod::NewIndex, [](const LuaMadeSimple::Lua& lua) -> int {
                prepare_to_handle(Operation::Set, lua);
                return 0;
            });
        }

        auto static setup_member_functions(LuaMadeSimple::Lua::Table& table) -> void
        {
            table.add_pair("get", [](const LuaMadeSimple::Lua& lua) -> int {
                prepare_to_handle(Operation::Get, lua);
                return 1;
            });

            table.add_pair("set", [](const LuaMadeSimple::Lua& lua) -> int {
                prepare_to_handle(Operation::Set, lua);
                return 0;
            });

            // This class is not overridable so this is always the final class and because of that we always want to set the type & make it global
            table.add_pair("type", [](const LuaMadeSimple::Lua& lua) -> int {
                lua.set_string("LocalUnrealParam");
                return 1;
            });

            // If this is the final object then we also want to finalize creating the table
            // If not then it's the responsibility of the overriding object to call 'make_global()'
            //table.make_global("LocalUnrealParam");
        }

        auto static prepare_to_handle(const Operation operation, const LuaMadeSimple::Lua& lua) -> void
        {
            auto& lua_object = lua.get_userdata<LuaType::LocalUnrealParam<ParamType>>();

            Unreal::FName property_type = lua_object.m_property->GetClass().GetFName();
            int32_t type_name_comparison_index = property_type.GetComparisonIndex();

            if (StaticState::m_property_value_pushers.contains(type_name_comparison_index))
            {
                const PusherParams pusher_params{
                        .operation = operation,
                        .lua = lua,
                        .base = lua_object.m_base,
                        .data = lua_object.get_local_cpp_object().get_data_ptr(),
                        .property = lua_object.m_property
                };
                StaticState::m_property_value_pushers[type_name_comparison_index](pusher_params);
            }
            else
            {
                // We can either throw an error and kill the execution
                /**/
                std::wstring property_type_name = property_type.ToString();
                lua.throw_error(std::format("Tried accessing unreal property without a registered handler. Property type '{}' not supported.", to_string(property_type_name)));
                //*/

                // Or we can treat unhandled property types as some sort of generic type
                // For example, we might want to pass the unhandled property value to some sort of low-level Lua API
                // Then the Lua script can deal with figuring out how to handle the type
                /*
                push_unhandledproperty(lua, base);
                //*/
            }
        }
    };

    class RemoteUnrealParam : public LuaMadeSimple::Type::RemoteObject<void>
    {
    private:
        Unreal::FProperty* m_property{};
        Unreal::UObject* m_base{};
        Unreal::FName m_type{};

    public:
        explicit RemoteUnrealParam(void* param_ptr, Unreal::UObject* base, Unreal::FProperty* param_property_type_name, const Unreal::FName type);
        explicit RemoteUnrealParam(void* data_ptr, const Unreal::FName type);

        // Constructor for param
        auto static construct(const LuaMadeSimple::Lua&, void* param_ptr, Unreal::UObject* base, Unreal::FProperty* property) -> const LuaMadeSimple::Lua::Table;
        auto static construct(const LuaMadeSimple::Lua&, void* data_ptr, const Unreal::FName type) -> const LuaMadeSimple::Lua::Table;

    private:
        auto static setup_metamethods(BaseObject&) -> void;
        auto static setup_member_functions(LuaMadeSimple::Lua::Table& table) -> void;
        auto static prepare_to_handle(const Operation, const LuaMadeSimple::Lua&) -> void;
    };

    template<IsConvertableToLuaInteger IntegerType>
    auto push_integer(const PusherParams& params) -> void
    {
        IntegerType* integer_ptr = static_cast<IntegerType*>(params.data);
        if (!integer_ptr) { params.lua.throw_error("[push_integer] data pointer is nullptr"); }

        switch (params.operation)
        {
            case Operation::GetNonTrivialLocal:
            case Operation::Get:
                params.lua.set_integer(*integer_ptr);
                return;
            case Operation::Set:
                *integer_ptr = static_cast<IntegerType>(params.lua.get_integer(params.stored_at_index));
                return;
            case Operation::GetParam:
                RemoteUnrealParam::construct(params.lua, integer_ptr, params.base, params.property);
                return;
            default:
                params.lua.throw_error("[push_integer] Unhandled Operation");
                break;
        }

        params.lua.throw_error(std::format("[push_integer] Unknown Operation ({}) not supported", static_cast<int32_t>(params.operation)));
    }
}


#endif //UE4SS_REWRITTEN_LUAUOBJECT_HPP
