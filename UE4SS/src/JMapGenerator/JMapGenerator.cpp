#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include <DynamicOutput/DynamicOutput.hpp>
#include <File/Macros.hpp>
#include <Helpers/String.hpp>
#include <JMapGenerator/JMapGenerator.hpp>
#include <UE4SSProgram.hpp>
#include <Unreal/CoreUObject/UObject/Class.hpp>
#include <Unreal/CoreUObject/UObject/UnrealType.hpp>
#include <Unreal/FMemory.hpp>
#include <Unreal/NameTypes.hpp>
#include <Unreal/Script.hpp>
#include <Unreal/ScriptContainerLayout.hpp>
#include <Unreal/Property/FEnumProperty.hpp>
#include <Unreal/Property/FOptionalProperty.hpp>
#include <Unreal/UKismetSystemLibrary.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UObjectArray.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/UnrealFlags.hpp>
#include <Unreal/UnrealVersion.hpp>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

// Dumps reflection data to the .jmap JSON format (https://github.com/trumank/jmap).
// The output mirrors the layout produced by jmap_dumper/serde so existing jmap consumers
// (ue_binja, jmap CLI conversions to .usmap/headers) work on files produced here.

namespace RC::JMapGenerator
{
    using namespace ::RC::Unreal;

    namespace
    {
        // ============================================================================
        // Minimal ordered JSON DOM + serde_json-compatible writer
        // ============================================================================
        struct JsonValue;

        using JsonArray = std::vector<JsonValue>;

        struct JsonObject
        {
            std::vector<std::pair<std::string, JsonValue>> members;

            auto add(std::string key, JsonValue value) -> void;
            auto find(std::string_view key) -> JsonValue*;
        };

        struct JsonValue
        {
            std::variant<std::nullptr_t, bool, int64_t, uint64_t, float, double, std::string, JsonArray, JsonObject> value{nullptr};

            JsonValue() = default;
            JsonValue(std::nullptr_t) : value(nullptr) {}
            JsonValue(bool v) : value(v) {}
            JsonValue(int64_t v) : value(v) {}
            JsonValue(uint64_t v) : value(v) {}
            JsonValue(float v) : value(v) {}
            JsonValue(double v) : value(v) {}
            JsonValue(std::string v) : value(std::move(v)) {}
            JsonValue(const char* v) : value(std::string{v}) {}
            JsonValue(JsonArray v) : value(std::move(v)) {}
            JsonValue(JsonObject v) : value(std::move(v)) {}
        };

        auto JsonObject::add(std::string key, JsonValue value) -> void
        {
            members.emplace_back(std::move(key), std::move(value));
        }

        auto JsonObject::find(std::string_view key) -> JsonValue*
        {
            for (auto& [k, v] : members)
            {
                if (k == key)
                {
                    return &v;
                }
            }
            return nullptr;
        }

        template <typename Sink>
        auto json_escape_to(Sink& out, std::string_view s) -> void
        {
            out.push_back('"');
            for (unsigned char c : s)
            {
                switch (c)
                {
                case '"':
                    out.append("\\\"");
                    break;
                case '\\':
                    out.append("\\\\");
                    break;
                case '\b':
                    out.append("\\b");
                    break;
                case '\f':
                    out.append("\\f");
                    break;
                case '\n':
                    out.append("\\n");
                    break;
                case '\r':
                    out.append("\\r");
                    break;
                case '\t':
                    out.append("\\t");
                    break;
                default:
                    if (c < 0x20)
                    {
                        char buf[8];
                        std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                        out.append(buf);
                    }
                    else
                    {
                        out.push_back(static_cast<char>(c));
                    }
                }
            }
            out.push_back('"');
        }

        template <typename Sink, typename FloatType>
        auto json_float_to(Sink& out, FloatType v) -> void
        {
            // serde_json serializes non-finite floats as null and always keeps a fractional part
            if (!std::isfinite(v))
            {
                out.append("null");
                return;
            }
            char buf[64];
            auto result = std::to_chars(buf, buf + sizeof(buf), v);
            std::string_view text{buf, static_cast<size_t>(result.ptr - buf)};
            out.append(text);
            if (text.find_first_of(".eE") == std::string_view::npos)
            {
                out.append(".0");
            }
        }

        // A small sink abstraction so the same writer can fill a std::string (compact sort keys)
        // or stream to the output file (pretty output).
        struct StringSink
        {
            std::string& out;
            auto push_back(char c) -> void
            {
                out.push_back(c);
            }
            auto append(std::string_view s) -> void
            {
                out.append(s);
            }
        };

        struct StreamSink
        {
            std::ostream& os;
            auto push_back(char c) -> void
            {
                os.put(c);
            }
            auto append(std::string_view s) -> void
            {
                os.write(s.data(), static_cast<std::streamsize>(s.size()));
            }
        };

