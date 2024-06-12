#pragma once

#include <unordered_map>

#include <Unreal/Core/HAL/Platform.hpp>
#include <Unreal/Core/Containers/ScriptArray.hpp>
#include <Unreal/Property/FArrayProperty.hpp>
#include <Unreal/Property/FObjectProperty.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UScriptStruct.hpp>
#include <lua.hpp>

namespace RC
{
    extern std::unordered_map<Unreal::FName, Unreal::UScriptStruct*> g_script_struct_cache_map;

    auto setup_script_struct_cache_map() -> void;
    auto setup_global_metatable(lua_State*) -> void;

    auto UObjectBase_memberr_function_wrapper_MyTestFunc(lua_State*) -> int;
    auto UObjectBase_member_function_wrapper_GetNamePrivate(lua_State*) -> int;
    auto UObjectBase_member_function_wrapper_Cast(lua_State*) -> int;
    auto FField_member_function_wrapper_CastField(lua_State*) -> int;
    auto UEnum_member_function_wrapper_ForEachName(lua_State*) -> int;
    auto UClass_member_function_wrapper_StaticClass(lua_State*) -> int;

    auto UObjectBase_metamethod_wrapper_Index(lua_State*, void*) -> int;
    auto UObjectBase_metamethod_wrapper_NewIndex(lua_State*, void*) -> int;
    auto UFunction_metamethod_wrapper_Call(lua_State*, void*) -> int;
    auto UObjectBaseUtility_metamethod_wrapper_Index(lua_State*, void*) -> int;
    auto UObjectBaseUtility_metamethod_wrapper_NewIndex(lua_State*, void*) -> int;
    auto ArrayTest_metamethod_wrapper_GC(lua_State*, void*) -> int;
    auto LuaUScriptStruct_metamethod_wrapper_Index(lua_State*, void*) -> int;
    auto LuaUScriptStruct_metamethod_wrapper_NewIndex(lua_State*, void*) -> int;
    auto LuaUScriptStruct_metamethod_wrapper_GC(lua_State*, void*) -> int;

    auto ArrayTest_member_function_wrapper_GetElementAtIndex(lua_State*) -> int;
    auto ArrayTest_member_function_wrapper_ForEach(lua_State*) -> int;

    auto lua_warn_wrapper(lua_State*) -> int;
    auto lua_print_wrapper(lua_State*) -> int;
    auto lua_FindAllOf_wrapper(lua_State*) -> int;
    auto lua_RegisterKeyBind_wrapper(lua_State*) -> int;
} // namespace RC

namespace RC::LuaBackCompat
{
    using namespace RC::Unreal;

    // Backwards compatibility with UE4SS 1.3.

    auto StaticFindObject(UClass* ObjectClass, UObject* InObjectPackage, const wchar_t* OrigInName, bool bExactClass = false) -> UObject*;
    auto StaticFindObject(const wchar_t* OrigInName) -> UObject*;
    auto NotifyOnNewObject(const wchar_t* class_name, std::function<void(UObject*)>& callable) -> void;

    auto lua_RegisterHook_wrapper(lua_State*) -> int;
    auto lua_UObjectBase_IsA_wrapper(lua_State*) -> int;
} // namespace RC::LuaBackCompat

namespace RC::UnrealRuntimeTypes
{
    using namespace Unreal;

    struct ToLuaParams
    {
        void* base{};
        void** out_data{};
        FProperty* property{};
        UFunction* function{};
        uint32_t pointer_depth{};
        bool base_no_processing_required{};
        bool should_copy_script_struct_contents{};
        bool should_containerize{}; // Only for types that are not already containerized by default.
        int32_t lua_stack_index{-1};

        // Add variable 'containerize_trivial_value' later if there's a need to create, for example, already existing integers in memory that can be accessed & altered.
        // If the variable is true then the integer handler should pass the pointer to the integer to 'lua_int32_t_to_lua_from_heap' instead of passing the integer value to lua_pushinteger.
    };
    using FromLuaParams = ToLuaParams;

    extern std::unordered_map<FName, void (*)(lua_State*, ToLuaParams)> g_unreal_property_to_lua;
    extern std::unordered_map<FName, void (*)(lua_State*, FromLuaParams)> g_unreal_property_from_lua;

    auto populate_unreal_property_to_lua_map() -> void;

    auto ObjectProperty_to_lua(lua_State* lua_state, ToLuaParams) -> void;
    auto ClassProperty_to_lua(lua_State* lua_state, ToLuaParams) -> void;
    auto Int8Property_to_lua(lua_State* lua_state, ToLuaParams) -> void;
    auto Int16Property_to_lua(lua_State* lua_state, ToLuaParams) -> void;
    auto IntProperty_to_lua(lua_State* lua_state, ToLuaParams) -> void;
    auto Int64Property_to_lua(lua_State* lua_state, ToLuaParams) -> void;
    auto ByteProperty_to_lua(lua_State* lua_state, ToLuaParams) -> void;
    auto UInt16Property_to_lua(lua_State* lua_state, ToLuaParams) -> void;
    auto UInt32Property_to_lua(lua_State* lua_state, ToLuaParams) -> void;
    auto UInt64Property_to_lua(lua_State* lua_state, ToLuaParams) -> void;
    auto StructProperty_to_lua(lua_State* lua_state, ToLuaParams) -> void;
    auto ArrayProperty_to_lua(lua_State* lua_state, ToLuaParams) -> void;
    auto FloatProperty_to_lua(lua_State* lua_state, ToLuaParams) -> void;
    auto DoubleProperty_to_lua(lua_State* lua_state, ToLuaParams) -> void;
    auto BoolProperty_to_lua(lua_State* lua_state, ToLuaParams) -> void;
    auto EnumProperty_to_lua(lua_State* lua_state, ToLuaParams) -> void;
    auto WeakObjectProperty_to_lua(lua_State* lua_state, ToLuaParams) -> void;
    auto NameProperty_to_lua(lua_State* lua_state, ToLuaParams) -> void;
    auto TextProperty_to_lua(lua_State* lua_state, ToLuaParams) -> void;
    auto StrProperty_to_lua(lua_State* lua_state, ToLuaParams) -> void;
    auto function_to_lua(lua_State* lua_state, ToLuaParams) -> void;

    auto ObjectProperty_from_lua(lua_State* lua_state, FromLuaParams) -> void;
    auto ClassProperty_from_lua(lua_State* lua_state, FromLuaParams) -> void;
    auto Int8Property_from_lua(lua_State* lua_state, FromLuaParams) -> void;
    auto Int16Property_from_lua(lua_State* lua_state, FromLuaParams) -> void;
    auto IntProperty_from_lua(lua_State* lua_state, FromLuaParams) -> void;
    auto Int64Property_from_lua(lua_State* lua_state, FromLuaParams) -> void;
    auto ByteProperty_from_lua(lua_State* lua_state, FromLuaParams) -> void;
    auto UInt16Property_from_lua(lua_State* lua_state, FromLuaParams) -> void;
    auto UInt32Property_from_lua(lua_State* lua_state, FromLuaParams) -> void;
    auto UInt64Property_from_lua(lua_State* lua_state, FromLuaParams) -> void;
    auto StructProperty_from_lua(lua_State* lua_state, FromLuaParams) -> void;
    auto ArrayProperty_from_lua(lua_State* lua_state, FromLuaParams) -> void;
    auto FloatProperty_from_lua(lua_State* lua_state, FromLuaParams) -> void;
    auto DoubleProperty_from_lua(lua_State* lua_state, FromLuaParams) -> void;
    auto BoolProperty_from_lua(lua_State* lua_state, FromLuaParams) -> void;
    auto EnumProperty_from_lua(lua_State* lua_state, FromLuaParams) -> void;
    auto WeakObjectProperty_from_lua(lua_State* lua_state, FromLuaParams) -> void;
    auto NameProperty_from_lua(lua_State* lua_state, FromLuaParams) -> void;
    auto TextProperty_from_lua(lua_State* lua_state, FromLuaParams) -> void;
    auto StrProperty_from_lua(lua_State* lua_state, FromLuaParams) -> void;

    auto Array_Type_Handler_Ptr() -> void;
    auto Array_Type_Handler_WChar_T() -> void;

    class Array
    {
      private:
        FScriptArray* ScriptArray{};
        FArrayProperty* Type{};
        int32_t TypeSize{};
        size_t TypeAlignment{};
        using TypeHandlerFunction = void (*)();
        using TypeHandlerMap = std::unordered_map<std::string, TypeHandlerFunction>;
        TypeHandlerMap TypeHandlers{
                {"Pointer", &Array_Type_Handler_Ptr},
                {"wchar_t", &Array_Type_Handler_WChar_T},
        };

      public:
        Array(size_t TypeSize, std::vector<std::string>& TypeNames);