        // indent_level < 0 -> compact; otherwise pretty with 2-space indentation (serde_json style)
        template <typename Sink>
        auto write_json(Sink& out, const JsonValue& value, int indent_level) -> void
        {
            auto write_indent = [&](int level) {
                out.push_back('\n');
                for (int i = 0; i < level * 2; ++i)
                {
                    out.push_back(' ');
                }
            };

            std::visit(
                    [&](const auto& v) {
                        using T = std::decay_t<decltype(v)>;
                        if constexpr (std::is_same_v<T, std::nullptr_t>)
                        {
                            out.append("null");
                        }
                        else if constexpr (std::is_same_v<T, bool>)
                        {
                            out.append(v ? "true" : "false");
                        }
                        else if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>)
                        {
                            char buf[32];
                            auto result = std::to_chars(buf, buf + sizeof(buf), v);
                            out.append(std::string_view{buf, static_cast<size_t>(result.ptr - buf)});
                        }
                        else if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>)
                        {
                            json_float_to(out, v);
                        }
                        else if constexpr (std::is_same_v<T, std::string>)
                        {
                            json_escape_to(out, v);
                        }
                        else if constexpr (std::is_same_v<T, JsonArray>)
                        {
                            if (v.empty())
                            {
                                out.append("[]");
                                return;
                            }
                            out.push_back('[');
                            bool first = true;
                            for (const auto& element : v)
                            {
                                if (!first)
                                {
                                    out.push_back(',');
                                }
                                first = false;
                                if (indent_level >= 0)
                                {
                                    write_indent(indent_level + 1);
                                }
                                write_json(out, element, indent_level >= 0 ? indent_level + 1 : -1);
                            }
                            if (indent_level >= 0)
                            {
                                write_indent(indent_level);
                            }
                            out.push_back(']');
                        }
                        else if constexpr (std::is_same_v<T, JsonObject>)
                        {
                            if (v.members.empty())
                            {
                                out.append("{}");
                                return;
                            }
                            out.push_back('{');
                            bool first = true;
                            for (const auto& [key, member] : v.members)
                            {
                                if (!first)
                                {
                                    out.push_back(',');
                                }
                                first = false;
                                if (indent_level >= 0)
                                {
                                    write_indent(indent_level + 1);
                                }
                                json_escape_to(out, key);
                                out.push_back(':');
                                if (indent_level >= 0)
                                {
                                    out.push_back(' ');
                                }
                                write_json(out, member, indent_level >= 0 ? indent_level + 1 : -1);
                            }
                            if (indent_level >= 0)
                            {
                                write_indent(indent_level);
                            }
                            out.push_back('}');
                        }
                    },
                    value.value);
        }

        auto to_compact_json(const JsonValue& value) -> std::string
        {
            std::string out{};
            StringSink sink{out};
            write_json(sink, value, -1);
            return out;
        }

        auto hex_address(uint64_t address) -> std::string
        {
            char buf[32];
            auto result = std::to_chars(buf, buf + sizeof(buf), address, 16);
            return std::string{"0x"} + std::string{buf, static_cast<size_t>(result.ptr - buf)};
        }

        auto base64_encode(const uint8_t* data, size_t size) -> std::string
        {
            static constexpr char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
            std::string out{};
            out.reserve(((size + 2) / 3) * 4);
            size_t i = 0;
            for (; i + 3 <= size; i += 3)
            {
                uint32_t n = (data[i] << 16) | (data[i + 1] << 8) | data[i + 2];
                out.push_back(alphabet[(n >> 18) & 63]);
                out.push_back(alphabet[(n >> 12) & 63]);
                out.push_back(alphabet[(n >> 6) & 63]);
                out.push_back(alphabet[n & 63]);
            }
            if (i + 1 == size)
            {
                uint32_t n = data[i] << 16;
                out.push_back(alphabet[(n >> 18) & 63]);
                out.push_back(alphabet[(n >> 12) & 63]);
                out.append("==");
            }
            else if (i + 2 == size)
            {
                uint32_t n = (data[i] << 16) | (data[i + 1] << 8);
                out.push_back(alphabet[(n >> 18) & 63]);
                out.push_back(alphabet[(n >> 12) & 63]);
                out.push_back(alphabet[(n >> 6) & 63]);
                out.push_back('=');
            }
            return out;
        }

        // ============================================================================
        // Flag stringification, replicating the bitflags crate's serde output
        // ("A | B | 0x8", zero flags -> ""). Tables mirror jmap/src/lib.rs verbatim,
        // including its definition order and intentionally missing bits.
        // ============================================================================
        struct FlagDef
        {
            const char* name;
            uint64_t bits;
        };

        auto flags_to_string(uint64_t bits, const FlagDef* defs, size_t def_count) -> std::string
        {
            std::string out{};
            uint64_t remaining = bits;
            for (size_t i = 0; i < def_count; ++i)
            {
                if (remaining == 0)
                {
                    break;
                }
                const auto& def = defs[i];
                if (def.bits == 0)
                {
                    continue;
                }
                if ((bits & def.bits) == def.bits)
                {
                    if (!out.empty())
                    {
                        out.append(" | ");
                    }
                    out.append(def.name);
                    remaining &= ~def.bits;
                }
            }
            if (remaining != 0)
            {
                if (!out.empty())
                {
                    out.append(" | ");
                }
                char buf[32];
                auto result = std::to_chars(buf, buf + sizeof(buf), remaining, 16);
                out.append("0x");
                out.append(buf, static_cast<size_t>(result.ptr - buf));
            }
            return out;
        }

        static constexpr FlagDef s_object_flag_defs[] = {
                {"RF_Public", 0x0001},
                {"RF_Standalone", 0x0002},
                {"RF_MarkAsNative", 0x0004},
                {"RF_Transactional", 0x0008},
                {"RF_ClassDefaultObject", 0x0010},
                {"RF_ArchetypeObject", 0x0020},
                {"RF_Transient", 0x0040},
                {"RF_MarkAsRootSet", 0x0080},
                {"RF_TagGarbageTemp", 0x0100},
                {"RF_NeedInitialization", 0x0200},
                {"RF_NeedLoad", 0x0400},
                {"RF_KeepForCooker", 0x0800},
                {"RF_NeedPostLoad", 0x1000},
                {"RF_NeedPostLoadSubobjects", 0x2000},
                {"RF_NewerVersionExists", 0x4000},
                {"RF_BeginDestroyed", 0x8000},
                {"RF_FinishDestroyed", 0x00010000},
                {"RF_BeingRegenerated", 0x00020000},
                {"RF_DefaultSubObject", 0x00040000},
                {"RF_WasLoaded", 0x00080000},
                {"RF_TextExportTransient", 0x00100000},
                {"RF_LoadCompleted", 0x00200000},
                {"RF_InheritableComponentTemplate", 0x00400000},
                {"RF_DuplicateTransient", 0x00800000},
                {"RF_StrongRefOnFrame", 0x01000000},
                {"RF_NonPIEDuplicateTransient", 0x02000000},
                {"RF_Dynamic", 0x04000000},
                {"RF_WillBeLoaded", 0x08000000},
        };

        static constexpr FlagDef s_function_flag_defs[] = {
                {"FUNC_Final", 0x0001},
                {"FUNC_RequiredAPI", 0x0002},
                {"FUNC_BlueprintAuthorityOnly", 0x0004},
                {"FUNC_BlueprintCosmetic", 0x0008},
                {"FUNC_Net", 0x0040},
                {"FUNC_NetReliable", 0x0080},
                {"FUNC_NetRequest", 0x0100},
                {"FUNC_Exec", 0x0200},
                {"FUNC_Native", 0x0400},
                {"FUNC_Event", 0x0800},
                {"FUNC_NetResponse", 0x1000},
                {"FUNC_Static", 0x2000},
                {"FUNC_NetMulticast", 0x4000},
                {"FUNC_UbergraphFunction", 0x8000},
                {"FUNC_MulticastDelegate", 0x00010000},
                {"FUNC_Public", 0x00020000},
                {"FUNC_Private", 0x00040000},
                {"FUNC_Protected", 0x00080000},
                {"FUNC_Delegate", 0x00100000},
                {"FUNC_NetServer", 0x00200000},
                {"FUNC_HasOutParms", 0x00400000},
                {"FUNC_HasDefaults", 0x00800000},
                {"FUNC_NetClient", 0x01000000},
                {"FUNC_DLLImport", 0x02000000},
                {"FUNC_BlueprintCallable", 0x04000000},
                {"FUNC_BlueprintEvent", 0x08000000},
                {"FUNC_BlueprintPure", 0x10000000},
                {"FUNC_EditorOnly", 0x20000000},
                {"FUNC_Const", 0x40000000},
                {"FUNC_NetValidate", 0x80000000},
                {"FUNC_AllFlags", 0xffffffff},
        };

        static constexpr FlagDef s_class_flag_defs[] = {
                {"CLASS_Abstract", 0x0001},
                {"CLASS_DefaultConfig", 0x0002},
                {"CLASS_Config", 0x0004},
                {"CLASS_Transient", 0x0008},
                {"CLASS_Parsed", 0x0010},
                {"CLASS_MatchedSerializers", 0x0020},
                {"CLASS_ProjectUserConfig", 0x0040},
                {"CLASS_Native", 0x0080},
                {"CLASS_NoExport", 0x0100},
                {"CLASS_NotPlaceable", 0x0200},
                {"CLASS_PerObjectConfig", 0x0400},
                {"CLASS_ReplicationDataIsSetUp", 0x0800},
                {"CLASS_EditInlineNew", 0x1000},
                {"CLASS_CollapseCategories", 0x2000},
                {"CLASS_Interface", 0x4000},
                {"CLASS_CustomConstructor", 0x8000},
                {"CLASS_Const", 0x00010000},
                {"CLASS_LayoutChanging", 0x00020000},
                {"CLASS_CompiledFromBlueprint", 0x00040000},
                {"CLASS_MinimalAPI", 0x00080000},
                {"CLASS_RequiredAPI", 0x00100000},
                {"CLASS_DefaultToInstanced", 0x00200000},
                {"CLASS_TokenStreamAssembled", 0x00400000},
                {"CLASS_HasInstancedReference", 0x00800000},
                {"CLASS_Hidden", 0x01000000},
                {"CLASS_Deprecated", 0x02000000},
                {"CLASS_HideDropDown", 0x04000000},
                {"CLASS_GlobalUserConfig", 0x08000000},
                {"CLASS_Intrinsic", 0x10000000},
                {"CLASS_Constructed", 0x20000000},
                {"CLASS_ConfigDoNotCheckDefaults", 0x40000000},
                {"CLASS_NewerVersionExists", 0x80000000},
        };

        static constexpr FlagDef s_class_cast_flag_defs[] = {
                {"CASTCLASS_UField", 0x0000000000000001},
                {"CASTCLASS_FInt8Property", 0x0000000000000002},
                {"CASTCLASS_UEnum", 0x0000000000000004},
                {"CASTCLASS_UStruct", 0x0000000000000008},
                {"CASTCLASS_UScriptStruct", 0x0000000000000010},
                {"CASTCLASS_UClass", 0x0000000000000020},
                {"CASTCLASS_FByteProperty", 0x0000000000000040},
                {"CASTCLASS_FIntProperty", 0x0000000000000080},
                {"CASTCLASS_FFloatProperty", 0x0000000000000100},
                {"CASTCLASS_FUInt64Property", 0x0000000000000200},
                {"CASTCLASS_FClassProperty", 0x0000000000000400},
                {"CASTCLASS_FUInt32Property", 0x0000000000000800},
                {"CASTCLASS_FInterfaceProperty", 0x0000000000001000},
                {"CASTCLASS_FNameProperty", 0x0000000000002000},
                {"CASTCLASS_FStrProperty", 0x0000000000004000},
                {"CASTCLASS_FProperty", 0x0000000000008000},
                {"CASTCLASS_FObjectProperty", 0x0000000000010000},
                {"CASTCLASS_FBoolProperty", 0x0000000000020000},
                {"CASTCLASS_FUInt16Property", 0x0000000000040000},
                {"CASTCLASS_UFunction", 0x0000000000080000},
                {"CASTCLASS_FStructProperty", 0x0000000000100000},
                {"CASTCLASS_FArrayProperty", 0x0000000000200000},
                {"CASTCLASS_FInt64Property", 0x0000000000400000},
                {"CASTCLASS_FDelegateProperty", 0x0000000000800000},
                {"CASTCLASS_FNumericProperty", 0x0000000001000000},
                {"CASTCLASS_FMulticastDelegateProperty", 0x0000000002000000},
                {"CASTCLASS_FObjectPropertyBase", 0x0000000004000000},
                {"CASTCLASS_FWeakObjectProperty", 0x0000000008000000},
                {"CASTCLASS_FLazyObjectProperty", 0x0000000010000000},
                {"CASTCLASS_FSoftObjectProperty", 0x0000000020000000},
                {"CASTCLASS_FTextProperty", 0x0000000040000000},
                {"CASTCLASS_FInt16Property", 0x0000000080000000},
                {"CASTCLASS_FDoubleProperty", 0x0000000100000000},
                {"CASTCLASS_FSoftClassProperty", 0x0000000200000000},
                {"CASTCLASS_UPackage", 0x0000000400000000},
                {"CASTCLASS_ULevel", 0x0000000800000000},
                {"CASTCLASS_AActor", 0x0000001000000000},
                {"CASTCLASS_APlayerController", 0x0000002000000000},
                {"CASTCLASS_APawn", 0x0000004000000000},
                {"CASTCLASS_USceneComponent", 0x0000008000000000},
                {"CASTCLASS_UPrimitiveComponent", 0x0000010000000000},
                {"CASTCLASS_USkinnedMeshComponent", 0x0000020000000000},
                {"CASTCLASS_USkeletalMeshComponent", 0x0000040000000000},
                {"CASTCLASS_UBlueprint", 0x0000080000000000},
                {"CASTCLASS_UDelegateFunction", 0x0000100000000000},
                {"CASTCLASS_UStaticMeshComponent", 0x0000200000000000},
                {"CASTCLASS_FMapProperty", 0x0000400000000000},
                {"CASTCLASS_FSetProperty", 0x0000800000000000},
                {"CASTCLASS_FEnumProperty", 0x0001000000000000},
                {"CASTCLASS_USparseDelegateFunction", 0x0002000000000000},
                {"CASTCLASS_FMulticastInlineDelegateProperty", 0x0004000000000000},
                {"CASTCLASS_FMulticastSparseDelegateProperty", 0x0008000000000000},
                {"CASTCLASS_FFieldPathProperty", 0x0010000000000000},
                {"CASTCLASS_FLargeWorldCoordinatesRealProperty", 0x0080000000000000},
                {"CASTCLASS_FOptionalProperty", 0x0100000000000000},
                {"CASTCLASS_FVerseValueProperty", 0x0200000000000000},
                {"CASTCLASS_FVRestValueProperty", 0x0400000000000000},
                {"CASTCLASS_FVerseStringProperty", 0x0800000000000000},
                {"CASTCLASS_FUtf8StrProperty", 0x1000000000000000},
                {"CASTCLASS_FAnsiStrProperty", 0x2000000000000000},
                {"CASTCLASS_FVCellProperty", 0x4000000000000000},
        };

        static constexpr FlagDef s_property_flag_defs[] = {
                {"CPF_Edit", 0x0001},
                {"CPF_ConstParm", 0x0002},
                {"CPF_BlueprintVisible", 0x0004},
                {"CPF_ExportObject", 0x0008},
                {"CPF_BlueprintReadOnly", 0x0010},
                {"CPF_Net", 0x0020},
                {"CPF_EditFixedSize", 0x0040},
                {"CPF_Parm", 0x0080},
                {"CPF_OutParm", 0x0100},
                {"CPF_ZeroConstructor", 0x0200},
                {"CPF_ReturnParm", 0x0400},
                {"CPF_DisableEditOnTemplate", 0x0800},
                {"CPF_NonNullable", 0x1000},
                {"CPF_Transient", 0x2000},
                {"CPF_Config", 0x4000},
                {"CPF_RequiredParm", 0x8000},
                {"CPF_DisableEditOnInstance", 0x00010000},
                {"CPF_EditConst", 0x00020000},
                {"CPF_GlobalConfig", 0x00040000},
                {"CPF_InstancedReference", 0x00080000},
                {"CPF_ExperimentalExternalObjects", 0x00100000},
                {"CPF_DuplicateTransient", 0x00200000},
                {"CPF_SaveGame", 0x01000000},
                {"CPF_NoClear", 0x02000000},
                {"CPF_Virtual", 0x04000000},
                {"CPF_ReferenceParm", 0x08000000},
                {"CPF_BlueprintAssignable", 0x10000000},
                {"CPF_Deprecated", 0x20000000},
                {"CPF_IsPlainOldData", 0x40000000},
                {"CPF_RepSkip", 0x80000000},
                {"CPF_RepNotify", 0x100000000},
                {"CPF_Interp", 0x200000000},
                {"CPF_NonTransactional", 0x400000000},
                {"CPF_EditorOnly", 0x800000000},
                {"CPF_NoDestructor", 0x1000000000},
                {"CPF_AutoWeak", 0x4000000000},
                {"CPF_ContainsInstancedReference", 0x8000000000},
                {"CPF_AssetRegistrySearchable", 0x10000000000},
                {"CPF_SimpleDisplay", 0x20000000000},
                {"CPF_AdvancedDisplay", 0x40000000000},
                {"CPF_Protected", 0x80000000000},
                {"CPF_BlueprintCallable", 0x100000000000},
                {"CPF_BlueprintAuthorityOnly", 0x200000000000},
                {"CPF_TextExportTransient", 0x400000000000},
                {"CPF_NonPIEDuplicateTransient", 0x800000000000},
                {"CPF_ExposeOnSpawn", 0x1000000000000},
                {"CPF_PersistentInstance", 0x2000000000000},
                {"CPF_UObjectWrapper", 0x4000000000000},
                {"CPF_HasGetValueTypeHash", 0x8000000000000},
                {"CPF_NativeAccessSpecifierPublic", 0x10000000000000},
                {"CPF_NativeAccessSpecifierProtected", 0x20000000000000},
                {"CPF_NativeAccessSpecifierPrivate", 0x40000000000000},
                {"CPF_SkipSerialization", 0x80000000000000},
                {"CPF_TObjectPtr", 0x100000000000000},
                {"CPF_ExperimentalOverridableLogic", 0x200000000000000},
                {"CPF_ExperimentalAlwaysOverriden", 0x400000000000000},
                {"CPF_ExperimentalNeverOverriden", 0x800000000000000},
                {"CPF_AllowSelfReference", 0x1000000000000000},
        };

        static constexpr FlagDef s_struct_flag_defs[] = {
                {"STRUCT_Native", 0x0001},
                {"STRUCT_IdenticalNative", 0x0002},
                {"STRUCT_HasInstancedReference", 0x0004},
                {"STRUCT_NoExport", 0x0008},
                {"STRUCT_Atomic", 0x0010},
                {"STRUCT_Immutable", 0x0020},
                {"STRUCT_AddStructReferencedObjects", 0x0040},
                {"STRUCT_RequiredAPI", 0x0200},
                {"STRUCT_NetSerializeNative", 0x0400},
                {"STRUCT_SerializeNative", 0x0800},
                {"STRUCT_CopyNative", 0x1000},
                {"STRUCT_IsPlainOldData", 0x2000},
                {"STRUCT_NoDestructor", 0x4000},
                {"STRUCT_ZeroConstructor", 0x8000},
                {"STRUCT_ExportTextItemNative", 0x00010000},
                {"STRUCT_ImportTextItemNative", 0x00020000},
                {"STRUCT_PostSerializeNative", 0x00040000},
                {"STRUCT_SerializeFromMismatchedTag", 0x00080000},
                {"STRUCT_NetDeltaSerializeNative", 0x00100000},
                {"STRUCT_PostScriptConstruct", 0x00200000},
                {"STRUCT_NetSharedSerialization", 0x00400000},
                {"STRUCT_Trashed", 0x00800000},
                {"STRUCT_Inherit", 0x0014},
                {"STRUCT_ComputedFlags", 0x007ffc42},
        };

        static constexpr FlagDef s_enum_flag_defs[] = {
                {"Flags", 0x00000001},
                {"NewerVersionExists", 0x00000002},
        };

        template <size_t N>
        auto flags_string(uint64_t bits, const FlagDef (&defs)[N]) -> std::string
        {
            return flags_to_string(bits, defs, N);
        }

        // ============================================================================
        // Memory validity checks (region cache over VirtualQuery)
        // ============================================================================
        class MemoryChecker
        {
        public:
            auto is_readable(const void* address, size_t size = 1) -> bool
            {
                auto begin = reinterpret_cast<uint64_t>(address);
                if (begin == 0)
                {
                    return false;
                }
                uint64_t end = begin + (size == 0 ? 1 : size);
                uint64_t current = begin;
                while (current < end)
                {
                    const Region* region = query(current);
                    if (!region || !region->readable)
                    {
                        return false;
                    }
                    current = region->end;
                }
                return true;
            }

            auto is_executable(const void* address) -> bool
            {
                const Region* region = query(reinterpret_cast<uint64_t>(address));
                return region && region->executable;
            }

        private:
            struct Region
            {
                uint64_t base{};
                uint64_t end{};
                bool readable{};
                bool executable{};
            };

            auto query(uint64_t address) -> const Region*
            {
                if (address == 0)
                {
                    return nullptr;
                }
                auto it = m_regions.upper_bound(address);
                if (it != m_regions.begin())
                {
                    auto& candidate = std::prev(it)->second;
                    if (address >= candidate.base && address < candidate.end)
                    {
                        return &candidate;
                    }
                }

                MEMORY_BASIC_INFORMATION info{};
                if (VirtualQuery(reinterpret_cast<LPCVOID>(address), &info, sizeof(info)) == 0)
                {
                    return nullptr;
                }
                Region region{};
                region.base = reinterpret_cast<uint64_t>(info.BaseAddress);
                region.end = region.base + info.RegionSize;
                if (info.State == MEM_COMMIT && !(info.Protect & (PAGE_NOACCESS | PAGE_GUARD)))
                {
                    region.readable = (info.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE |
                                                       PAGE_EXECUTE_WRITECOPY)) != 0;
                    region.executable = (info.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) != 0;
                }
                auto [inserted, _] = m_regions.insert_or_assign(region.base, region);
                return &inserted->second;
            }

            std::map<uint64_t, Region> m_regions{};
        };

        // Raw views of engine containers holding property values; layouts are identical across supported versions.
        struct ScriptArrayView
        {
            void* Data;
            int32_t ArrayNum;
            int32_t ArrayMax;
        };


        // ============================================================================
        // The dumper
        // ============================================================================
        constexpr int32_t MaxSaneContainerElements = 0x4000000;

        struct DumpedObject
        {
            uint64_t address{};
            uint64_t vtable{};
            std::string class_path{};
            std::optional<std::string> outer{};
            std::optional<std::string> super_struct{};
            std::optional<std::string> class_default_object{};
            bool is_class{};
            JsonValue json{};
        };

        class JMapDumper
        {
        public:
            JMapDumper(bool include_blueprint_types, bool skip_property_values)
                : m_include_blueprint_types(include_blueprint_types), m_skip_property_values(skip_property_values)
            {
            }

            auto dump() -> void;

        private:
            auto path_of(UObject* object) -> const std::string&;
            auto is_live_object(UObject* object) -> bool;
            auto enum_names_of(UEnum* uenum) -> const std::vector<std::pair<std::string, int64_t>>&;

            auto should_dump(UObject* object, const std::string& path) -> bool;
            auto dump_object_entry(UObject* object, const std::string& path) -> std::optional<DumpedObject>;
            auto insert_object(std::string path, DumpedObject entry) -> void;
            auto schedule_extras(UObject* object) -> void;

            auto build_object_fields(JsonObject& out, UObject* object) -> void;
            auto build_struct_fields(JsonObject& out, UStruct* ustruct, DumpedObject& entry) -> void;

            auto dump_property(FProperty* property) -> std::optional<JsonValue>;
            auto dump_property_values(UStruct* ustruct, void* container) -> JsonValue;
            auto dump_property_value(FProperty* property, void* container, int32_t index) -> std::optional<JsonValue>;

            auto engine_fname_layout_matches() -> bool;
            auto analyze_vtables() -> std::map<uint64_t, std::vector<uint64_t>>;
            auto write_output(const std::map<uint64_t, std::vector<uint64_t>>& vtables) -> void;

            auto warn(const std::string& message) -> void
            {
                ++m_warning_count;
                // A single version-missing member repeats across thousands of objects, so log each
                // distinct message once and let the final summary report the true count.
                if (!m_warned_messages.insert(message).second)
                {
                    return;
                }
                Output::send<LogLevel::Warning>(STR("[JMapGenerator] {}\n"), ensure_str(message));
            }

            bool m_include_blueprint_types{};
            bool m_skip_property_values{};
            MemoryChecker m_memory{};
            std::unordered_map<const UObject*, std::string> m_path_cache{};
            std::unordered_map<UEnum*, std::vector<std::pair<std::string, int64_t>>> m_enum_names_cache{};
            std::map<std::string, DumpedObject> m_objects{};
            std::unordered_set<UObject*> m_scheduled{};
            std::vector<UObject*> m_extra_objects{};
            std::unordered_set<std::string> m_warned_messages{};
            uint64_t m_warning_count{};
        };

        auto JMapDumper::path_of(UObject* object) -> const std::string&
        {
            if (auto it = m_path_cache.find(object); it != m_path_cache.end())
            {
                return it->second;
            }

            std::string name = to_utf8_string(object->GetNamePrivate().ToString());
            UObject* outer = object->GetOuterPrivate();
            std::string path{};
            if (outer)
            {
                // jmap separator convention: '.' after packages, ':' after everything else
                const std::string& outer_path = path_of(outer);
                UClass* outer_class = outer->GetClassPrivate();
                bool outer_is_package = outer_class && (outer_class->GetClassCastFlags() & CASTCLASS_UPackage) != 0;
                path.reserve(outer_path.size() + 1 + name.size());
                path.append(outer_path);
                path.push_back(outer_is_package ? '.' : ':');
                path.append(name);
            }
            else
            {
                path = std::move(name);
            }
            return m_path_cache.emplace(object, std::move(path)).first->second;
        }

        auto JMapDumper::is_live_object(UObject* object) -> bool
        {
            if (!object || (reinterpret_cast<uintptr_t>(object) & 0x7) != 0)
            {
                return false;
            }
            if (!m_memory.is_readable(object, 0x30))
            {
                return false;
            }
            int32_t index = object->GetInternalIndex();
            if (index < 0 || index >= FUObjectArray::GetNumElements())
            {
                return false;
            }
            FUObjectItem* item = FUObjectArray::IndexToObject(index);
            return item && item->GetUObject() == object;
        }

        auto JMapDumper::enum_names_of(UEnum* uenum) -> const std::vector<std::pair<std::string, int64_t>>&
        {
            if (auto it = m_enum_names_cache.find(uenum); it != m_enum_names_cache.end())
            {
                return it->second;
            }
            std::vector<std::pair<std::string, int64_t>> names{};
            for (auto& [key, value] : uenum->ForEachName())
            {
                names.emplace_back(to_utf8_string(key.ToString()), value);
            }
            return m_enum_names_cache.emplace(uenum, std::move(names)).first->second;
        }

        auto JMapDumper::should_dump(UObject* object, const std::string& path) -> bool
        {
            if (path.starts_with("/Script/"))
            {
                return true;
            }
            if (!m_include_blueprint_types)
            {
                return false;
            }
            // Blueprint mode: additionally take dynamically generated reflection types
            // (BlueprintGeneratedClass instances, user defined structs/enums, their functions)
            uint32_t object_flags = static_cast<uint32_t>(object->GetObjectFlags());
            if ((object_flags & (RF_ClassDefaultObject | RF_ArchetypeObject)) != 0)
            {
                return false;
            }
            UClass* object_class = object->GetClassPrivate();
            uint64_t cast_flags = object_class->GetClassCastFlags();
            return (cast_flags & (CASTCLASS_UClass | CASTCLASS_UScriptStruct | CASTCLASS_UEnum | CASTCLASS_UFunction)) != 0;
        }

        auto JMapDumper::schedule_extras(UObject* object) -> void
        {
            // Pull in the closure needed for a self-contained blueprint dump: outer chain
            // (packages) and, for classes, the CDO holding the default property values.
            for (UObject* outer = object->GetOuterPrivate(); outer; outer = outer->GetOuterPrivate())
            {
                if (m_scheduled.insert(outer).second)
                {
                    m_extra_objects.push_back(outer);
                }
            }
            UClass* object_class = object->GetClassPrivate();
            if (object_class && (object_class->GetClassCastFlags() & CASTCLASS_UClass) != 0)
            {
                if (UObject* cdo = static_cast<UClass*>(object)->GetClassDefaultObject())
                {
                    if (m_scheduled.insert(cdo).second)
                    {
                        m_extra_objects.push_back(cdo);
                    }
                }
            }
        }

        auto JMapDumper::build_object_fields(JsonObject& out, UObject* object) -> void
        {
            out.add("address", hex_address(reinterpret_cast<uint64_t>(object)));
            out.add("vtable", hex_address(*reinterpret_cast<uint64_t*>(object)));
            out.add("object_flags", flags_string(static_cast<uint32_t>(object->GetObjectFlags()), s_object_flag_defs));
            if (UObject* outer = object->GetOuterPrivate())
            {
                out.add("outer", path_of(outer));
            }
            else
            {
                out.add("outer", nullptr);
            }
            UClass* object_class = object->GetClassPrivate();
            out.add("class", path_of(object_class));
            out.add("children", JsonArray{});
            out.add("property_values", dump_property_values(object_class, object));
        }

        auto JMapDumper::build_struct_fields(JsonObject& out, UStruct* ustruct, DumpedObject& entry) -> void
        {
            if (UStruct* super_struct = ustruct->GetSuperStruct())
            {
                entry.super_struct = path_of(super_struct);
                out.add("super_struct", *entry.super_struct);
            }
            else
            {
                out.add("super_struct", nullptr);
            }

            JsonArray properties{};
            for (FProperty* property : TFieldRange<FProperty>(ustruct, EFieldIterationFlags::IncludeDeprecated))
            {
                // Contain failures to the property that caused them. A member the running engine
                // version doesn't have would otherwise take the whole class out of the dump.
                try
                {
                    if (auto property_json = dump_property(property))
                    {
                        properties.push_back(std::move(*property_json));
                    }
                }
                catch (std::exception& e)
                {
                    warn(std::string{"failed to dump property type: "} + e.what());
                }
            }
            out.add("properties", std::move(properties));
            out.add("properties_size", static_cast<int64_t>(ustruct->GetPropertiesSize()));
            out.add("min_alignment", static_cast<int64_t>(ustruct->GetMinAlignment()));

            std::string script{};
            auto& script_array = ustruct->GetScript();
            if (script_array.Num() > 0 && script_array.GetData() && m_memory.is_readable(script_array.GetData(), static_cast<size_t>(script_array.Num())))
            {
                script = base64_encode(reinterpret_cast<const uint8_t*>(script_array.GetData()), static_cast<size_t>(script_array.Num()));
            }
            out.add("script", std::move(script));
        }

        auto JMapDumper::dump_object_entry(UObject* object, const std::string& path) -> std::optional<DumpedObject>
        {
            UClass* object_class = object->GetClassPrivate();
            if (!object_class)
            {
                return std::nullopt;
            }

            DumpedObject entry{};
            entry.address = reinterpret_cast<uint64_t>(object);
            entry.vtable = *reinterpret_cast<uint64_t*>(object);
            entry.class_path = path_of(object_class);
            if (UObject* outer = object->GetOuterPrivate())
            {
                entry.outer = path_of(outer);
            }

            uint32_t object_flags = static_cast<uint32_t>(object->GetObjectFlags());
            bool is_basic_object = (object_flags & (RF_ClassDefaultObject | RF_ArchetypeObject)) != 0;
            uint64_t cast_flags = object_class->GetClassCastFlags();

            JsonObject json{};
            if (!is_basic_object && (cast_flags & CASTCLASS_UClass) != 0)
            {
                auto* uclass = static_cast<UClass*>(object);
                json.add("type", "Class");
                build_object_fields(json, object);
                build_struct_fields(json, uclass, entry);
                json.add("class_flags", flags_string(static_cast<uint32_t>(uclass->GetClassFlags()), s_class_flag_defs));
                json.add("class_cast_flags", flags_string(uclass->GetClassCastFlags(), s_class_cast_flag_defs));
                if (UObject* cdo = uclass->GetClassDefaultObject())
                {
                    entry.class_default_object = path_of(cdo);
                    json.add("class_default_object", *entry.class_default_object);
                }
                else
                {
                    json.add("class_default_object", nullptr);
                }
                json.add("instance_vtable", nullptr);
                JsonArray interfaces{};
                for (auto& implemented_interface : uclass->GetInterfaces())
                {
                    JsonObject interface_json{};
                    if (implemented_interface.Class)
                    {
                        interface_json.add("class", path_of(implemented_interface.Class));
                    }
                    else
                    {
                        interface_json.add("class", nullptr);
                    }
                    interface_json.add("pointer_offset", static_cast<int64_t>(implemented_interface.PointerOffset));
                    interface_json.add("implemented_by_k2", implemented_interface.bImplementedByK2);
                    interfaces.push_back(std::move(interface_json));
                }
                json.add("interfaces", std::move(interfaces));
                entry.is_class = true;
            }
            else if (!is_basic_object && (cast_flags & CASTCLASS_UFunction) != 0)
            {
                auto* ufunction = static_cast<UFunction*>(object);
                json.add("type", "Function");
                build_object_fields(json, object);
                build_struct_fields(json, ufunction, entry);
                json.add("function_flags", flags_string(ufunction->GetFunctionFlags(), s_function_flag_defs));
                json.add("func", hex_address(reinterpret_cast<uint64_t>(ufunction->GetFuncPtr())));
            }
            else if (!is_basic_object && (cast_flags & CASTCLASS_UScriptStruct) != 0)
            {
                auto* uscriptstruct = static_cast<UScriptStruct*>(object);
                json.add("type", "ScriptStruct");
                build_object_fields(json, object);
                build_struct_fields(json, uscriptstruct, entry);
                json.add("struct_flags", flags_string(static_cast<uint32_t>(uscriptstruct->GetStructFlags()), s_struct_flag_defs));
            }
            else if (!is_basic_object && (cast_flags & CASTCLASS_UEnum) != 0)
            {
                auto* uenum = static_cast<UEnum*>(object);
                json.add("type", "Enum");
                build_object_fields(json, object);
                auto& cpp_type = uenum->GetCppType();
                auto* cpp_type_view = reinterpret_cast<const ScriptArrayView*>(&cpp_type);
                std::string cpp_type_utf8{};
                if (cpp_type_view->Data && cpp_type_view->ArrayNum > 0)
                {
                    cpp_type_utf8 = to_utf8_string(StringType{static_cast<const TCHAR*>(cpp_type_view->Data), static_cast<size_t>(cpp_type_view->ArrayNum - 1)});
                }
                json.add("cpp_type", std::move(cpp_type_utf8));
                if (Version::IsAtLeast(4, 26))
                {
                    json.add("enum_flags", flags_string(static_cast<uint8_t>(uenum->GetEnumFlags()), s_enum_flag_defs));
                }
                else
                {
                    json.add("enum_flags", nullptr);
                }
                const char* cpp_form = "Regular";
                switch (uenum->GetCppForm())
                {
                case UEnum::ECppForm::Regular:
                    cpp_form = "Regular";
                    break;
                case UEnum::ECppForm::Namespaced:
                    cpp_form = "Namespaced";
                    break;
                case UEnum::ECppForm::EnumClass:
                    cpp_form = "EnumClass";
                    break;
                }
                json.add("cpp_form", cpp_form);
                JsonArray names{};
                for (const auto& [name, value] : enum_names_of(uenum))
                {
                    JsonArray pair{};
                    pair.push_back(name);
                    pair.push_back(static_cast<int64_t>(value));
                    names.push_back(std::move(pair));
                }
                json.add("names", std::move(names));
            }
            else if (!is_basic_object && (cast_flags & CASTCLASS_UPackage) != 0)
            {
                json.add("type", "Package");
                build_object_fields(json, object);
            }
            else
            {
                json.add("type", "Object");
                build_object_fields(json, object);
            }

            entry.json = JsonValue{std::move(json)};
            return entry;
        }

        auto has_canonical_cdo(const std::string& class_path, const DumpedObject& entry) -> bool
        {
            if (!entry.is_class || !entry.class_default_object)
            {
                return false;
            }
            auto separator = class_path.find_last_of(".:");
            if (separator == std::string::npos)
            {
                return false;
            }
            std::string expected = class_path.substr(0, separator) + ".Default__" + class_path.substr(separator + 1);
            return *entry.class_default_object == expected;
        }

        auto JMapDumper::insert_object(std::string path, DumpedObject entry) -> void
        {
            auto it = m_objects.find(path);
            if (it == m_objects.end())
            {
                m_objects.emplace(std::move(path), std::move(entry));
                return;
            }
            // UE normally guarantees one object per path, but plugins can collide paths via
            // LowLevelRename; prefer the class whose CDO still lives at the canonical path.
            bool prefer_new = has_canonical_cdo(path, entry) && !has_canonical_cdo(path, it->second);
            warn("path collision " + path + ": existing " + hex_address(it->second.address) + ", new " + hex_address(entry.address));
            if (prefer_new)
            {
                it->second = std::move(entry);
            }
        }

        // ============================================================================
        // Property (type) dumping
        // ============================================================================
        auto JMapDumper::dump_property(FProperty* property) -> std::optional<JsonValue>
        {
            // The jmap schema types some object references as a plain string rather than an optional
            // one (a property's struct/property_class/meta_class/interface_class). Emitting null for
            // those makes the file fail to deserialize, so fall back to UE's own spelling for an
            // absent object reference. Used where the schema has no null case; object_path_or_null is
            // for the genuinely optional fields.
            auto object_path_or_none = [&](UObject* object) -> JsonValue {
                if (!object)
                {
                    return std::string{"None"};
                }
                return path_of(object);
            };

            auto object_path_or_null = [&](UObject* object) -> JsonValue {
                if (!object)
                {
                    return nullptr;
                }
                return path_of(object);
            };

            JsonObject json{};
            json.add("address", hex_address(reinterpret_cast<uint64_t>(property)));
            json.add("name", to_utf8_string(property->GetFName().ToString()));
            json.add("offset", static_cast<int64_t>(property->GetOffset_Internal()));
            json.add("array_dim", static_cast<int64_t>(property->GetArrayDim()));
            json.add("size", static_cast<int64_t>(property->GetElementSize()));

            auto property_class = property->GetClass();
            if (property_class.HasAnyCastFlags(CASTCLASS_FStructProperty))
            {
                json.add("type", "StructProperty");
                json.add("struct", object_path_or_none(static_cast<FStructProperty*>(property)->GetStruct()));
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FStrProperty))
            {
                json.add("type", "StrProperty");
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FNameProperty))
            {
                json.add("type", "NameProperty");
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FTextProperty))
            {
                json.add("type", "TextProperty");
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FMulticastInlineDelegateProperty))
            {
                json.add("type", "MulticastInlineDelegateProperty");
                json.add("signature_function", object_path_or_null(static_cast<FMulticastDelegateProperty*>(property)->GetSignatureFunction()));
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FMulticastSparseDelegateProperty))
            {
                json.add("type", "MulticastSparseDelegateProperty");
                json.add("signature_function", object_path_or_null(static_cast<FMulticastDelegateProperty*>(property)->GetSignatureFunction()));
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FMulticastDelegateProperty))
            {
                json.add("type", "MulticastDelegateProperty");
                json.add("signature_function", object_path_or_null(static_cast<FMulticastDelegateProperty*>(property)->GetSignatureFunction()));
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FDelegateProperty))
            {
                json.add("type", "DelegateProperty");
                json.add("signature_function", object_path_or_null(static_cast<FDelegateProperty*>(property)->GetSignatureFunction()));
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FBoolProperty))
            {
                auto* bool_property = static_cast<FBoolProperty*>(property);
                json.add("type", "BoolProperty");
                json.add("field_size", static_cast<int64_t>(bool_property->GetFieldSize()));
                json.add("byte_offset", static_cast<int64_t>(bool_property->GetByteOffset()));
                json.add("byte_mask", static_cast<int64_t>(bool_property->GetByteMask()));
                json.add("field_mask", static_cast<int64_t>(bool_property->GetFieldMask()));
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FArrayProperty))
            {
                json.add("type", "ArrayProperty");
                auto inner = dump_property(static_cast<FArrayProperty*>(property)->GetInner());
                if (!inner)
                {
                    return std::nullopt;
                }
                json.add("inner", std::move(*inner));
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FEnumProperty))
            {
                auto* enum_property = static_cast<FEnumProperty*>(property);
                json.add("type", "EnumProperty");
                auto container = dump_property(enum_property->GetUnderlyingProp());
                if (!container)
                {
                    return std::nullopt;
                }
                json.add("container", std::move(*container));
                json.add("enum", object_path_or_null(enum_property->GetEnum()));
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FMapProperty))
            {
                auto* map_property = static_cast<FMapProperty*>(property);
                json.add("type", "MapProperty");
                auto key_prop = dump_property(map_property->GetKeyProp());
                auto value_prop = dump_property(map_property->GetValueProp());
                if (!key_prop || !value_prop)
                {
                    return std::nullopt;
                }
                json.add("key_prop", std::move(*key_prop));
                json.add("value_prop", std::move(*value_prop));
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FSetProperty))
            {
                json.add("type", "SetProperty");
                auto key_prop = dump_property(static_cast<FSetProperty*>(property)->GetElementProp());
                if (!key_prop)
                {
                    return std::nullopt;
                }
                json.add("key_prop", std::move(*key_prop));
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FFloatProperty))
            {
                json.add("type", "FloatProperty");
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FDoubleProperty))
            {
                json.add("type", "DoubleProperty");
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FByteProperty))
            {
                json.add("type", "ByteProperty");
                json.add("enum", object_path_or_null(static_cast<FByteProperty*>(property)->GetEnum()));
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FUInt16Property))
            {
                json.add("type", "UInt16Property");
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FUInt32Property))
            {
                json.add("type", "UInt32Property");
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FUInt64Property))
            {
                json.add("type", "UInt64Property");
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FInt8Property))
            {
                json.add("type", "Int8Property");
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FInt16Property))
            {
                json.add("type", "Int16Property");
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FIntProperty))
            {
                json.add("type", "IntProperty");
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FInt64Property))
            {
                json.add("type", "Int64Property");
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FClassProperty))
            {
                auto* class_property = static_cast<FClassProperty*>(property);
                json.add("type", "ClassProperty");
                json.add("property_class", object_path_or_none(class_property->GetPropertyClass()));
                json.add("meta_class",
                         FClassProperty::MemberOffsets.contains(STR("MetaClass")) ? object_path_or_none(class_property->GetMetaClass()) : JsonValue{std::string{"None"}});
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FObjectProperty))
            {
                json.add("type", "ObjectProperty");
                json.add("property_class", object_path_or_none(static_cast<FObjectPropertyBase*>(property)->GetPropertyClass()));
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FSoftClassProperty))
            {
                auto* soft_class_property = static_cast<FSoftClassProperty*>(property);
                json.add("type", "SoftClassProperty");
                json.add("property_class", object_path_or_none(soft_class_property->GetPropertyClass()));
                // The field itself exists on every engine version. Pre-4.18 the class is named
                // AssetClassProperty, so UE4SS's lookup of "/Script/CoreUObject.SoftClassProperty"
                // finds nothing and never registers offsets for it. Emit None rather than lose the
                // property; registering the old class name would recover the real value.
                json.add("meta_class",
                         FSoftClassProperty::MemberOffsets.contains(STR("MetaClass")) ? object_path_or_none(soft_class_property->GetMetaClass())
                                                                                      : JsonValue{std::string{"None"}});
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FSoftObjectProperty))
            {
                json.add("type", "SoftObjectProperty");
                json.add("property_class", object_path_or_none(static_cast<FObjectPropertyBase*>(property)->GetPropertyClass()));
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FWeakObjectProperty))
            {
                json.add("type", "WeakObjectProperty");
                json.add("property_class", object_path_or_none(static_cast<FObjectPropertyBase*>(property)->GetPropertyClass()));
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FLazyObjectProperty))
            {
                json.add("type", "LazyObjectProperty");
                json.add("property_class", object_path_or_none(static_cast<FObjectPropertyBase*>(property)->GetPropertyClass()));
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FInterfaceProperty))
            {
                json.add("type", "InterfaceProperty");
                json.add("interface_class", object_path_or_none(static_cast<FInterfaceProperty*>(property)->GetInterfaceClass()));
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FFieldPathProperty))
            {
                json.add("type", "FieldPathProperty");
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FOptionalProperty))
            {
                json.add("type", "OptionalProperty");
                auto inner = dump_property(static_cast<FOptionalProperty*>(property)->GetValueProperty());
                if (!inner)
                {
                    return std::nullopt;
                }
                json.add("inner", std::move(*inner));
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FUtf8StrProperty))
            {
                json.add("type", "FUtf8StrProperty");
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FAnsiStrProperty))
            {
                json.add("type", "AnsiStrProperty");
            }
            else
            {
                warn("unhandled property class for '" + to_utf8_string(property->GetFName().ToString()) + "', skipping");
                return std::nullopt;
            }

            json.add("flags", flags_string(static_cast<uint64_t>(property->GetPropertyFlags()), s_property_flag_defs));
            return JsonValue{std::move(json)};
        }

        // ============================================================================
        // Property value dumping
        // ============================================================================
        auto JMapDumper::dump_property_values(UStruct* ustruct, void* container) -> JsonValue
        {
            JsonObject values{};
            if (!ustruct || !container || m_skip_property_values)
            {
                return JsonValue{std::move(values)};
            }
            // Shadowed property names across the super chain must replace in place (OrderMap semantics
            // in the reference dumper) instead of producing duplicate JSON keys
            auto add_value = [&values](std::string key, JsonValue value) {
                if (auto* existing = values.find(key))
                {
                    *existing = std::move(value);
                }
                else
                {
                    values.add(std::move(key), std::move(value));
                }
            };
            for (FProperty* property : TFieldRange<FProperty>(ustruct, EFieldIterationFlags::Default))
            {
                // Contain failures to the property that caused them, so one unreadable value
                // doesn't cost the whole object its property values.
                try
                {
                    int32_t array_dim = property->GetArrayDim();
                    if (array_dim == 1)
                    {
                        if (auto value = dump_property_value(property, container, 0))
                        {
                            add_value(to_utf8_string(property->GetFName().ToString()), std::move(*value));
                        }
                    }
                    else
                    {
                        JsonArray elements{};
                        bool success = true;
                        for (int32_t i = 0; i < array_dim; ++i)
                        {
                            if (auto value = dump_property_value(property, container, i))
                            {
                                elements.push_back(std::move(*value));
                            }
                            else
                            {
                                success = false;
                            }
                        }
                        if (success)
                        {
                            add_value(to_utf8_string(property->GetFName().ToString()), std::move(elements));
                        }
                    }
                }
                catch (std::exception& e)
                {
                    warn(std::string{"failed to dump property value: "} + e.what());
                }
            }
            return JsonValue{std::move(values)};
        }

        auto JMapDumper::dump_property_value(FProperty* property, void* container, int32_t index) -> std::optional<JsonValue>
        {
            int32_t element_size = property->GetElementSize();
            void* ptr = static_cast<uint8_t*>(container) + property->GetOffset_Internal() + static_cast<int64_t>(index) * element_size;
            if (element_size <= 0 || !m_memory.is_readable(ptr, static_cast<size_t>(element_size)))
            {
                return std::nullopt;
            }

            auto read_signed = [&](auto type_tag) -> int64_t {
                std::decay_t<decltype(type_tag)> raw{};
                std::memcpy(&raw, ptr, sizeof(raw));
                return static_cast<int64_t>(raw);
            };

            auto property_class = property->GetClass();
            if (property_class.HasAnyCastFlags(CASTCLASS_FStructProperty))
            {
                UScriptStruct* value_struct = static_cast<FStructProperty*>(property)->GetStruct();
                if (!value_struct)
                {
                    return std::nullopt;
                }
                return dump_property_values(value_struct, ptr);
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FStrProperty))
            {
                auto* view = static_cast<const ScriptArrayView*>(ptr);
                if (!view->Data || view->ArrayNum <= 0)
                {
                    return JsonValue{std::string{}};
                }
                if (view->ArrayNum > MaxSaneContainerElements || !m_memory.is_readable(view->Data, static_cast<size_t>(view->ArrayNum) * sizeof(TCHAR)))
                {
                    return std::nullopt;
                }
                return JsonValue{to_utf8_string(StringType{static_cast<const TCHAR*>(view->Data), static_cast<size_t>(view->ArrayNum - 1)})};
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FNameProperty))
            {
                // The engine's FName size is a runtime value, while this build's FName type has its
                // alignment (and therefore possibly its size) baked in. Copy the smallest of the
                // three so we never read past the property nor overrun the local.
                auto copy_size = std::min({static_cast<size_t>(FName::StaticSize()), sizeof(FName), static_cast<size_t>(element_size)});
                FName name{};
                std::memcpy(&name, ptr, copy_size);
                return JsonValue{to_utf8_string(name.ToString())};
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FTextProperty) ||
                     property_class.HasAnyCastFlags(CASTCLASS_FMulticastInlineDelegateProperty) ||
                     property_class.HasAnyCastFlags(CASTCLASS_FMulticastSparseDelegateProperty) ||
                     property_class.HasAnyCastFlags(CASTCLASS_FMulticastDelegateProperty) || property_class.HasAnyCastFlags(CASTCLASS_FDelegateProperty))
            {
                return std::nullopt;
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FBoolProperty))
            {
                auto* bool_property = static_cast<FBoolProperty*>(property);
                uint8_t byte = *(static_cast<uint8_t*>(ptr) + bool_property->GetByteOffset());
                return JsonValue{(byte & bool_property->GetByteMask()) != 0};
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FArrayProperty))
            {
                auto* array_property = static_cast<FArrayProperty*>(property);
                FProperty* inner = array_property->GetInner();
                auto* view = static_cast<const ScriptArrayView*>(ptr);
                JsonArray elements{};
                if (view->ArrayNum < 0 || view->ArrayNum > MaxSaneContainerElements)
                {
                    return std::nullopt;
                }
                if (view->Data && view->ArrayNum > 0)
                {
                    if (!inner || !m_memory.is_readable(view->Data, static_cast<size_t>(view->ArrayNum) * inner->GetElementSize()))
                    {
                        return std::nullopt;
                    }
                    for (int32_t i = 0; i < view->ArrayNum; ++i)
                    {
                        if (auto value = dump_property_value(inner, view->Data, i))
                        {
                            elements.push_back(std::move(*value));
                        }
                        else
                        {
                            return std::nullopt;
                        }
                    }
                }
                return JsonValue{std::move(elements)};
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FEnumProperty))
            {
                auto* enum_property = static_cast<FEnumProperty*>(property);
                FNumericProperty* underlying = enum_property->GetUnderlyingProp();
                UEnum* uenum = enum_property->GetEnum();
                if (!underlying || !uenum)
                {
                    return std::nullopt;
                }
                void* value_ptr = static_cast<uint8_t*>(ptr) + underlying->GetOffset_Internal();
                auto underlying_class = underlying->GetClass();
                int64_t value{};
                if (underlying_class.HasAnyCastFlags(CASTCLASS_FByteProperty))
                {
                    uint8_t raw{};
                    std::memcpy(&raw, value_ptr, sizeof(raw));
                    value = raw;
                }
                else if (underlying_class.HasAnyCastFlags(CASTCLASS_FInt8Property))
                {
                    int8_t raw{};
                    std::memcpy(&raw, value_ptr, sizeof(raw));
                    value = raw;
                }
                else if (underlying_class.HasAnyCastFlags(CASTCLASS_FInt16Property))
                {
                    int16_t raw{};
                    std::memcpy(&raw, value_ptr, sizeof(raw));
                    value = raw;
                }
                else if (underlying_class.HasAnyCastFlags(CASTCLASS_FIntProperty))
                {
                    int32_t raw{};
                    std::memcpy(&raw, value_ptr, sizeof(raw));
                    value = raw;
                }
                else if (underlying_class.HasAnyCastFlags(CASTCLASS_FInt64Property))
                {
                    std::memcpy(&value, value_ptr, sizeof(value));
                }
                else if (underlying_class.HasAnyCastFlags(CASTCLASS_FUInt16Property))
                {
                    uint16_t raw{};
                    std::memcpy(&raw, value_ptr, sizeof(raw));
                    value = raw;
                }
                else if (underlying_class.HasAnyCastFlags(CASTCLASS_FUInt32Property))
                {
                    uint32_t raw{};
                    std::memcpy(&raw, value_ptr, sizeof(raw));
                    value = raw;
                }
                else if (underlying_class.HasAnyCastFlags(CASTCLASS_FUInt64Property))
                {
                    uint64_t raw{};
                    std::memcpy(&raw, value_ptr, sizeof(raw));
                    value = static_cast<int64_t>(raw);
                }
                else
                {
                    return std::nullopt;
                }
                for (const auto& [name, enum_value] : enum_names_of(uenum))
                {
                    if (enum_value == value)
                    {
                        return JsonValue{name};
                    }
                }
                return JsonValue{value};
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FMapProperty))
            {
                auto* map_property = static_cast<FMapProperty*>(property);
                FProperty* key_prop = map_property->GetKeyProp();
                FProperty* value_prop = map_property->GetValueProp();
                if (!key_prop || !value_prop)
                {
                    return std::nullopt;
                }
                auto* map = static_cast<FScriptMap*>(ptr);
                auto layout = ScriptContainerLayout::ComputeMapLayout(key_prop, value_prop);
                auto element_stride = layout.SetLayout.Size;
                int32_t max_index = map->GetMaxIndex();
                if (max_index < 0 || max_index > MaxSaneContainerElements)
                {
                    return std::nullopt;
                }
                // BTreeMap in the reference sorts entries and keeps the last value per key;
                // sort by the compact JSON of the key for a deterministic order
                std::map<std::string, std::pair<JsonValue, JsonValue>> entries{};
                for (int32_t i = 0; i < max_index; ++i)
                {
                    if (!map->IsValidIndex(i))
                    {
                        continue;
                    }
                    void* pair_ptr = map->GetData(i, layout);
                    if (!m_memory.is_readable(pair_ptr, static_cast<size_t>(element_stride)))
                    {
                        continue;
                    }
                    auto key = dump_property_value(key_prop, pair_ptr, 0);
                    auto value = dump_property_value(value_prop, pair_ptr, 0);
                    if (key && value)
                    {
                        entries.insert_or_assign(to_compact_json(*key), std::make_pair(std::move(*key), std::move(*value)));
                    }
                }
                JsonArray pairs{};
                for (auto& [_, entry] : entries)
                {
                    JsonArray pair{};
                    pair.push_back(std::move(entry.first));
                    pair.push_back(std::move(entry.second));
                    pairs.push_back(std::move(pair));
                }
                return JsonValue{std::move(pairs)};
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FSetProperty))
            {
                auto* set_property = static_cast<FSetProperty*>(property);
                FProperty* element_prop = set_property->GetElementProp();
                if (!element_prop)
                {
                    return std::nullopt;
                }
                auto* set = static_cast<FScriptSet*>(ptr);
                auto layout = ScriptContainerLayout::ComputeSetLayout(element_prop);
                auto element_stride = layout.Size;
                int32_t max_index = set->GetMaxIndex();
                if (max_index < 0 || max_index > MaxSaneContainerElements)
                {
                    return std::nullopt;
                }
                std::map<std::string, JsonValue> elements{};
                for (int32_t i = 0; i < max_index; ++i)
                {
                    if (!set->IsValidIndex(i))
                    {
                        continue;
                    }
                    void* element_ptr = set->GetData(i, layout);
                    if (!m_memory.is_readable(element_ptr, static_cast<size_t>(element_stride)))
                    {
                        continue;
                    }
                    if (auto value = dump_property_value(element_prop, element_ptr, 0))
                    {
                        elements.insert_or_assign(to_compact_json(*value), std::move(*value));
                    }
                }
                JsonArray values{};
                for (auto& [_, element] : elements)
                {
                    values.push_back(std::move(element));
                }
                return JsonValue{std::move(values)};
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FFloatProperty))
            {
                float raw{};
                std::memcpy(&raw, ptr, sizeof(raw));
                return JsonValue{raw};
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FDoubleProperty))
            {
                double raw{};
                std::memcpy(&raw, ptr, sizeof(raw));
                return JsonValue{raw};
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FByteProperty))
            {
                auto* byte_property = static_cast<FByteProperty*>(property);
                uint8_t raw{};
                std::memcpy(&raw, ptr, sizeof(raw));
                if (UEnum* uenum = byte_property->GetEnum())
                {
                    for (const auto& [name, enum_value] : enum_names_of(uenum))
                    {
                        if (enum_value == static_cast<int64_t>(raw))
                        {
                            return JsonValue{name};
                        }
                    }
                }
                return JsonValue{static_cast<int64_t>(raw)};
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FUInt16Property))
            {
                uint16_t raw{};
                std::memcpy(&raw, ptr, sizeof(raw));
                return JsonValue{static_cast<uint64_t>(raw)};
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FUInt32Property))
            {
                uint32_t raw{};
                std::memcpy(&raw, ptr, sizeof(raw));
                return JsonValue{static_cast<uint64_t>(raw)};
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FUInt64Property))
            {
                uint64_t raw{};
                std::memcpy(&raw, ptr, sizeof(raw));
                return JsonValue{raw};
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FInt8Property))
            {
                return JsonValue{read_signed(int8_t{})};
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FInt16Property))
            {
                return JsonValue{read_signed(int16_t{})};
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FIntProperty))
            {
                return JsonValue{read_signed(int32_t{})};
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FInt64Property))
            {
                return JsonValue{read_signed(int64_t{})};
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FObjectProperty))
            {
                UObject* target{};
                std::memcpy(&target, ptr, sizeof(target));
                if (!target)
                {
                    return JsonValue{nullptr};
                }
                if (!is_live_object(target))
                {
                    return std::nullopt;
                }
                return JsonValue{path_of(target)};
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FWeakObjectProperty) || property_class.HasAnyCastFlags(CASTCLASS_FSoftObjectProperty) ||
                     property_class.HasAnyCastFlags(CASTCLASS_FLazyObjectProperty) || property_class.HasAnyCastFlags(CASTCLASS_FInterfaceProperty) ||
                     property_class.HasAnyCastFlags(CASTCLASS_FFieldPathProperty) || property_class.HasAnyCastFlags(CASTCLASS_FOptionalProperty))
            {
                return std::nullopt;
            }
            else if (property_class.HasAnyCastFlags(CASTCLASS_FUtf8StrProperty) || property_class.HasAnyCastFlags(CASTCLASS_FAnsiStrProperty))
            {
                auto* view = static_cast<const ScriptArrayView*>(ptr);
                if (!view->Data || view->ArrayNum <= 0)
                {
                    return JsonValue{std::string{}};
                }
                if (view->ArrayNum > MaxSaneContainerElements || !m_memory.is_readable(view->Data, static_cast<size_t>(view->ArrayNum)))
                {
                    return std::nullopt;
                }
                return JsonValue{std::string{static_cast<const char*>(view->Data), static_cast<size_t>(view->ArrayNum - 1)}};
            }

            return std::nullopt;
        }

        // ============================================================================
        // VTable analysis (mirrors jmap_dumper/src/vtable.rs)
        // ============================================================================
        auto JMapDumper::analyze_vtables() -> std::map<uint64_t, std::vector<uint64_t>>
        {
            constexpr size_t max_vtable_entries = 4096;

            // First instance's vtable per class, and the set of distinct vtable addresses
            std::unordered_map<std::string, uint64_t> class_vtables{};
            std::set<uint64_t> vtable_addresses{};
            for (const auto& [path, entry] : m_objects)
            {
                auto it = class_vtables.find(entry.class_path);
                if (it == class_vtables.end())
                {
                    class_vtables.emplace(entry.class_path, entry.vtable);
                }
                vtable_addresses.insert(entry.vtable);
            }

            // Walk each vtable, bounded by the next known vtable start
            std::map<uint64_t, std::vector<uint64_t>> vtables{};
            for (auto it = vtable_addresses.begin(); it != vtable_addresses.end(); ++it)
            {
                uint64_t vtable = *it;
                auto next_it = std::next(it);
                std::optional<uint64_t> bound = next_it != vtable_addresses.end() ? std::optional{*next_it} : std::nullopt;

                std::vector<uint64_t> functions{};
                uint64_t address = vtable;
                while (!(bound && address >= *bound) && functions.size() < max_vtable_entries)
                {
                    if (!m_memory.is_readable(reinterpret_cast<const void*>(address), sizeof(uint64_t)))
                    {
                        break;
                    }
                    uint64_t function{};
                    std::memcpy(&function, reinterpret_cast<const void*>(address), sizeof(function));
                    if (!m_memory.is_executable(reinterpret_cast<const void*>(function)))
                    {
                        break;
                    }
                    functions.push_back(function);
                    address += sizeof(uint64_t);
                }
                vtables.emplace(vtable, std::move(functions));
            }

            // Walked vtables overshoot; a parent's true vtable can never be longer than any
            // child's, so propagate child lengths up each super chain
            for (const auto& [path, entry] : m_objects)
            {
                if (!entry.is_class)
                {
                    continue;
                }
                auto vtable_it = class_vtables.find(path);
                if (vtable_it == class_vtables.end())
                {
                    continue;
                }
                size_t vtable_len = vtables.at(vtable_it->second).size();

                const DumpedObject* current = &entry;
                while (current->super_struct)
                {
                    auto parent_it = m_objects.find(*current->super_struct);
                    if (parent_it == m_objects.end())
                    {
                        break;
                    }
                    current = &parent_it->second;
                    auto parent_vtable_it = class_vtables.find(parent_it->first);
                    if (parent_vtable_it == class_vtables.end())
                    {
                        continue;
                    }
                    auto& parent_functions = vtables.at(parent_vtable_it->second);
                    if (parent_functions.size() > vtable_len)
                    {
                        parent_functions.resize(vtable_len);
                    }
                    vtable_len = parent_functions.size();
                }
            }

            // Record the observed instance vtable on each dumped class
            for (const auto& [class_path, vtable] : class_vtables)
            {
                auto it = m_objects.find(class_path);
                if (it != m_objects.end() && it->second.is_class)
                {
                    if (auto* object_json = std::get_if<JsonObject>(&it->second.json.value))
                    {
                        if (auto* instance_vtable = object_json->find("instance_vtable"))
                        {
                            *instance_vtable = JsonValue{hex_address(vtable)};
                        }
                    }
                }
            }

            return vtables;
        }

        // ============================================================================
        // Metadata and output
        // ============================================================================
        auto sanitize_for_filename(std::string str) -> std::string
        {
            for (char& c : str)
            {
                if (c == ' ' || c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|')
                {
                    c = '_';
                }
            }
            return str;
        }

        auto iso8601_utc_now() -> std::string
        {
            std::time_t now = std::time(nullptr);
            std::tm utc{};
            gmtime_s(&utc, &now);
            char buf[32];
            std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &utc);
            return buf;
        }

        // Extracts the changelist from engine version strings like "5.1.1-23058290+++UE5+Release-5.1"
        auto parse_build_change_list(const std::string& engine_version) -> std::optional<std::string>
        {
            auto dash = engine_version.find('-');
            if (dash == std::string::npos)
            {
                return std::nullopt;
            }
            auto plus = engine_version.find('+', dash + 1);
            std::string change_list = engine_version.substr(dash + 1, plus == std::string::npos ? std::string::npos : plus - dash - 1);
            if (change_list.empty() || !std::all_of(change_list.begin(), change_list.end(), [](char c) {
                    return c >= '0' && c <= '9';
                }))
            {
                return std::nullopt;
            }
            return change_list;
        }

        // Engine globals and hot functions resolved by UE4SS at startup. These cannot be recovered from the
        // reflection data itself, so consumers that want to attach to (or disassemble) the image -- SDK
        // generators producing a standalone backend, disassembler importers naming GObjects/ProcessEvent --
        // would otherwise have to re-scan for them. Addresses are absolute; subtract image_base_address for RVAs.
        auto build_engine_offsets() -> JsonObject
        {
            JsonObject offsets{};
            auto add_offset = [&offsets](const char* name, void* address) {
                if (address)
                {
                    offsets.add(name, hex_address(reinterpret_cast<uint64_t>(address)));
                }
            };

            add_offset("guobject_array", UObjectArray::GetGUObjectArrayAddress());
            add_offset("gmalloc", GMalloc);
            add_offset("gnatives", GNatives_Internal);
            add_offset("process_event", UObject::ProcessEventInternal.get_function_address());
            add_offset("process_internal", UObject::ProcessInternalInternal.get_function_address());
            add_offset("fname_to_string", FName::ToStringInternal.get_function_address());

            return offsets;
        }

        auto JMapDumper::write_output(const std::map<uint64_t, std::vector<uint64_t>>& vtables) -> void
        {
            std::string engine_version = to_utf8_string(*UKismetSystemLibrary::GetEngineVersion());
            std::string game_name = to_utf8_string(*UKismetSystemLibrary::GetGameName());
            auto build_change_list = parse_build_change_list(engine_version);

            wchar_t module_path[MAX_PATH]{};
            GetModuleFileNameW(nullptr, module_path, MAX_PATH);
            std::wstring module_path_wide{module_path};
            auto separator = module_path_wide.find_last_of(L"/\\");
            std::string source = to_utf8_string(separator == std::wstring::npos ? module_path_wide : module_path_wide.substr(separator + 1));

            JsonObject metadata{};
            metadata.add("tool", "UE4SS JMapGenerator (https://github.com/UE4SS-RE/RE-UE4SS)");
            metadata.add("timestamp", iso8601_utc_now());
            metadata.add("source", source);
            JsonObject engine_version_json{};
            engine_version_json.add("major", static_cast<int64_t>(Version::Major));
            engine_version_json.add("minor", static_cast<int64_t>(Version::Minor));
            metadata.add("engine_version", std::move(engine_version_json));
            if (build_change_list)
            {
                metadata.add("build_change_list", *build_change_list);
            }
            else
            {
                metadata.add("build_change_list", nullptr);
            }

            std::string filename = sanitize_for_filename(game_name) + "-" + sanitize_for_filename(engine_version) + "-" + UE4SS_LIB_BUILD_GITSHA + ".jmap";
            std::filesystem::path output_path = std::filesystem::path{UE4SSProgram::get_program().get_working_directory()} / filename;

            std::ofstream file{output_path, std::ios::binary};
            if (!file)
            {
                Output::send<LogLevel::Error>(STR("[JMapGenerator] Failed to open output file: {}\n"), output_path.wstring());
                return;
            }

            StreamSink sink{file};
            sink.append("{\n  \"metadata\": ");
            write_json(sink, JsonValue{std::move(metadata)}, 1);
            sink.append(",\n  \"image_base_address\": ");
            json_escape_to(sink, hex_address(reinterpret_cast<uint64_t>(GetModuleHandleW(nullptr))));
            if (auto engine_offsets = build_engine_offsets(); !engine_offsets.members.empty())
            {
                sink.append(",\n  \"engine_offsets\": ");
                write_json(sink, JsonValue{std::move(engine_offsets)}, 1);
            }
            sink.append(",\n  \"objects\": ");
            if (m_objects.empty())
            {
                sink.append("{}");
            }
            else
            {
                sink.append("{");
                bool first = true;
                for (const auto& [path, entry] : m_objects)
                {
                    if (!first)
                    {
                        sink.append(",");
                    }
                    first = false;
                    sink.append("\n    ");
                    json_escape_to(sink, path);
                    sink.append(": ");
                    write_json(sink, entry.json, 2);
                }
                sink.append("\n  }");
            }
            sink.append(",\n  \"vtables\": ");
            if (vtables.empty())
            {
                sink.append("{}");
            }
            else
            {
                sink.append("{");
                bool first = true;
                for (const auto& [vtable, functions] : vtables)
                {
                    if (!first)
                    {
                        sink.append(",");
                    }
                    first = false;
                    sink.append("\n    ");
                    json_escape_to(sink, hex_address(vtable));
                    sink.append(": ");
                    JsonArray function_array{};
                    function_array.reserve(functions.size());
                    for (uint64_t function : functions)
                    {
                        function_array.push_back(hex_address(function));
                    }
                    write_json(sink, JsonValue{std::move(function_array)}, 2);
                }
                sink.append("\n  }");
            }
            sink.append("\n}");
            file.flush();

            Output::send(STR("[JMapGenerator] Dump complete: {} objects, {} vtables, {} warnings\n"), m_objects.size(), vtables.size(), m_warning_count);
            Output::send(STR("[JMapGenerator] Output file: {}\n"), output_path.wstring());
        }

        // FName's alignment is fixed at compile time (FNAME_ALIGN8), while the engine's real layout is
        // read from reflection at startup. A UE4SS build that disagrees with the running engine -- a
        // default build against UE 4.21 or below, which needs the LessEqual421 target -- mis-strides
        // every container holding names. Property values then read back as garbage, and FName::ToString()
        // indexes the engine's name pool with a bogus index, faulting inside the game.
        auto JMapDumper::engine_fname_layout_matches() -> bool
        {
            int32_t engine_size{};
            int32_t engine_alignment{};
            try
            {
                engine_size = FName::StaticSize();
                engine_alignment = FName::StaticAlignment();
            }
            catch (std::exception& e)
            {
                warn(std::string{"could not determine the engine's FName layout: "} + e.what());
                return false;
            }

            if (engine_size == static_cast<int32_t>(sizeof(FName)) && engine_alignment == static_cast<int32_t>(alignof(FName)))
            {
                return true;
            }

            Output::send<LogLevel::Warning>(
                    STR("[JMapGenerator] FName layout mismatch: engine reports size 0x{:X} align 0x{:X}, this UE4SS build assumes size 0x{:X} align 0x{:X}\n"),
                    engine_size,
                    engine_alignment,
                    static_cast<int32_t>(sizeof(FName)),
                    static_cast<int32_t>(alignof(FName)));
            Output::send<LogLevel::Warning>(
                    STR("[JMapGenerator] Use a UE4SS build matching this engine version (the LessEqual421 target for UE 4.21 and below).\n"));
            return false;
        }

        auto JMapDumper::dump() -> void
        {
            Output::send(STR("[JMapGenerator] Dumping reflection data to .jmap (format by trumank, https://github.com/trumank/jmap)\n"));
            if (m_include_blueprint_types)
            {
                Output::send(STR("[JMapGenerator] Including Blueprint-generated types\n"));
            }
            // Container traversal here derives its strides from reflection, so a mismatch no longer
            // corrupts the dump. It still means this UE4SS build disagrees with the running engine
            // elsewhere, so surface it rather than letting it be diagnosed from odd values later.
            if (!m_skip_property_values)
            {
                engine_fname_layout_matches();
            }

            // Main pass over GUObjectArray
            UObjectGlobals::ForEachUObject([&](UObject* object, [[maybe_unused]] int32 object_index, [[maybe_unused]] int32 chunk_index) {
                try
                {
                    if (!object || *reinterpret_cast<uint64_t*>(object) == 0 || !object->GetClassPrivate())
                    {
                        return LoopAction::Continue;
                    }
                    const std::string& path = path_of(object);
                    if (!should_dump(object, path))
                    {
                        return LoopAction::Continue;
                    }
                    m_scheduled.insert(object);
                    if (auto entry = dump_object_entry(object, path))
                    {
                        if (m_include_blueprint_types && !path.starts_with("/Script/"))
                        {
                            schedule_extras(object);
                        }
                        insert_object(path, std::move(*entry));
                    }
                }
                catch (std::exception& e)
                {
                    warn(std::string{"failed to dump object: "} + e.what());
                }
                catch (...)
                {
                    warn("failed to dump object: unknown error");
                }
                return LoopAction::Continue;
            });

            // Closure pass (blueprint mode): outer packages and class default objects
            for (size_t i = 0; i < m_extra_objects.size(); ++i)
            {
                UObject* object = m_extra_objects[i];
                try
                {
                    if (!object || *reinterpret_cast<uint64_t*>(object) == 0 || !object->GetClassPrivate())
                    {
                        continue;
                    }
                    const std::string& path = path_of(object);
                    if (m_objects.contains(path))
                    {
                        continue;
                    }
                    if (auto entry = dump_object_entry(object, path))
                    {
                        schedule_extras(object);
                        insert_object(path, std::move(*entry));
                    }
                }
                catch (std::exception& e)
                {
                    warn(std::string{"failed to dump object: "} + e.what());
                }
                catch (...)
                {
                    warn("failed to dump object: unknown error");
                }
            }

            Output::send(STR("[JMapGenerator] Dumped {} objects, analyzing children and vtables...\n"), m_objects.size());

            // Fill in children sets from outer links
            std::map<std::string, std::set<std::string>> child_map{};
            for (const auto& [path, entry] : m_objects)
            {
                if (entry.outer)
                {
                    child_map[*entry.outer].insert(path);
                }
            }
            for (auto& [outer_path, children] : child_map)
            {
                auto it = m_objects.find(outer_path);
                if (it == m_objects.end())
                {
                    continue;
                }
                if (auto* object_json = std::get_if<JsonObject>(&it->second.json.value))
                {
                    if (auto* children_json = object_json->find("children"))
                    {
                        JsonArray children_array{};
                        children_array.reserve(children.size());
                        for (const auto& child : children)
                        {
                            children_array.push_back(child);
                        }
                        *children_json = JsonValue{std::move(children_array)};
                    }
                }
            }

            auto vtables = analyze_vtables();
            write_output(vtables);
        }
    } // namespace

    auto generate_jmap(bool include_blueprint_types, bool skip_property_values) -> void
    {
        JMapDumper dumper{include_blueprint_types, skip_property_values};
        dumper.dump();
    }
} // namespace RC::JMapGenerator