      public:
        FScriptArray* GetScriptArray()
        {
            return ScriptArray;
        }
        FArrayProperty* GetType()
        {
            return Type;
        }

        template <typename CallableType>
        auto WithScriptArray(CallableType&& Callable) const
        {
            return Callable(ScriptArray);
        }

        int32 AddUninitializedValues(int32 Count)
        {
            const int32 OldNum = WithScriptArray([this, Count](auto* Array) {
                return Array->Add(Count, TypeSize, TypeAlignment);
            });
            return OldNum;
        }
    };

    template <typename T>
    concept HasStaticClassMemberFunction = requires(T t) {
        {
            T::StaticClass()
        };
    };

    struct ArrayTest
    {
      private:
        FScriptArray* ScriptArray{};
        FScriptArray InlineScriptArray{};
        bool IsScriptArrayInline{};

      public:
        size_t ElementSize{};
        size_t ElementMinAlignment{};
        std::string TypeName{};
        FArrayProperty* Property{};
        bool TypeIsAlwaysPointer{};

      private:
        uint8* GetRawPtr(int32 Index = 0)
        {
            if (!Num())
            {
                return NULL;
            }
            return static_cast<uint8*>(GetElementAtIndex(Index));
        }

        void DestructItems(int32 Index, int32 Count)
        {
            if (!(Property->GetPropertyFlags() & (CPF_IsPlainOldData | CPF_NoDestructor)))
            {
                if (Count > 0)
                {
                    uint8* Dest = GetRawPtr(Index);
                    for (int32 LoopIndex = 0; LoopIndex < Count; LoopIndex++, Dest += ElementSize)
                    {
                        Property->DestroyValue(Dest);
                    }
                }
            }
        }

      public:
        // Script constructor! Do not use directly!
        ArrayTest() : IsScriptArrayInline(true)
        {
            // Property to be later by the scripting system.
        }

        ArrayTest(FScriptArray* ScriptArray,
                  size_t ElementSize,
                  size_t ElementMinAlignment,
                  const std::string& TypeName,
                  bool TypeIsAlwaysPointer,
                  FArrayProperty* Property = nullptr)
            : ScriptArray(ScriptArray), IsScriptArrayInline(false), ElementSize(ElementSize), ElementMinAlignment(ElementMinAlignment), TypeName(TypeName),
              TypeIsAlwaysPointer(TypeIsAlwaysPointer), Property(Property)
        {
        }

        ArrayTest(FScriptArray&& ScriptArray,
                  size_t ElementSize,
                  size_t ElementMinAlignment,
                  const std::string& TypeName,
                  bool TypeIsAlwaysPointer,
                  FArrayProperty* Property = nullptr)
            : ScriptArray(nullptr), InlineScriptArray(std::move(ScriptArray), ElementSize, ElementMinAlignment), IsScriptArrayInline(true),
              ElementSize(ElementSize), ElementMinAlignment(ElementMinAlignment), TypeName(TypeName), TypeIsAlwaysPointer(TypeIsAlwaysPointer), Property(Property)
        {
        }

        ArrayTest(ArrayTest&& From)
        {
            if (!From.IsScriptArrayInline)
            {
                ScriptArray = From.ScriptArray;
                From.ScriptArray = nullptr;
            }
            else
            {
                InlineScriptArray.MoveAssign(From.InlineScriptArray, From.ElementSize, From.ElementMinAlignment);
            }
            IsScriptArrayInline = std::move(From.IsScriptArrayInline);
            ElementSize = std::move(From.ElementSize);
            ElementMinAlignment = std::move(From.ElementMinAlignment);
            TypeName = std::move(From.TypeName);
            Property = std::move(From.Property);
            TypeIsAlwaysPointer = std::move(From.TypeIsAlwaysPointer);
        }

        ~ArrayTest()
        {
            if (!GetScriptArray())
            {
                return;
            }
            if (Property)
            {
                if (IsScriptArrayInline)
                {
                    DestructItems(0, Num());
                }
            }
            else
            {
                GetScriptArray()->Empty(0, ElementSize, ElementMinAlignment);
                ScriptArray = nullptr;
            }
        }

        FScriptArray* GetScriptArray()
        {
            if (IsScriptArrayInline)
            {
                return &InlineScriptArray;
            }
            else
            {
                return ScriptArray;
            }
        }

        auto Num() -> int32
        {
            return GetScriptArray() ? GetScriptArray()->Num() : 0;
        }
        auto Max() -> int32
        {
            return GetScriptArray() ? GetScriptArray()->Max() : 0;
        }

        using FuncParamType = bool (*)(int);
        typedef bool (*FuncParamTypedef)(int);
        auto FuncTestOne(bool (*func_param)(int)) -> void
        {
            func_param(1);
        }
        auto FuncTestTwo(FuncParamType func_param) -> void
        {
            func_param(2);
        }
        auto FuncTestThree(FuncParamTypedef func_param) -> void
        {
            func_param(3);
        }
        auto FuncTestFour(std::function<bool(int)> func_param) -> void
        {
            func_param(4);
        }

        auto ForEach(LoopAction (*Callable)(int Index, void* Element)) -> void
        {
            if (!GetScriptArray() || !GetScriptArray()->GetData() || GetScriptArray()->IsEmpty())
            {
                return;
            }

            printf_s("ScriptArray: %p\n", GetScriptArray());
            printf_s("Data: %p\n", GetScriptArray()->GetData());
            printf_s("*Data: %p\n", *(void**)GetScriptArray()->GetData());
            for (int i = 0; i < GetScriptArray()->Num(); ++i)
            {
                const auto Element = std::bit_cast<void*>(std::bit_cast<char*>(GetScriptArray()->GetData()) + ElementMinAlignment * i);
                // For type-information, we might need to store the UClass pointer instead of just the size & alignment.
                // That might collide with properties whenever support for that is added.
                // Maybe we can store both, one being nullptr when not applicable.
                // printf_s("[%i] Element: %p\n", i, Element);
                if (Callable(i, Element) == LoopAction::Break)
                {
                    break;
                }
            }
        }

        auto GetElementAtIndex(int32 index) -> void*
        {
            if (!GetScriptArray() || !GetScriptArray()->GetData() || GetScriptArray()->IsEmpty())
            {
                return nullptr;
            }
            printf_s("ScriptArray: %p\n", GetScriptArray());
            printf_s("Data: %p\n", GetScriptArray()->GetData());
            printf_s("*Data: %p\n", *(void**)GetScriptArray()->GetData());

            if (!GetScriptArray()->IsValidIndex(index))
            {
                return nullptr;
            }

            auto Element = std::bit_cast<void*>(std::bit_cast<char*>(GetScriptArray()->GetData()) + ElementMinAlignment * index);
            printf_s("[%i] Element: %p\n", index, Element);
            return Element;
        }
    };

    struct LuaUScriptStruct
    {
        UObject* base{};
        FStructProperty* struct_property{};
        UScriptStruct* script_struct{};
        bool is_user_constructed{};

        LuaUScriptStruct(UObject* base, FStructProperty* struct_property, UScriptStruct* script_struct, bool is_user_constructed = false)
            : base(base), struct_property(struct_property), script_struct(script_struct), is_user_constructed(is_user_constructed)
        {
        }

        LuaUScriptStruct(const LuaUScriptStruct& from)
        {
            if (from.is_user_constructed && from.script_struct)
            {
                auto struct_ops = from.script_struct->GetCppStructOps();
                base = static_cast<UObject*>(FMemory::Malloc(struct_ops->GetSize(), struct_ops->GetAlignment()));
                from.script_struct->CopyScriptStruct(base, from.base);
            }
            else
            {
                base = from.base;
            }
            struct_property = from.struct_property;
            script_struct = from.script_struct;
            is_user_constructed = from.is_user_constructed;
        }

        LuaUScriptStruct(LuaUScriptStruct&& from) noexcept
        {
            base = from.base;
            struct_property = from.struct_property;
            script_struct = from.script_struct;
            is_user_constructed = from.is_user_constructed;
            from.base = nullptr;
            from.struct_property = nullptr;
            from.script_struct = nullptr;
            from.is_user_constructed = false;
        }

        ~LuaUScriptStruct()
        {
            if (is_user_constructed && script_struct && base)
            {
                // This might need to be moved elsewhere.
                // Could you have a situation where you have a user-constructed struct that's moved or is referenced outside the Lua script ?
                // If yes, then we can't free here because it will cause use-after-free.
                // If no, then we can and should free here.
                // Since this is a UScriptStruct, which as far as I know are always inlined into other structs or objects when not constructed by the user,
                // it should be safe to free here as there would be no place to reference it. It's mostly (only ?) used for temporary logic in a function or as a UFunction param.
                script_struct->DestroyStruct(base);
                FMemory::Free(base);
                base = nullptr;
            }
        }
    };
} // namespace RC::UnrealRuntimeTypes
