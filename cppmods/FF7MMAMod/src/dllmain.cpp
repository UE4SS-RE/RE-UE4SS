#include <FF7MMAMod.hpp>
#include <MMA/Writer.hpp>
#include <GUI/GUITab.hpp>
#include <UE4SSProgram.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/UClass.hpp>
#include <DynamicOutput/Output.hpp>
#include <Helpers/String.hpp>

#include <glaze/glaze.hpp>

#include <Unreal/Hooks/Hooks.hpp>
#include <Unreal/UFunction.hpp>
#include <Unreal/FFrame.hpp>
#include <Unreal/FProperty.hpp>

namespace RC::FF7MMA { using ModJson = glz::generic_json<glz::num_mode::i64>; }

#include <imgui.h>
#define NOMINMAX
#include <Windows.h>
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <ctime>
#include <fstream>
#include <sstream>

namespace RC::FF7MMA {

    FF7MMAMod* FF7MMAMod::s_instance = nullptr;
    Unreal::UClass* FF7MMAMod::s_mmaBaseClass = nullptr;
    Unreal::UClass* FF7MMAMod::s_dataObjectBaseClass = nullptr;
    bool FF7MMAMod::s_mmaClassSearched = false;

    namespace {
        // Unreflected UMemoryMappedAsset fields live after UObject (0x28); the exact
        // offset of the layout pointer is located at runtime (s_layoutFieldOffset).
        size_t s_layoutFieldOffset = 0;

        bool IsReadable(const void* ptr, size_t bytes) {
            if (!ptr) return false;
            MEMORY_BASIC_INFORMATION mbi{};
            if (VirtualQuery(ptr, &mbi, sizeof(mbi)) == 0) return false;
            if (mbi.State != MEM_COMMIT) return false;
            if (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) return false;
            uintptr_t regionEnd = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
            return reinterpret_cast<uintptr_t>(ptr) + bytes <= regionEnd;
        }

        bool IsPlausibleWideName(const wchar_t* name) {
            if (!IsReadable(name, 2 * 128)) return false;
            for (int i = 0; i < 128; ++i) {
                wchar_t c = name[i];
                if (c == 0) return i >= 2;
                if (c < 0x20 || c > 0x7E) return false;
            }
            return false;
        }

        bool IsPlausibleLayout(const Unreal::FTypeLayoutDesc* layout) {
            if (!IsReadable(layout, sizeof(Unreal::FTypeLayoutDesc))) return false;
            if (!layout->Name || !IsPlausibleWideName(layout->Name)) return false;
            if (layout->Size == 0 || layout->Size > 0x10000) return false;
            uint32_t align = layout->Alignment;
            if (align == 0 || align > 16 || (align & (align - 1)) != 0) return false;
            return true;
        }

        // Number of contiguous readable bytes starting at ptr, capped
        size_t ContiguousReadableBytes(const void* ptr, size_t cap) {
            uintptr_t address = reinterpret_cast<uintptr_t>(ptr);
            uintptr_t end = address;
            while (end - address < cap) {
                MEMORY_BASIC_INFORMATION mbi{};
                if (VirtualQuery(reinterpret_cast<const void*>(end), &mbi, sizeof(mbi)) == 0) break;
                if (mbi.State != MEM_COMMIT) break;
                if (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) break;
                end = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
            }
            return end > address ? std::min<size_t>(end - address, cap) : 0;
        }

        // Layout ptr(8) + flags(4) + FrozenContentSize(4) + Content ptr(8).
        // The stored size is often 0 at runtime; fall back to the readable region
        // extent, which only serves as a bounds guard for the parser.
        bool TryReadFieldsAt(uint8_t* base, size_t offset, Unreal::FTypeLayoutDesc*& layout,
                             uint32_t& someBools, uint32_t& contentSize, uint8_t*& content) {
            auto* candidate = *reinterpret_cast<Unreal::FTypeLayoutDesc**>(base + offset);
            if (!IsPlausibleLayout(candidate)) return false;
            uint32_t size = *reinterpret_cast<uint32_t*>(base + offset + 12);
            uint8_t* data = *reinterpret_cast<uint8_t**>(base + offset + 16);
            if (!IsReadable(data, 64)) return false;
            constexpr size_t SizeCap = 0x40000000;
            size_t available = ContiguousReadableBytes(data, SizeCap);
            if (available < 64) return false;
            if (size == 0 || size > available) {
                size = static_cast<uint32_t>(std::min(available, SizeCap));
            }
            layout = candidate;
            someBools = *reinterpret_cast<uint32_t*>(base + offset + 8);
            contentSize = size;
            content = data;
            return true;
        }

        // case-insensitive glob with '*' wildcards
        bool WildcardMatch(const std::string& pattern, const std::string& text) {
            auto low = [](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); };
            size_t p = 0, t = 0, star = std::string::npos, mark = 0;
            while (t < text.size()) {
                if (p < pattern.size() && pattern[p] != '*' && low(pattern[p]) == low(text[t])) {
                    ++p; ++t;
                } else if (p < pattern.size() && pattern[p] == '*') {
                    star = p++;
                    mark = t;
                } else if (star != std::string::npos) {
                    p = star + 1;
                    t = ++mark;
                } else {
                    return false;
                }
            }
            while (p < pattern.size() && pattern[p] == '*') ++p;
            return p == pattern.size();
        }

        // Accepts a plain object name or a full object path and yields the
        // lowercase short name ("/Game/X/CharaSpec.CharaSpec" -> "charaspec")
        std::string NormalizeTableKey(const std::string& name);

        std::string ToLower(std::string s) {
            std::transform(s.begin(), s.end(), s.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return s;
        }

        std::string NormalizeTableKey(const std::string& name) {
            size_t start = name.find_last_of("/.");
            std::string shortName = start == std::string::npos ? name : name.substr(start + 1);
            return ToLower(shortName);
        }

        std::string SanitizeFileName(const std::string& name) {
            std::string out;
            for (char c : name) {
                if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-' || c == '.') {
                    out += c;
                } else {
                    out += '_';
                }
            }
            return out.empty() ? "unnamed" : out;
        }

        std::filesystem::path GetExeDirectory() {
            wchar_t buffer[MAX_PATH];
            GetModuleFileNameW(nullptr, buffer, MAX_PATH);
            return std::filesystem::path(buffer).parent_path();
        }

        // ---- JSON-driven table mods (PalSchema-style) ----

        std::vector<PropertyValue> CloneRowValues(const std::vector<PropertyValue>& values);

        std::map<std::string, ModJson::object_t> s_tableMods; // lowercase table name -> row objects
        std::map<std::string, std::string> s_tableModNames;    // lowercase -> name as written in the file
        std::set<std::string> s_appliedTables;
        std::map<std::string, std::string> s_modResults;
        std::vector<uint8_t*> s_blobs; // side allocations and rebuilt images stay alive forever
        std::map<Unreal::UObject*, std::array<uint8_t, 88>> s_savedRoots; // pre-redirect root header bytes
        std::map<Unreal::UObject*, std::pair<uint8_t*, size_t>> s_liveBase; // rebuilt image + size, once roots were redirected

        // The blob the game reads (the original, or the rebuilt image after a root redirect)
        uint8_t* LiveBaseFor(Unreal::UObject* obj, uint8_t* contentFromFields) {
            auto it = s_liveBase.find(obj);
            return it != s_liveBase.end() ? it->second.first : contentFromFields;
        }

        // Parse source for a table: the rebuilt image if the roots were redirected
        // (the original blob's root pointers then target memory outside itself)
        void GetParseSource(Unreal::UObject* obj, uint8_t*& content, size_t& size) {
            auto it = s_liveBase.find(obj);
            if (it != s_liveBase.end()) {
                content = it->second.first;
                size = it->second.second;
            }
        }

        bool EnsureWritable(void* ptr, size_t bytes) {
            uint8_t* p = static_cast<uint8_t*>(ptr);
            uint8_t* end = p + bytes;
            while (p < end) {
                MEMORY_BASIC_INFORMATION mbi{};
                if (VirtualQuery(p, &mbi, sizeof(mbi)) == 0) return false;
                DWORD prot = mbi.Protect & 0xFF;
                bool writable = prot == PAGE_READWRITE || prot == PAGE_WRITECOPY ||
                                prot == PAGE_EXECUTE_READWRITE || prot == PAGE_EXECUTE_WRITECOPY;
                uint8_t* regionEnd = static_cast<uint8_t*>(mbi.BaseAddress) + mbi.RegionSize;
                if (!writable) {
                    size_t span = static_cast<size_t>((regionEnd < end ? regionEnd : end) - p);
                    DWORD oldProt;
                    if (!VirtualProtect(p, span, PAGE_READWRITE, &oldProt) &&
                        !VirtualProtect(p, span, PAGE_WRITECOPY, &oldProt)) {
                        return false;
                    }
                }
                p = regionEnd;
            }
            return true;
        }

        // Frozen self-relative pointer at fieldAddr pointing at target (any address)
        void WritePackedPtrAt(uint8_t* fieldAddr, const void* target) {
            int64_t rel = static_cast<int64_t>(reinterpret_cast<uint64_t>(target) -
                                               reinterpret_cast<uint64_t>(fieldAddr));
            uint64_t packed = (static_cast<uint64_t>(rel) << 1) | 1;
            std::memcpy(fieldAddr, &packed, 8);
        }

        size_t ScalarByteSize(EFF7PropertyType type) {
            switch (type) {
                case EFF7PropertyType::BoolProperty:
                case EFF7PropertyType::ByteProperty:
                case EFF7PropertyType::Int8Property: return 1;
                case EFF7PropertyType::UInt16Property:
                case EFF7PropertyType::Int16Property: return 2;
                case EFF7PropertyType::Int64Property:
                case EFF7PropertyType::NameProperty: return 8;
                case EFF7PropertyType::StrProperty: return 16;
                default: return 4;
            }
        }

        bool WriteScalarBytes(uint8_t* target, EFF7PropertyType type, const PropertyValue& value) {
            switch (type) {
                case EFF7PropertyType::BoolProperty: {
                    auto* v = std::get_if<bool>(&value);
                    if (!v) return false;
                    *target = *v ? 1 : 0;
                    return true;
                }
                case EFF7PropertyType::ByteProperty: {
                    auto* v = std::get_if<uint8_t>(&value);
                    if (!v) return false;
                    std::memcpy(target, v, 1);
                    return true;
                }
                case EFF7PropertyType::Int8Property: {
                    auto* v = std::get_if<int8_t>(&value);
                    if (!v) return false;
                    std::memcpy(target, v, 1);
                    return true;
                }
                case EFF7PropertyType::UInt16Property: {
                    auto* v = std::get_if<uint16_t>(&value);
                    if (!v) return false;
                    std::memcpy(target, v, 2);
                    return true;
                }
                case EFF7PropertyType::Int16Property: {
                    auto* v = std::get_if<int16_t>(&value);
                    if (!v) return false;
                    std::memcpy(target, v, 2);
                    return true;
                }
                case EFF7PropertyType::UIntProperty: {
                    auto* v = std::get_if<uint32_t>(&value);
                    if (!v) return false;
                    std::memcpy(target, v, 4);
                    return true;
                }
                case EFF7PropertyType::IntProperty: {
                    auto* v = std::get_if<int32_t>(&value);
                    if (!v) return false;
                    std::memcpy(target, v, 4);
                    return true;
                }
                case EFF7PropertyType::Int64Property: {
                    auto* v = std::get_if<int64_t>(&value);
                    if (!v) return false;
                    std::memcpy(target, v, 8);
                    return true;
                }
                case EFF7PropertyType::FloatProperty: {
                    auto* v = std::get_if<float>(&value);
                    if (!v) return false;
                    std::memcpy(target, v, 4);
                    return true;
                }
                case EFF7PropertyType::NameProperty: {
                    auto* v = std::get_if<FName>(&value);
                    if (!v) return false;
                    uint32_t idx = v->GetComparisonIndex().ToUnstableInt();
                    uint32_t number = static_cast<uint32_t>(v->GetNumber());
                    std::memcpy(target, &idx, 4);
                    std::memcpy(target + 4, &number, 4);
                    return true;
                }
                default:
                    return false;
            }
        }

        // Element block for an array property, sized to at least `capacity` elements
        // so mixed old/new header reads never run past the allocation
        uint8_t* BuildArraySideBlock(EFF7PropertyType type, const std::vector<PropertyValue>& elems,
                                     size_t capacity) {
            size_t n = elems.size();
            if (capacity < n) capacity = n;
            uint8_t* base = nullptr;
            if (type == EFF7PropertyType::StrProperty) {
                size_t headerBytes = capacity * 16;
                size_t charBytes = 0;
                for (const auto& e : elems) {
                    if (auto* str = std::get_if<StringType>(&e)) {
                        if (!str->empty()) charBytes += (str->size() + 1) * 2;
                    }
                }
                size_t total = headerBytes + charBytes;
                base = static_cast<uint8_t*>(::operator new(total));
                std::memset(base, 0, total);
                size_t charPos = headerBytes;
                for (size_t i = 0; i < n; ++i) {
                    uint8_t* header = base + i * 16;
                    auto* str = std::get_if<StringType>(&elems[i]);
                    if (!str || str->empty()) {
                        uint64_t packed = 1;
                        std::memcpy(header, &packed, 8);
                        continue;
                    }
                    int32_t num = static_cast<int32_t>(str->size()) + 1;
                    WritePackedPtrAt(header, base + charPos);
                    std::memcpy(header + 8, &num, 4);
                    std::memcpy(header + 12, &num, 4);
                    std::memcpy(base + charPos, str->data(), str->size() * 2);
                    charPos += static_cast<size_t>(num) * 2;
                }
            } else {
                size_t stride = ScalarByteSize(type);
                size_t total = capacity * stride;
                base = static_cast<uint8_t*>(::operator new(total));
                std::memset(base, 0, total);
                for (size_t i = 0; i < n; ++i) {
                    WriteScalarBytes(base + i * stride, type, elems[i]);
                }
            }
            s_blobs.push_back(base);
            return base;
        }

        bool GetJsonInt(const ModJson& v, int64_t& out) {
            if (v.holds<int64_t>()) { out = v.get<int64_t>(); return true; }
            if (v.holds<double>()) { out = static_cast<int64_t>(std::llround(v.get<double>())); return true; }
            if (v.holds<bool>()) { out = v.get<bool>() ? 1 : 0; return true; }
            return false;
        }

        bool GetJsonDouble(const ModJson& v, double& out) {
            if (v.holds<double>()) { out = v.get<double>(); return true; }
            if (v.holds<int64_t>()) { out = static_cast<double>(v.get<int64_t>()); return true; }
            return false;
        }

        bool ConvertJsonScalar(const ModJson& v, EFF7PropertyType type, PropertyValue& out, std::string& err) {
            int64_t i;
            double d;
            switch (type) {
                case EFF7PropertyType::BoolProperty:
                    if (v.holds<bool>()) { out = v.get<bool>(); return true; }
                    if (GetJsonInt(v, i)) { out = (i != 0); return true; }
                    err = "expected bool";
                    return false;
                case EFF7PropertyType::ByteProperty:
                    if (GetJsonInt(v, i)) { out = static_cast<uint8_t>(i); return true; }
                    err = "expected number"; return false;
                case EFF7PropertyType::Int8Property:
                    if (GetJsonInt(v, i)) { out = static_cast<int8_t>(i); return true; }
                    err = "expected number"; return false;
                case EFF7PropertyType::UInt16Property:
                    if (GetJsonInt(v, i)) { out = static_cast<uint16_t>(i); return true; }
                    err = "expected number"; return false;
                case EFF7PropertyType::Int16Property:
                    if (GetJsonInt(v, i)) { out = static_cast<int16_t>(i); return true; }
                    err = "expected number"; return false;
                case EFF7PropertyType::UIntProperty:
                    if (GetJsonInt(v, i)) { out = static_cast<uint32_t>(i); return true; }
                    err = "expected number"; return false;
                case EFF7PropertyType::IntProperty:
                    if (GetJsonInt(v, i)) { out = static_cast<int32_t>(i); return true; }
                    err = "expected number"; return false;
                case EFF7PropertyType::Int64Property:
                    if (GetJsonInt(v, i)) { out = i; return true; }
                    err = "expected number"; return false;
                case EFF7PropertyType::FloatProperty:
                    if (GetJsonDouble(v, d)) { out = static_cast<float>(d); return true; }
                    err = "expected number"; return false;
                case EFF7PropertyType::StrProperty:
                    if (v.holds<std::string>()) { out = to_generic_string(v.get<std::string>()); return true; }
                    if (v.is_null()) { out = StringType(); return true; }
                    err = "expected string"; return false;
                case EFF7PropertyType::NameProperty:
                    if (v.holds<std::string>()) {
                        out = FName(to_generic_string(v.get<std::string>()), Unreal::FNAME_Add);
                        return true;
                    }
                    if (v.is_null()) { out = FName(); return true; }
                    err = "expected string"; return false;
                default:
                    err = "unsupported type";
                    return false;
            }
        }

        PropertyValue DefaultValue(const FFF7Property& prop) {
            if (prop.bIsArray) {
                auto arr = std::make_shared<PropertyArray>();
                arr->ElementType = prop.Type;
                return arr;
            }
            switch (prop.Type) {
                case EFF7PropertyType::BoolProperty: return false;
                case EFF7PropertyType::ByteProperty: return static_cast<uint8_t>(0);
                case EFF7PropertyType::Int8Property: return static_cast<int8_t>(0);
                case EFF7PropertyType::UInt16Property: return static_cast<uint16_t>(0);
                case EFF7PropertyType::Int16Property: return static_cast<int16_t>(0);
                case EFF7PropertyType::UIntProperty: return static_cast<uint32_t>(0);
                case EFF7PropertyType::IntProperty: return static_cast<int32_t>(0);
                case EFF7PropertyType::Int64Property: return static_cast<int64_t>(0);
                case EFF7PropertyType::FloatProperty: return 0.0f;
                case EFF7PropertyType::StrProperty: return StringType();
                case EFF7PropertyType::NameProperty: return FName();
                default: return static_cast<int32_t>(0);
            }
        }

        // Applies "Prop": value pairs from a JSON row object; "$"-prefixed keys are directives
        bool ApplyRowProps(MMARow& row, const std::vector<FFF7Property>& schema,
                           const ModJson::object_t& obj, std::vector<std::string>& errs,
                           std::vector<int32_t>* outChanged = nullptr) {
            bool anyError = false;
            for (const auto& [propName, propVal] : obj) {
                if (!propName.empty() && propName[0] == '$') continue;
                int32_t idx = -1;
                for (size_t i = 0; i < schema.size(); ++i) {
                    if (to_utf8_string(schema[i].Name.ToString()) == propName) {
                        idx = static_cast<int32_t>(i);
                        break;
                    }
                }
                if (idx < 0) {
                    errs.push_back("unknown property '" + propName + "'");
                    anyError = true;
                    continue;
                }
                const auto& prop = schema[idx];
                std::string err;
                if (prop.bIsArray) {
                    if (!propVal.is_array()) {
                        errs.push_back("property '" + propName + "' expects an array");
                        anyError = true;
                        continue;
                    }
                    auto arr = std::make_shared<PropertyArray>();
                    arr->ElementType = prop.Type;
                    bool ok = true;
                    for (const auto& elem : propVal.get_array()) {
                        PropertyValue ev;
                        if (!ConvertJsonScalar(elem, prop.Type, ev, err)) {
                            errs.push_back("property '" + propName + "' element: " + err);
                            ok = false;
                            anyError = true;
                            break;
                        }
                        arr->Elements.push_back(std::move(ev));
                    }
                    if (ok) {
                        row.Values[idx] = arr;
                        if (outChanged) outChanged->push_back(idx);
                    }
                } else {
                    PropertyValue value;
                    if (ConvertJsonScalar(propVal, prop.Type, value, err)) {
                        row.Values[idx] = std::move(value);
                        if (outChanged) outChanged->push_back(idx);
                    } else {
                        errs.push_back("property '" + propName + "': " + err);
                        anyError = true;
                    }
                }
            }
            return !anyError;
        }

        std::string EscapeJson(const std::string& s) {
            std::string out;
            for (char c : s) {
                switch (c) {
                    case '"': out += "\\\""; break;
                    case '\\': out += "\\\\"; break;
                    case '\n': out += "\\n"; break;
                    case '\r': out += "\\r"; break;
                    case '\t': out += "\\t"; break;
                    default:
                        if (static_cast<unsigned char>(c) < 0x20) {
                            char buf[8];
                            snprintf(buf, sizeof(buf), "\\u%04X", c);
                            out += buf;
                        } else {
                            out += c;
                        }
                }
            }
            return out;
        }
    }

    namespace {
        // ---- data-mod JSON generation (templates and session-diff exports) ----

        std::map<Unreal::UObject*, std::vector<MMARow>> s_pristineRows; // state at first parse this session

        struct CatalogProp { std::string Name; std::string Type; bool bIsArray; };
        std::map<std::string, std::vector<CatalogProp>> s_schemaCatalog;
        bool s_schemaCatalogDirty = false;

        std::vector<MMARow> DeepCopyRows(const std::vector<MMARow>& rows) {
            std::vector<MMARow> out;
            out.reserve(rows.size());
            for (const auto& r : rows) {
                MMARow copy = r;
                copy.Values = CloneRowValues(r.Values);
                out.push_back(std::move(copy));
            }
            return out;
        }

        bool ValuesEqual(const PropertyValue& a, const PropertyValue& b) {
            if (a.index() != b.index()) return false;
            if (auto* arrA = std::get_if<std::shared_ptr<PropertyArray>>(&a)) {
                auto* arrB = std::get_if<std::shared_ptr<PropertyArray>>(&b);
                size_t na = *arrA ? (*arrA)->Elements.size() : 0;
                size_t nb = *arrB ? (*arrB)->Elements.size() : 0;
                if (na != nb) return false;
                for (size_t i = 0; i < na; ++i) {
                    if (!ValuesEqual((*arrA)->Elements[i], (*arrB)->Elements[i])) return false;
                }
                return true;
            }
            if (auto* nameA = std::get_if<FName>(&a)) {
                auto* nameB = std::get_if<FName>(&b);
                return nameA->GetComparisonIndex().ToUnstableInt() == nameB->GetComparisonIndex().ToUnstableInt() &&
                       nameA->GetNumber() == nameB->GetNumber();
            }
            return a == b;
        }

        void AppendModJsonScalar(std::string& out, const PropertyValue& v) {
            std::visit([&out](auto&& val) {
                using T = std::decay_t<decltype(val)>;
                if constexpr (std::is_same_v<T, bool>) {
                    out += val ? "true" : "false";
                } else if constexpr (std::is_same_v<T, float>) {
                    if (std::isnan(val)) { out += "NaN"; }
                    else if (std::isinf(val)) { out += val > 0 ? "Infinity" : "-Infinity"; }
                    else { char buf[48]; snprintf(buf, sizeof(buf), "%.9g", val); out += buf; }
                } else if constexpr (std::is_same_v<T, uint8_t> || std::is_same_v<T, uint16_t> ||
                                     std::is_same_v<T, uint32_t>) {
                    out += std::to_string(static_cast<uint64_t>(val));
                } else if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, int16_t> ||
                                     std::is_same_v<T, int32_t> || std::is_same_v<T, int64_t>) {
                    out += std::to_string(static_cast<int64_t>(val));
                } else if constexpr (std::is_same_v<T, StringType>) {
                    out += '"';
                    out += EscapeJson(to_utf8_string(val));
                    out += '"';
                } else if constexpr (std::is_same_v<T, FName>) {
                    out += '"';
                    out += EscapeJson(to_utf8_string(val.ToString()));
                    out += '"';
                } else {
                    out += "null";
                }
            }, v);
        }

        void AppendModJsonValue(std::string& out, const PropertyValue& v) {
            if (auto* arr = std::get_if<std::shared_ptr<PropertyArray>>(&v)) {
                out += '[';
                if (*arr) {
                    for (size_t i = 0; i < (*arr)->Elements.size(); ++i) {
                        if (i) out += ", ";
                        AppendModJsonScalar(out, (*arr)->Elements[i]);
                    }
                }
                out += ']';
            } else {
                AppendModJsonScalar(out, v);
            }
        }

        // One row in the data-mod format; onlyProps limits to changed properties
        void AppendModJsonRow(std::string& out, const std::vector<FFF7Property>& schema, const MMARow& row,
                              const std::set<int32_t>* onlyProps, const char* indent) {
            out += '{';
            bool first = true;
            for (size_t i = 0; i < schema.size() && i < row.Values.size(); ++i) {
                if (onlyProps && !onlyProps->count(static_cast<int32_t>(i))) continue;
                out += first ? "\n" : ",\n";
                first = false;
                out += indent;
                out += "  \"";
                out += EscapeJson(to_utf8_string(schema[i].Name.ToString()));
                out += "\": ";
                AppendModJsonValue(out, row.Values[i]);
            }
            out += "\n";
            out += indent;
            out += '}';
        }

        // Self-documenting starter file in the data-mod format with one example row.
        // The "$schema" block is ignored by the loader.
        bool WriteTemplateFile(const std::filesystem::path& path, const std::string& tableName,
                               const MMADataTable& table) {
            const auto& schema = table.GetSchema();
            std::string buf;
            buf += "{\n  \"$schema\": {\n    \"table\": \"" + EscapeJson(tableName) + "\",\n";
            buf += "    \"usage\": \"copy into ue4ss/Mods/FF7MMAMod/data/ and keep only the rows and properties you want to change. An existing row name edits that row; a new name adds a row ($clone copies one first). {\\\"$delete\\\": true} removes a row, \\\"$all\\\" edits every row, \\\"$priority\\\" sets a new row's key priority.\",\n";
            buf += "    \"properties\": {\n";
            for (size_t i = 0; i < schema.size(); ++i) {
                buf += "      \"" + EscapeJson(to_utf8_string(schema[i].Name.ToString())) + "\": \"";
                buf += PropertyTypeName(schema[i].Type);
                if (schema[i].bIsArray) buf += "[]";
                buf += i + 1 < schema.size() ? "\",\n" : "\"\n";
            }
            buf += "    }\n  },\n";
            buf += "  \"" + EscapeJson(tableName) + "\": {\n";
            if (!table.GetRows().empty()) {
                const MMARow& row = table.GetRows().front();
                buf += "    \"" + EscapeJson(to_utf8_string(row.RowName.ToString())) + "\": ";
                AppendModJsonRow(buf, schema, row, nullptr, "    ");
                buf += "\n";
            }
            buf += "  }\n}\n";

            std::ofstream out(path, std::ios::binary | std::ios::trunc);
            if (!out.is_open()) return false;
            out << buf;
            return out.good();
        }

        // Minimal data-mod JSON covering everything that changed since the table was
        // first parsed this session. Returns the file path, or empty if no changes.
        std::string ExportSessionDiff(const std::filesystem::path& dumpDir, const std::string& tableName,
                                      const MMADataTable& table, const std::vector<MMARow>& pristine) {
            const auto& schema = table.GetSchema();
            const auto& rows = table.GetRows();

            std::map<std::string, const MMARow*> pristineByName;
            for (const auto& r : pristine) {
                pristineByName[to_utf8_string(r.RowName.ToString())] = &r;
            }
            std::set<std::string> currentNames;

            std::string body;
            bool firstRow = true;
            auto beginRow = [&](const std::string& rowName) {
                body += firstRow ? "\n" : ",\n";
                firstRow = false;
                body += "    \"" + EscapeJson(rowName) + "\": ";
            };

            for (const auto& row : rows) {
                std::string rowName = to_utf8_string(row.RowName.ToString());
                currentNames.insert(rowName);
                auto it = pristineByName.find(rowName);
                if (it == pristineByName.end()) {
                    // added row: emit all properties
                    beginRow(rowName);
                    AppendModJsonRow(body, schema, row, nullptr, "    ");
                    continue;
                }
                std::set<int32_t> changed;
                const MMARow& old = *it->second;
                size_t n = row.Values.size() < old.Values.size() ? row.Values.size() : old.Values.size();
                for (size_t i = 0; i < n; ++i) {
                    if (!ValuesEqual(row.Values[i], old.Values[i])) {
                        changed.insert(static_cast<int32_t>(i));
                    }
                }
                if (!changed.empty()) {
                    beginRow(rowName);
                    AppendModJsonRow(body, schema, row, &changed, "    ");
                }
            }
            for (const auto& [rowName, oldRow] : pristineByName) {
                (void)oldRow;
                if (!currentNames.count(rowName)) {
                    beginRow(rowName);
                    body += "{ \"$delete\": true }";
                }
            }

            if (firstRow) {
                return {}; // nothing changed
            }

            std::error_code ec;
            auto dir = dumpDir / "exports";
            std::filesystem::create_directories(dir, ec);
            std::time_t now = std::time(nullptr);
            std::tm tmv{};
            localtime_s(&tmv, &now);
            char stamp[32];
            std::strftime(stamp, sizeof(stamp), "%Y%m%d_%H%M%S", &tmv);
            auto path = dir / (SanitizeFileName(tableName) + "_" + stamp + ".json");

            std::ofstream out(path, std::ios::binary | std::ios::trunc);
            if (!out.is_open()) return {};
            out << "{\n  \"" << EscapeJson(tableName) << "\": {" << body << "\n  }\n}\n";
            return to_utf8_string(path.wstring());
        }
    }

    FF7MMAMod::FF7MMAMod() : CppUserModBase() {
        ModName = STR("FF7MMAMod");
        ModVersion = STR("1.1");
        ModDescription = STR("FF7 Memory Mapped Asset viewer and dumper");
        ModAuthors = STR("Narknon");

        s_instance = this;

        register_tab(STR("FF7 MMA"), [](CppUserModBase* mod) {
            UE4SS_ENABLE_IMGUI()
            static_cast<FF7MMAMod*>(mod)->RenderUI();
        });
    }

    FF7MMAMod::~FF7MMAMod() {
        if (s_instance == this) {
            s_instance = nullptr;
        }
    }

    auto FF7MMAMod::on_unreal_init() -> void {
        Output::send<LogLevel::Default>(STR("[FF7MMAMod] Unreal initialized\n"));
        CacheMMABaseClass();

        m_dumpDir = GetExeDirectory() / "ue4ss" / "MMADumps";
        std::error_code ec;
        std::filesystem::create_directories(m_dumpDir, ec);

        // Remove stale run artifacts so the harness only sees fresh output
        std::filesystem::remove(m_dumpDir / "COMPLETE", ec);
        std::filesystem::remove(m_dumpDir / "status.json", ec);

        std::ifstream targets(m_dumpDir / "targets.txt");
        if (targets.is_open()) {
            std::string line;
            while (std::getline(targets, line)) {
                while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
                if (!line.empty() && line[0] != '#') {
                    m_targets.insert(ToLower(line));
                }
            }
        }

        LoadDataFiles();
        SetupBlueprintBridge();

        m_startTime = std::chrono::steady_clock::now();
        m_lastScan = m_startTime - std::chrono::seconds(60);
        m_unrealInitialized = true;
    }

    void FF7MMAMod::LoadDataFiles() {
        auto dataDir = GetExeDirectory() / "ue4ss" / "Mods" / "FF7MMAMod" / "data";
        std::error_code ec;
        if (!std::filesystem::exists(dataDir, ec)) {
            return;
        }
        std::vector<std::filesystem::path> files;
        for (const auto& entry : std::filesystem::directory_iterator(dataDir, ec)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                files.push_back(entry.path());
            }
        }
        std::sort(files.begin(), files.end());
        for (const auto& file : files) {
            std::ifstream in(file, std::ios::binary);
            std::stringstream ss;
            ss << in.rdbuf();
            std::string buffer = ss.str();

            ModJson json;
            auto err = glz::read_json(json, buffer);
            std::string fileName = to_utf8_string(file.filename().wstring());
            if (err) {
                s_modResults[fileName] = "JSON parse error: " + glz::format_error(err, buffer);
                Output::send<LogLevel::Warning>(STR("[FF7MMAMod] Failed to parse {}\n"),
                                                to_generic_string(fileName));
                continue;
            }
            if (!json.is_object()) {
                s_modResults[fileName] = "top level must be an object of tables";
                continue;
            }
            size_t rowCount = 0;
            for (auto& [tableName, rowsVal] : json.get_object()) {
                if (!tableName.empty() && tableName[0] == '$') continue; // metadata like "$schema"
                if (!rowsVal.is_object()) continue;
                s_tableModNames[NormalizeTableKey(tableName)] = tableName;
                auto& dest = s_tableMods[NormalizeTableKey(tableName)];
                for (auto& [rowName, rowVal] : rowsVal.get_object()) {
                    // merge per property so two mods can edit different properties
                    // of the same row; a $delete replaces earlier edits outright
                    auto existingIt = dest.find(rowName);
                    bool incomingDeletes = rowVal.is_object() && rowVal.get_object().count("$delete");
                    if (existingIt != dest.end() && existingIt->second.is_object() && rowVal.is_object() &&
                        !incomingDeletes && !existingIt->second.get_object().count("$delete")) {
                        auto& target = existingIt->second.get_object();
                        for (auto& [propName, propVal] : rowVal.get_object()) {
                            target[propName] = std::move(propVal);
                        }
                    } else {
                        dest[rowName] = std::move(rowVal);
                    }
                    ++rowCount;
                }
            }
            Output::send<LogLevel::Default>(STR("[FF7MMAMod] Loaded {} ({} row mods)\n"),
                                            to_generic_string(fileName), rowCount);
        }
    }

    void FF7MMAMod::ApplyTableMods(Unreal::UObject* obj, const std::string& name, const std::string& lowerName) {
        auto modsIt = s_tableMods.find(lowerName);
        if (modsIt == s_tableMods.end() || s_appliedTables.count(lowerName)) {
            return;
        }

        MMAFields fields = ReadMMAFields(obj);
        if (!fields.Content || !fields.Layout || fields.FrozenContentSize == 0) {
            return; // not loaded yet, retry next scan
        }
        s_appliedTables.insert(lowerName);

        std::string summary = ApplyEntriesToObject(obj, &modsIt->second);
        s_modResults[name] = summary;
        QueueBpTableEvent(name, summary);
        Output::send<LogLevel::Default>(STR("[FF7MMAMod] {} {}\n"), to_generic_string(name), to_generic_string(summary));
    }

    std::string FF7MMAMod::ApplyEntriesToObject(Unreal::UObject* obj, const void* rowEntries) {
        std::scoped_lock lock(m_opMutex);
        const auto& entries = *static_cast<const ModJson::object_t*>(rowEntries);

        MMAFields fields = ReadMMAFields(obj);
        if (!fields.Content || !fields.Layout || fields.FrozenContentSize == 0) {
            return "table not loaded";
        }

        uint8_t* parseContent = fields.Content;
        size_t parseSize = fields.FrozenContentSize;
        GetParseSource(obj, parseContent, parseSize);

        MMADataTable table;
        if (!table.ParseFromLayout(parseContent, parseSize, fields.Layout)) {
            return "parse failed: " + table.GetError();
        }

        const auto& schema = table.GetSchema();
        auto& rows = table.GetRowsMutable();
        int edits = 0, adds = 0, dels = 0;
        std::vector<std::string> errs;
        std::map<size_t, std::set<int32_t>> changedByRow;

        // "$all" applies its properties to every row, before named-row entries
        auto allIt = entries.find("$all");
        if (allIt != entries.end() && allIt->second.is_object()) {
            const auto& allObj = allIt->second.get_object();
            for (size_t i = 0; i < rows.size(); ++i) {
                std::vector<int32_t> changed;
                if (!ApplyRowProps(rows[i], schema, allObj, errs, &changed)) {
                    errs.push_back("(in $all edit)");
                    break;
                }
                changedByRow[i].insert(changed.begin(), changed.end());
            }
            edits += static_cast<int>(rows.size());
        }

        std::set<std::string> deleteNames;

        // Row names containing '*' are patterns that edit (or delete) every
        // matching existing row. Exact-name entries are applied afterwards and win.
        for (const auto& [rowName, rowVal] : entries) {
            if (rowName.empty() || rowName[0] == '$') continue;
            if (rowName.find('*') == std::string::npos) continue;

            bool wantsDelete = rowVal.is_null();
            const ModJson::object_t* rowObj = nullptr;
            if (rowVal.is_object()) {
                rowObj = &rowVal.get_object();
                auto delIt = rowObj->find("$delete");
                if (delIt != rowObj->end() && delIt->second.holds<bool>() && delIt->second.get<bool>()) {
                    wantsDelete = true;
                }
            } else if (!wantsDelete) {
                errs.push_back("wildcard '" + rowName + "' must be an object or null");
                continue;
            }

            int matched = 0;
            for (size_t i = 0; i < rows.size(); ++i) {
                std::string currentName = to_utf8_string(rows[i].RowName.ToString());
                if (!WildcardMatch(rowName, currentName)) continue;
                ++matched;
                if (wantsDelete) {
                    deleteNames.insert(currentName);
                    continue;
                }
                std::vector<int32_t> changed;
                if (!ApplyRowProps(rows[i], schema, *rowObj, errs, &changed)) {
                    errs.push_back("(in wildcard edit '" + rowName + "')");
                }
                changedByRow[i].insert(changed.begin(), changed.end());
            }
            if (matched == 0) {
                errs.push_back("wildcard '" + rowName + "' matched no rows");
            } else if (!wantsDelete) {
                edits += matched;
            }
        }

        for (const auto& [rowName, rowVal] : entries) {
            if (!rowName.empty() && rowName[0] == '$') continue; // $all handled above, others are metadata
            if (rowName.find('*') != std::string::npos) continue; // wildcards handled above
            if (rowVal.is_null()) { // PalSchema-style deletion
                deleteNames.insert(rowName);
                continue;
            }
            if (!rowVal.is_object()) {
                errs.push_back("row '" + rowName + "' must be an object");
                continue;
            }
            const auto& rowObj = rowVal.get_object();
            FName rn(to_generic_string(rowName), Unreal::FNAME_Add);

            auto delIt = rowObj.find("$delete");
            if (delIt != rowObj.end() && delIt->second.holds<bool>() && delIt->second.get<bool>()) {
                deleteNames.insert(rowName);
                continue;
            }

            MMARow* existing = table.FindRow(rn);
            if (existing) {
                std::vector<int32_t> changed;
                if (!ApplyRowProps(*existing, schema, rowObj, errs, &changed)) {
                    errs.push_back("(in edit of '" + rowName + "')");
                }
                size_t rowIdx = static_cast<size_t>(existing - rows.data());
                changedByRow[rowIdx].insert(changed.begin(), changed.end());
                ++edits;
                continue;
            }

            // New row: clone an existing one or start from defaults
            MMARow newRow;
            newRow.RowName = rn;
            auto cloneIt = rowObj.find("$clone");
            if (cloneIt != rowObj.end() && cloneIt->second.holds<std::string>()) {
                FName src(to_generic_string(cloneIt->second.get<std::string>()), Unreal::FNAME_Add);
                MMARow* srcRow = table.FindRow(src);
                if (!srcRow) {
                    errs.push_back("clone source '" + cloneIt->second.get<std::string>() + "' not found for '" + rowName + "'");
                    continue;
                }
                newRow.Values = CloneRowValues(srcRow->Values);
            } else {
                newRow.Values.reserve(schema.size());
                for (const auto& prop : schema) {
                    newRow.Values.push_back(DefaultValue(prop));
                }
            }
            int32_t maxPriority = 0;
            for (const auto& r : rows) maxPriority = std::max(maxPriority, r.Priority);
            newRow.Priority = maxPriority + 1;
            auto prioIt = rowObj.find("$priority");
            int64_t prio;
            if (prioIt != rowObj.end() && GetJsonInt(prioIt->second, prio)) {
                newRow.Priority = static_cast<int32_t>(prio);
            }
            if (!ApplyRowProps(newRow, schema, rowObj, errs)) {
                errs.push_back("(in add of '" + rowName + "')");
            }
            rows.push_back(std::move(newRow));
            ++adds;
        }

        // Patch every edited value of pre-existing rows into the live blob (the
        // memory the game actually reads); rows added above have no live offsets.
        int patched = 0, patchFailed = 0;
        for (const auto& [rowIdx, props] : changedByRow) {
            for (int32_t propIdx : props) {
                std::string patchMessage;
                bool ok;
                const auto& prop = schema[propIdx];
                if (prop.bIsArray) {
                    ok = PatchArrayLive(obj, table, rowIdx, propIdx, patchMessage);
                } else if (prop.Type == EFF7PropertyType::StrProperty) {
                    ok = PatchStringLive(obj, table, rowIdx, propIdx, patchMessage);
                } else {
                    ok = PatchScalarLive(obj, table, rowIdx, propIdx, rows[rowIdx].Values[propIdx], patchMessage);
                }
                if (ok) {
                    ++patched;
                } else {
                    ++patchFailed;
                    if (patchFailed <= 5) errs.push_back(patchMessage);
                }
            }
        }

        if (!deleteNames.empty()) {
            auto shouldDelete = [&](const MMARow& r) {
                return deleteNames.count(to_utf8_string(r.RowName.ToString())) != 0;
            };
            size_t before = rows.size();
            rows.erase(std::remove_if(rows.begin(), rows.end(), shouldDelete), rows.end());
            dels = static_cast<int>(before - rows.size());
        }

        // Structural changes need the rebuilt image with the root headers redirected
        std::string redirectMessage;
        if (adds > 0 || dels > 0) {
            if (!RebuildAndSwap(obj, table, redirectMessage)) {
                errs.push_back("structural: " + redirectMessage);
                redirectMessage.clear();
            }
        }

        std::string summary = "applied: " + std::to_string(edits) + " edits (" +
                              std::to_string(patched) + " values patched live" +
                              (patchFailed ? ", " + std::to_string(patchFailed) + " failed" : "") + "), " +
                              std::to_string(adds) + " adds, " + std::to_string(dels) + " deletes";
        if (!redirectMessage.empty()) {
            summary += "; " + redirectMessage;
        }
        if (!errs.empty()) {
            summary += "; errors: ";
            for (size_t i = 0; i < errs.size() && i < 10; ++i) {
                if (i) summary += " | ";
                summary += errs[i];
            }
        }

        return summary;
    }

    Unreal::UObject* FF7MMAMod::CreateDataObject(Unreal::UObject* source, const std::string& newName,
                                                 bool copyRows, std::string& outMessage) {
        std::scoped_lock lock(m_opMutex);
        if (s_layoutFieldOffset == 0) {
            outMessage = "layout field offset not located yet";
            return nullptr;
        }
        if (!source || !IsDataObject(source)) {
            outMessage = "source is not a data object";
            return nullptr;
        }
        MMAFields srcFields = ReadMMAFields(source);
        if (!srcFields.Layout || !srcFields.Content || srcFields.FrozenContentSize == 0) {
            outMessage = "source not loaded";
            return nullptr;
        }

        Unreal::UObject* outer = source->GetOutermost();
        StringType wideName = to_generic_string(newName);
        // collision check against the MMA population only; a full GUObjectArray
        // FindObject walks outer chains of half-constructed objects during boot
        for (auto* existing : FindAllMMAObjects()) {
            if (existing && ToLower(to_utf8_string(existing->GetName())) == ToLower(newName)) {
                outMessage = "an object named '" + newName + "' already exists";
                return nullptr;
            }
        }

        uint8_t* parseContent = srcFields.Content;
        size_t parseSize = srcFields.FrozenContentSize;
        GetParseSource(source, parseContent, parseSize);

        MMADataTable table;
        if (!table.ParseFromLayout(parseContent, parseSize, srcFields.Layout)) {
            outMessage = "source parse failed: " + table.GetError();
            return nullptr;
        }
        if (!copyRows) {
            table.GetRowsMutable().clear();
        }

        std::vector<std::string> buildWarnings;
        auto built = MMAImageBuilder::Build(table, buildWarnings);
        if (!built.Data) {
            outMessage = "image build failed";
            return nullptr;
        }
        MMADataTable check;
        if (!check.ParseFromLayout(built.Data, built.Size, srcFields.Layout) ||
            check.GetRows().size() != table.GetRows().size()) {
            outMessage = "roundtrip verification failed: " + check.GetError();
            ::operator delete(built.Data);
            return nullptr;
        }
        s_blobs.push_back(built.Data);

        Unreal::FStaticConstructObjectParameters params(source->GetClassPrivate(), outer);
        params.Name = FName(wideName, Unreal::FNAME_Add);
        params.SetFlags = static_cast<Unreal::EObjectFlags>(
            Unreal::RF_Public | Unreal::RF_Standalone | Unreal::RF_MarkAsRootSet);
        Unreal::UObject* newObj = Unreal::UObjectGlobals::StaticConstructObject(params);
        if (!newObj) {
            outMessage = "object construction failed";
            return nullptr;
        }

        uint8_t* base = reinterpret_cast<uint8_t*>(newObj);
        *reinterpret_cast<Unreal::FTypeLayoutDesc**>(base + s_layoutFieldOffset) = srcFields.Layout;
        *reinterpret_cast<uint32_t*>(base + s_layoutFieldOffset + 8) = srcFields.SomeBools;
        *reinterpret_cast<uint32_t*>(base + s_layoutFieldOffset + 12) = static_cast<uint32_t>(built.Size);
        *reinterpret_cast<uint8_t**>(base + s_layoutFieldOffset + 16) = built.Data;

        outMessage = "created data object '" + newName + "' (" +
                     std::to_string(table.GetRows().size()) + " rows, image " +
                     std::to_string(built.Size) + " bytes)";
        Output::send<LogLevel::Default>(STR("[FF7MMAMod] {}\n"), to_generic_string(outMessage));
        return newObj;
    }

    std::string FF7MMAMod::ApplyJsonText(const std::string& jsonText) {
        std::scoped_lock lock(m_opMutex);
        ModJson doc;
        auto err = glz::read_json(doc, jsonText);
        if (err) {
            return "JSON parse error: " + glz::format_error(err, jsonText);
        }
        if (!doc.is_object()) {
            return "top level must be an object of tables";
        }

        std::map<std::string, Unreal::UObject*> byName;
        for (auto* candidate : FindAllMMAObjects()) {
            if (candidate) {
                byName[ToLower(to_utf8_string(candidate->GetName()))] = candidate;
            }
        }

        std::string result;
        for (auto& [tableName, rowsVal] : doc.get_object()) {
            if (!tableName.empty() && tableName[0] == '$') continue;
            if (!rowsVal.is_object()) continue;
            auto& entries = rowsVal.get_object();

            Unreal::UObject* obj = nullptr;
            auto found = byName.find(NormalizeTableKey(tableName));
            if (found != byName.end()) {
                obj = found->second;
            } else {
                auto createIt = entries.find("$create");
                if (createIt != entries.end() && createIt->second.holds<std::string>()) {
                    auto source = byName.find(NormalizeTableKey(createIt->second.get<std::string>()));
                    if (source != byName.end()) {
                        std::string createMessage;
                        obj = CreateDataObject(source->second, tableName, false, createMessage);
                        if (!obj) {
                            result += tableName + ": " + createMessage + "; ";
                            continue;
                        }
                    }
                }
                if (!obj) {
                    result += tableName + ": table not loaded; ";
                    continue;
                }
            }

            std::string summary = ApplyEntriesToObject(obj, &entries);
            QueueBpTableEvent(tableName, summary);
            result += tableName + ": " + summary + "; ";
        }
        if (result.empty()) {
            result = "no table entries in document";
        }
        return result;
    }

    std::string FF7MMAMod::ReloadDataMods() {
        std::scoped_lock lock(m_opMutex);
        s_tableMods.clear();
        s_tableModNames.clear();
        s_appliedTables.clear();
        s_modResults.clear();
        m_allModsEventFired = false;
        LoadDataFiles();

        size_t applied = 0;
        for (auto* obj : FindAllMMAObjects()) {
            if (!obj) continue;
            std::string name = to_utf8_string(obj->GetName());
            std::string lowerName = ToLower(name);
            if (!s_tableMods.count(lowerName)) continue;
            ApplyTableMods(obj, name, lowerName);
            if (s_appliedTables.count(lowerName)) ++applied;
        }
        WriteStatus();

        size_t targets = s_tableMods.size();
        std::string result = "reloaded " + std::to_string(targets) + " table mod(s): applied " +
                             std::to_string(applied) + ", " + std::to_string(targets - applied) +
                             " not loaded";
        Output::send<LogLevel::Default>(STR("[FF7MMAMod] {}\n"), to_generic_string(result));
        return result;
    }

    namespace {
        Unreal::FName s_fnApplyJson{};
        Unreal::FName s_fnSetValue{};
        Unreal::FName s_fnGetValue{};
        Unreal::FName s_fnGetRow{};
        Unreal::FName s_fnGetSchema{};
        Unreal::FName s_fnSet{};
        Unreal::FName s_fnReloadJson{};

        Unreal::FProperty* FindFuncParam(Unreal::UFunction* function, const CharType* name);

        // A typed Blueprint pin read from the stub function's own reflection.
        // The stub may declare Value as Float, Integer, Integer64, Boolean,
        // String or Name; we take whatever it is and convert.
        struct BpValue {
            enum class Kind { None, Number, Bool, Str, Name } ValueKind = Kind::None;
            double Number = 0.0;
            bool Flag = false;
            StringType Str;
            Unreal::FName Name;
        };

        BpValue ReadBpValue(Unreal::FFrame& stack, Unreal::UFunction* function, const CharType* name) {
            BpValue out;
            auto* prop = FindFuncParam(function, name);
            if (!prop || !stack.Locals()) return out;
            uint8_t* ptr = stack.Locals() + prop->GetOffset_Internal();
            StringType propClass = prop->GetClass().GetName();
            if (propClass == STR("FloatProperty")) {
                out.ValueKind = BpValue::Kind::Number;
                out.Number = *reinterpret_cast<float*>(ptr);
            } else if (propClass == STR("DoubleProperty")) {
                out.ValueKind = BpValue::Kind::Number;
                out.Number = *reinterpret_cast<double*>(ptr);
            } else if (propClass == STR("IntProperty")) {
                out.ValueKind = BpValue::Kind::Number;
                out.Number = *reinterpret_cast<int32_t*>(ptr);
            } else if (propClass == STR("Int64Property")) {
                out.ValueKind = BpValue::Kind::Number;
                out.Number = static_cast<double>(*reinterpret_cast<int64_t*>(ptr));
            } else if (propClass == STR("ByteProperty")) {
                out.ValueKind = BpValue::Kind::Number;
                out.Number = *ptr;
            } else if (propClass == STR("BoolProperty")) {
                out.ValueKind = BpValue::Kind::Bool;
                out.Flag = *ptr != 0;
            } else if (propClass == STR("StrProperty")) {
                out.ValueKind = BpValue::Kind::Str;
                auto* str = reinterpret_cast<Unreal::FString*>(ptr);
                const auto& chars = str->GetCharArray();
                if (chars.Num() > 0 && chars.GetData()) out.Str = StringType(chars.GetData());
            } else if (propClass == STR("NameProperty")) {
                out.ValueKind = BpValue::Kind::Name;
                out.Name = *reinterpret_cast<Unreal::FName*>(ptr);
            }
            return out;
        }

        // Converts a Blueprint pin value to the table property's type
        bool ConvertBpValue(const BpValue& in, EFF7PropertyType type, PropertyValue& out, std::string& err) {
            switch (type) {
                case EFF7PropertyType::BoolProperty:
                    if (in.ValueKind == BpValue::Kind::Bool) { out = in.Flag; return true; }
                    if (in.ValueKind == BpValue::Kind::Number) { out = in.Number != 0.0; return true; }
                    if (in.ValueKind == BpValue::Kind::Str) {
                        std::string text = ToLower(to_utf8_string(in.Str));
                        if (text == "true" || text == "1") { out = true; return true; }
                        if (text == "false" || text == "0" || text.empty()) { out = false; return true; }
                        err = "cannot read '" + text + "' as bool";
                        return false;
                    }
                    err = "property is Bool, pin is not";
                    return false;
                case EFF7PropertyType::ByteProperty:
                case EFF7PropertyType::Int8Property:
                case EFF7PropertyType::UInt16Property:
                case EFF7PropertyType::Int16Property:
                case EFF7PropertyType::UIntProperty:
                case EFF7PropertyType::IntProperty:
                case EFF7PropertyType::Int64Property:
                case EFF7PropertyType::FloatProperty: {
                    double v;
                    if (in.ValueKind == BpValue::Kind::Number) v = in.Number;
                    else if (in.ValueKind == BpValue::Kind::Bool) v = in.Flag ? 1.0 : 0.0;
                    else if (in.ValueKind == BpValue::Kind::Str) {
                        std::string text = to_utf8_string(in.Str);
                        char* end = nullptr;
                        v = std::strtod(text.c_str(), &end);
                        if (!end || end == text.c_str()) {
                            err = "cannot read '" + text + "' as a number";
                            return false;
                        }
                    }
                    else { err = "property is numeric, pin is not"; return false; }
                    switch (type) {
                        case EFF7PropertyType::ByteProperty: out = static_cast<uint8_t>(v); break;
                        case EFF7PropertyType::Int8Property: out = static_cast<int8_t>(v); break;
                        case EFF7PropertyType::UInt16Property: out = static_cast<uint16_t>(v); break;
                        case EFF7PropertyType::Int16Property: out = static_cast<int16_t>(v); break;
                        case EFF7PropertyType::UIntProperty: out = static_cast<uint32_t>(v); break;
                        case EFF7PropertyType::IntProperty: out = static_cast<int32_t>(v); break;
                        case EFF7PropertyType::Int64Property: out = static_cast<int64_t>(v); break;
                        default: out = static_cast<float>(v); break;
                    }
                    return true;
                }
                case EFF7PropertyType::StrProperty:
                    if (in.ValueKind == BpValue::Kind::Str) { out = in.Str; return true; }
                    if (in.ValueKind == BpValue::Kind::Name) { out = in.Name.ToString(); return true; }
                    err = "property is String, pin is not";
                    return false;
                case EFF7PropertyType::NameProperty:
                    if (in.ValueKind == BpValue::Kind::Name) { out = in.Name; return true; }
                    if (in.ValueKind == BpValue::Kind::Str) {
                        out = Unreal::FName(in.Str, Unreal::FNAME_Add);
                        return true;
                    }
                    err = "property is Name, pin is not";
                    return false;
                default:
                    err = "unsupported property type";
                    return false;
            }
        }

        Unreal::FProperty* FindFuncParam(Unreal::UFunction* function, const CharType* name) {
            for (Unreal::FProperty* prop = function->GetFirstProperty(); prop; prop = prop->GetNextFieldAsProperty()) {
                if (prop->GetName() == name) return prop;
            }
            return nullptr;
        }

        // Out params are not in the callee frame: for blueprint to blueprint calls the
        // frame records the caller variable addresses in OutParms, so resolve through
        // that first and fall back to Locals for inputs
        uint8_t* ParamAddress(Unreal::FFrame& stack, Unreal::FProperty* prop) {
            for (auto* rec = stack.OutParms(); rec; rec = rec->NextOutParm) {
                if (rec->Property == prop) return rec->PropAddr;
            }
            return stack.Locals() ? stack.Locals() + prop->GetOffset_Internal() : nullptr;
        }

        StringType ReadStrParam(Unreal::FFrame& stack, Unreal::UFunction* function, const CharType* name) {
            auto* prop = FindFuncParam(function, name);
            if (!prop || !stack.Locals()) return {};
            auto* str = reinterpret_cast<Unreal::FString*>(ParamAddress(stack, prop));
            const auto& chars = str->GetCharArray();
            if (chars.Num() <= 0 || !chars.GetData()) return {};
            return StringType(chars.GetData());
        }

        void WriteStrParam(Unreal::FFrame& stack, Unreal::UFunction* function, const CharType* name,
                           const StringType& value) {
            auto* prop = FindFuncParam(function, name);
            if (!prop || !stack.Locals()) return;
            auto* str = reinterpret_cast<Unreal::FString*>(ParamAddress(stack, prop));
            *str = Unreal::FString(value);
        }
    }

    void FF7MMAMod::SetupBlueprintBridge() {
        s_fnApplyJson = FName(STR("MMA_ApplyJson"), Unreal::FNAME_Add);
        s_fnSetValue = FName(STR("MMA_SetValue"), Unreal::FNAME_Add);
        s_fnGetValue = FName(STR("MMA_GetValue"), Unreal::FNAME_Add);
        s_fnGetRow = FName(STR("MMA_GetRow"), Unreal::FNAME_Add);
        s_fnGetSchema = FName(STR("MMA_GetSchema"), Unreal::FNAME_Add);
        s_fnSet = FName(STR("MMA_Set"), Unreal::FNAME_Add);
        s_fnReloadJson = FName(STR("MMA_ReloadJson"), Unreal::FNAME_Add);

        // Post hooks: the compiled stub assigns its outputs their defaults in its
        // return node, so the bridge has to write after the body has run
        Unreal::Hook::RegisterProcessLocalScriptFunctionPostCallback(
            [](auto& info, Unreal::UObject* Context, Unreal::FFrame& Stack, void* RESULT_DECL) {
                (void)info; (void)Context; (void)RESULT_DECL;
                if (auto* self = FF7MMAMod::GetInstance()) {
                    self->OnScriptFunctionPre(Stack);
                }
            },
            // bOnce must stay false or the bridge only serves the first
            // blueprint call of the session; it writes params so it is not readonly
            {false, false, STR("FF7MMAMod"), STR("BPBridge")});

        // Only self calls reach ProcessLocalScriptFunction. Calls into another
        // object (a function library included) and anything started through
        // ProcessEvent (Lua, C++, the UE4SS function caller) run ProcessInternal
        // instead, so the bridge has to sit on both.
        Unreal::Hook::RegisterProcessInternalPostCallback(
            [](auto& info, Unreal::UObject* Context, Unreal::FFrame& Stack, void* RESULT_DECL) {
                (void)info; (void)Context; (void)RESULT_DECL;
                if (auto* self = FF7MMAMod::GetInstance()) {
                    self->OnScriptFunctionPre(Stack);
                }
            },
            {false, false, STR("FF7MMAMod"), STR("BPBridgeProcessInternal")});

        // ProcessEvent runs the blueprint VM, so it has to be called on the game
        // thread. on_update runs on the UE4SS mod thread instead.
        Unreal::Hook::RegisterEngineTickPostCallback(
            [](auto& info, Unreal::UEngine* Context, float DeltaSeconds, bool bIdleMode) {
                (void)info; (void)Context; (void)DeltaSeconds; (void)bIdleMode;
                if (auto* self = FF7MMAMod::GetInstance()) {
                    self->FireBpEvents();
                }
            },
            {false, true, STR("FF7MMAMod"), STR("BPEvents")});
    }

    void FF7MMAMod::OnScriptFunctionPre(Unreal::FFrame& stack) {
        Unreal::UFunction* node = stack.Node();
        if (!node) return;
        Unreal::FName functionName = node->GetNamePrivate();

        bool isApply = functionName == s_fnApplyJson;
        bool isSet = functionName == s_fnSetValue;
        bool isGet = functionName == s_fnGetValue;
        bool isGetRow = functionName == s_fnGetRow;
        bool isGetSchema = functionName == s_fnGetSchema;
        bool isTypedSet = functionName == s_fnSet;
        bool isReload = functionName == s_fnReloadJson;
        if (!isApply && !isSet && !isGet && !isGetRow && !isGetSchema && !isTypedSet && !isReload) return;

        std::scoped_lock lock(m_opMutex);
        if (isTypedSet) {
            std::string tableName = NormalizeTableKey(to_utf8_string(ReadStrParam(stack, node, STR("Table"))));
            std::string rowName = to_utf8_string(ReadStrParam(stack, node, STR("Row")));
            std::string propName = to_utf8_string(ReadStrParam(stack, node, STR("Prop")));
            std::string result = "table not loaded";
            for (auto* candidate : FindAllMMAObjects()) {
                if (!candidate || ToLower(to_utf8_string(candidate->GetName())) != tableName) continue;
                MMADataTable* table = GetParsedTable(candidate);
                if (!table) { result = "table parse failed"; break; }
                MMARow* row = table->FindRow(FName(to_generic_string(rowName), Unreal::FNAME_Add));
                if (!row) { result = "row '" + rowName + "' not found"; break; }
                int32_t propIdx = table->FindPropertyIndex(FName(to_generic_string(propName), Unreal::FNAME_Add));
                if (propIdx < 0) { result = "property '" + propName + "' not found"; break; }
                const auto& prop = table->GetSchema()[propIdx];
                if (prop.bIsArray) { result = "property is an array, use MMA_ApplyJson"; break; }
                BpValue pinValue = ReadBpValue(stack, node, STR("Value"));
                if (pinValue.ValueKind == BpValue::Kind::None) {
                    result = "Value pin missing or of unsupported type";
                    break;
                }
                PropertyValue converted;
                std::string convertError;
                if (!ConvertBpValue(pinValue, prop.Type, converted, convertError)) {
                    result = convertError;
                    break;
                }
                size_t rowIdx = static_cast<size_t>(row - table->GetRows().data());
                std::string patchMessage;
                if (prop.Type == EFF7PropertyType::StrProperty) {
                    row->Values[propIdx] = converted;
                    result = PatchStringLive(candidate, *table, rowIdx, propIdx, patchMessage)
                                 ? patchMessage : "failed: " + patchMessage;
                } else {
                    result = PatchScalarLive(candidate, *table, rowIdx, propIdx, converted, patchMessage)
                                 ? patchMessage : "failed: " + patchMessage;
                }
                break;
            }
            WriteStrParam(stack, node, STR("Result"), to_generic_string(result));
        } else if (isReload) {
            WriteStrParam(stack, node, STR("Result"), to_generic_string(ReloadDataMods()));
        } else if (isApply) {
            std::string json = to_utf8_string(ReadStrParam(stack, node, STR("Json")));
            std::string result = ApplyJsonText(json);
            WriteStrParam(stack, node, STR("Result"), to_generic_string(result));
        } else if (isSet) {
            std::string tableName = to_utf8_string(ReadStrParam(stack, node, STR("Table")));
            std::string rowName = to_utf8_string(ReadStrParam(stack, node, STR("Row")));
            std::string propName = to_utf8_string(ReadStrParam(stack, node, STR("Prop")));
            std::string valueJson = to_utf8_string(ReadStrParam(stack, node, STR("ValueJson")));
            std::string doc = "{\"" + EscapeJson(tableName) + "\": {\"" + EscapeJson(rowName) +
                              "\": {\"" + EscapeJson(propName) + "\": " + valueJson + "}}}";
            std::string result = ApplyJsonText(doc);
            WriteStrParam(stack, node, STR("Result"), to_generic_string(result));
        } else {
            std::string tableName = NormalizeTableKey(to_utf8_string(ReadStrParam(stack, node, STR("Table"))));
            MMADataTable* table = nullptr;
            for (auto* candidate : FindAllMMAObjects()) {
                if (candidate && ToLower(to_utf8_string(candidate->GetName())) == tableName) {
                    table = GetParsedTable(candidate);
                    break;
                }
            }

            if (isGetSchema) {
                std::string schemaJson;
                if (table) {
                    schemaJson += '[';
                    const auto& schema = table->GetSchema();
                    for (size_t i = 0; i < schema.size(); ++i) {
                        if (i) schemaJson += ", ";
                        schemaJson += "{\"name\": \"" + EscapeJson(to_utf8_string(schema[i].Name.ToString())) +
                                      "\", \"type\": \"" + PropertyTypeName(schema[i].Type) +
                                      "\", \"isArray\": " + (schema[i].bIsArray ? "true" : "false") + "}";
                    }
                    schemaJson += ']';
                }
                WriteStrParam(stack, node, STR("SchemaJson"), to_generic_string(schemaJson));
                return;
            }

            std::string rowName = to_utf8_string(ReadStrParam(stack, node, STR("Row")));
            MMARow* row = table ? table->FindRow(FName(to_generic_string(rowName), Unreal::FNAME_Add)) : nullptr;

            if (isGetRow) {
                std::string rowJson;
                if (row) {
                    AppendModJsonRow(rowJson, table->GetSchema(), *row, nullptr, "");
                }
                WriteStrParam(stack, node, STR("RowJson"), to_generic_string(rowJson));
                return;
            }

            std::string propName = to_utf8_string(ReadStrParam(stack, node, STR("Prop")));
            std::string valueJson;
            if (row) {
                int32_t propIdx = table->FindPropertyIndex(FName(to_generic_string(propName), Unreal::FNAME_Add));
                if (propIdx >= 0 && propIdx < static_cast<int32_t>(row->Values.size())) {
                    AppendModJsonValue(valueJson, row->Values[propIdx]);
                }
            }
            WriteStrParam(stack, node, STR("ValueJson"), to_generic_string(valueJson));
        }
    }

    void FF7MMAMod::QueueBpTableEvent(const std::string& table, const std::string& summary) {
        m_pendingBpEvents.push_back({table, summary});
    }

    void FF7MMAMod::FireBpEvents() {
        // Resolving the event target scans the object array; do it rarely and give
        // up permanently when the loader blueprint is not installed.
        static std::chrono::steady_clock::time_point s_lastResolve{};
        static int s_resolveAttempts = 0;
        static bool s_eventsDisabled = false;

        // dispatching an event runs blueprint code, which re-enters the script
        // function hook that drives this
        static bool s_firing = false;
        if (s_firing) {
            return;
        }
        s_firing = true;
        struct ResetOnExit {
            bool& Flag;
            ~ResetOnExit() { Flag = false; }
        } resetOnExit{s_firing};

        if (s_eventsDisabled) {
            m_pendingBpEvents.clear();
            return;
        }
        auto now = std::chrono::steady_clock::now();
        if (now - s_lastResolve < std::chrono::seconds(5)) {
            return;
        }
        s_lastResolve = now;

        bool wantAllApplied = !m_allModsEventFired && !s_tableMods.empty();
        if (m_pendingBpEvents.empty() && !wantAllApplied && m_readyEventFired) {
            return;
        }
        if (++s_resolveAttempts > 24) {
            s_eventsDisabled = true;
            m_pendingBpEvents.clear();
            Output::send<LogLevel::Warning>(
                STR("[FF7MMAMod] BP events disabled: BP_EndGameInstance_C never resolved after 24 attempts\n"));
            return;
        }

        // MMA_OnReady tells the blueprint loader that the MMA_* stubs are being
        // intercepted from now on; calls made before this run their empty bodies
        if (!m_readyEventFired) {
            auto* target = Unreal::UObjectGlobals::FindFirstOf(STR("BP_EndGameInstance_C"));
            if (target && !target->HasAnyFlags(static_cast<Unreal::EObjectFlags>(0x00000010 | 0x00000020))) {
                if (auto* eventFunc = target->GetFunctionByNameInChain(STR("MMA_OnReady"))) {
                    int32_t parmsSize = eventFunc->GetParmsSize();
                    std::vector<uint8_t> parms(parmsSize > 0 ? parmsSize : 1, 0);
                    target->ProcessEvent(eventFunc, parms.data());
                    Output::send<LogLevel::Default>(STR("[FF7MMAMod] fired MMA_OnReady\n"));
                } else {
                    Output::send<LogLevel::Default>(
                        STR("[FF7MMAMod] MMA_OnReady is not defined on the loader, skipping\n"));
                }
                m_readyEventFired = true;
                s_resolveAttempts = 0;
            } else {
                Output::send<LogLevel::Default>(
                    STR("[FF7MMAMod] MMA_OnReady waiting for a BP_EndGameInstance_C instance (attempt {})\n"),
                    s_resolveAttempts);
            }
        }

        if (!m_pendingBpEvents.empty()) {
            auto* target = Unreal::UObjectGlobals::FindFirstOf(STR("BP_EndGameInstance_C"));
            // resolve through the instance's own class chain so a blueprint that
            // overrides the event gets its override run, not the base stub
            auto* eventFunc = target ? target->GetFunctionByNameInChain(STR("MMA_OnTableModsApplied")) : nullptr;
            bool targetValid = eventFunc && target &&
                               !target->HasAnyFlags(static_cast<Unreal::EObjectFlags>(0x00000010 | 0x00000020));
            if (!targetValid) {
                Output::send<LogLevel::Default>(
                    STR("[FF7MMAMod] MMA_OnTableModsApplied not fired yet (attempt {}): function {}, instance {}\n"),
                    s_resolveAttempts,
                    eventFunc ? STR("found") : STR("MISSING"),
                    target ? STR("found") : STR("MISSING"));
            }
            if (targetValid) {
                for (const auto& [tableName, summary] : m_pendingBpEvents) {
                    int32_t parmsSize = eventFunc->GetParmsSize();
                    std::vector<uint8_t> parms(parmsSize > 0 ? parmsSize : 1, 0);
                    auto* tableProp = FindFuncParam(eventFunc, STR("Table"));
                    auto* summaryProp = FindFuncParam(eventFunc, STR("Summary"));
                    if (tableProp) {
                        new (parms.data() + tableProp->GetOffset_Internal())
                            Unreal::FString(to_generic_string(tableName));
                    }
                    if (summaryProp) {
                        new (parms.data() + summaryProp->GetOffset_Internal())
                            Unreal::FString(to_generic_string(summary));
                    }
                    target->ProcessEvent(eventFunc, parms.data());
                    if (tableProp) {
                        reinterpret_cast<Unreal::FString*>(parms.data() + tableProp->GetOffset_Internal())->~FString();
                    }
                    if (summaryProp) {
                        reinterpret_cast<Unreal::FString*>(parms.data() + summaryProp->GetOffset_Internal())->~FString();
                    }
                }
                Output::send<LogLevel::Default>(
                    STR("[FF7MMAMod] fired MMA_OnTableModsApplied for {} table(s)\n"),
                    static_cast<int>(m_pendingBpEvents.size()));
                m_pendingBpEvents.clear();
                s_resolveAttempts = 0;
            }
        }

        if (!m_allModsEventFired && !s_tableMods.empty() && m_pendingBpEvents.empty()) {
            bool allApplied = true;
            for (const auto& [tableName, entries] : s_tableMods) {
                if (!s_appliedTables.count(tableName)) { allApplied = false; break; }
            }
            if (allApplied) {
                auto* target = Unreal::UObjectGlobals::FindFirstOf(STR("BP_EndGameInstance_C"));
                auto* eventFunc = target ? target->GetFunctionByNameInChain(STR("MMA_OnAllModsApplied")) : nullptr;
                if (eventFunc && target &&
                    !target->HasAnyFlags(static_cast<Unreal::EObjectFlags>(0x00000010 | 0x00000020))) {
                    int32_t parmsSize = eventFunc->GetParmsSize();
                    std::vector<uint8_t> parms(parmsSize > 0 ? parmsSize : 1, 0);
                    target->ProcessEvent(eventFunc, parms.data());
                    // only mark it done once it actually dispatched, otherwise a
                    // single failed lookup consumes the event permanently
                    m_allModsEventFired = true;
                    Output::send<LogLevel::Default>(STR("[FF7MMAMod] fired MMA_OnAllModsApplied\n"));
                } else {
                    Output::send<LogLevel::Default>(
                        STR("[FF7MMAMod] MMA_OnAllModsApplied not fired yet (attempt {}): function {}, instance {}\n"),
                        s_resolveAttempts,
                        eventFunc ? STR("found") : STR("MISSING"),
                        target ? STR("found") : STR("MISSING"));
                }
            }
        }
    }

    bool FF7MMAMod::RebuildAndSwap(Unreal::UObject* obj, MMADataTable& table, std::string& outMessage) {
        std::scoped_lock lock(m_opMutex);
        if (s_layoutFieldOffset == 0) {
            outMessage = "layout field offset not located yet";
            return false;
        }
        MMAFields fields = ReadMMAFields(obj);
        if (!fields.Layout || !fields.Content) {
            outMessage = "asset fields unavailable";
            return false;
        }

        std::vector<std::string> buildWarnings;
        auto built = MMAImageBuilder::Build(table, buildWarnings);
        if (!built.Data) {
            outMessage = "image build failed";
            return false;
        }

        // The rebuilt image must parse identically before the game can see it
        auto check = std::make_unique<MMADataTable>();
        if (!check->ParseFromLayout(built.Data, built.Size, fields.Layout) ||
            check->GetRows().size() != table.GetRows().size()) {
            outMessage = "roundtrip verification failed: " + check->GetError();
            ::operator delete(built.Data);
            return false;
        }
        s_blobs.push_back(built.Data);

        // Redirect the live blob's root headers into the rebuilt image. The game
        // keeps walking pointers from the original allocation, so the original is
        // never moved; its self-relative root pointers now target the new image.
        uint8_t* live = fields.Content;
        uint32_t keyOff = table.GetKeyToIndexOffset();
        uint32_t valuesOff = table.GetValuesOffset();
        if (!EnsureWritable(live + keyOff, 56) || !EnsureWritable(live + valuesOff, 16)) {
            outMessage = "live root headers are not writable";
            return false;
        }
        if (!s_savedRoots.count(obj)) {
            std::array<uint8_t, 88> saved{};
            std::memcpy(saved.data(), live, 88);
            s_savedRoots[obj] = saved;
        }

        int32_t numRows = built.NumRows;
        int32_t buckets = static_cast<int32_t>(built.BucketCount);
        int32_t firstFree = -1, numFree = 0;
        WritePackedPtrAt(live + keyOff, built.Data + built.KeysPos);
        std::memcpy(live + keyOff + 8, &numRows, 4);
        std::memcpy(live + keyOff + 12, &numRows, 4);
        WritePackedPtrAt(live + keyOff + 16, built.Data + built.BitsPos);
        std::memcpy(live + keyOff + 24, &numRows, 4);
        std::memcpy(live + keyOff + 28, &numRows, 4);
        std::memcpy(live + keyOff + 32, &firstFree, 4);
        std::memcpy(live + keyOff + 36, &numFree, 4);
        WritePackedPtrAt(live + keyOff + 40, built.Data + built.IndexesPos);
        std::memcpy(live + keyOff + 48, &buckets, 4);
        std::memcpy(live + keyOff + 52, &buckets, 4);
        WritePackedPtrAt(live + valuesOff, built.Data + built.ValuesPos);
        std::memcpy(live + valuesOff + 8, &numRows, 4);
        std::memcpy(live + valuesOff + 12, &numRows, 4);

        // Future edits patch the rebuilt image the roots now point at
        s_liveBase[obj] = { built.Data, built.Size };
        m_parsedTables[obj] = std::move(check);
        m_dirtyTables.erase(obj);

        outMessage = std::to_string(numRows) + " rows, root headers redirected to rebuilt image (" +
                     std::to_string(built.Size) + " bytes)";
        if (!buildWarnings.empty()) {
            outMessage += ", " + std::to_string(buildWarnings.size()) + " build warnings";
        }
        Output::send<LogLevel::Default>(STR("[FF7MMAMod] {}\n"), to_generic_string(outMessage));
        return true;
    }

    bool FF7MMAMod::RestoreOriginal(Unreal::UObject* obj) {
        std::scoped_lock lock(m_opMutex);
        auto it = s_savedRoots.find(obj);
        if (it == s_savedRoots.end()) {
            return false;
        }
        MMAFields fields = ReadMMAFields(obj);
        if (!fields.Content || !EnsureWritable(fields.Content, 88)) {
            return false;
        }
        std::memcpy(fields.Content, it->second.data(), 88);
        s_liveBase.erase(obj);
        m_parsedTables.erase(obj);
        m_dirtyTables.erase(obj);
        return true;
    }

    bool FF7MMAMod::PatchScalarLive(Unreal::UObject* obj, MMADataTable& table, size_t rowIdx, size_t propIdx,
                                    const PropertyValue& value, std::string& outMessage) {
        MMAFields fields = ReadMMAFields(obj);
        uint8_t* live = LiveBaseFor(obj, fields.Content);
        if (!live || live != table.GetContentPtr()) {
            outMessage = "live content changed; re-select the table";
            return false;
        }
        auto& rows = table.GetRowsMutable();
        if (rowIdx >= rows.size() || propIdx >= table.GetSchema().size()) {
            outMessage = "row/property out of range";
            return false;
        }
        auto& row = rows[rowIdx];
        if (propIdx >= row.ValueOffsets.size()) {
            outMessage = "row is not in the live image yet (use Apply Changes)";
            return false;
        }
        const auto& prop = table.GetSchema()[propIdx];
        if (prop.bIsArray || prop.Type == EFF7PropertyType::StrProperty) {
            outMessage = "not a scalar property";
            return false;
        }
        size_t size = ScalarByteSize(prop.Type);
        uint64_t offset = row.ValueOffsets[propIdx];
        if (offset + size > table.GetContentSize()) {
            outMessage = "value offset out of bounds";
            return false;
        }
        uint8_t* target = live + offset;
        if (!EnsureWritable(target, size)) {
            outMessage = "live memory is not writable";
            return false;
        }
        if (!WriteScalarBytes(target, prop.Type, value)) {
            outMessage = "value type mismatch";
            return false;
        }
        row.Values[propIdx] = value;
        outMessage = "patched live";
        return true;
    }

    bool FF7MMAMod::PatchStringLive(Unreal::UObject* obj, MMADataTable& table, size_t rowIdx, size_t propIdx,
                                    std::string& outMessage) {
        MMAFields fields = ReadMMAFields(obj);
        uint8_t* live = LiveBaseFor(obj, fields.Content);
        if (!live || live != table.GetContentPtr()) {
            outMessage = "live content changed; re-select the table";
            return false;
        }
        auto& rows = table.GetRowsMutable();
        if (rowIdx >= rows.size() || propIdx >= table.GetSchema().size()) {
            outMessage = "row/property out of range";
            return false;
        }
        auto& row = rows[rowIdx];
        if (propIdx >= row.ValueOffsets.size()) {
            outMessage = "row is not in the live image yet (use Apply Changes)";
            return false;
        }
        const auto* str = std::get_if<StringType>(&row.Values[propIdx]);
        if (!str) {
            outMessage = "value is not a string";
            return false;
        }
        uint64_t offset = row.ValueOffsets[propIdx];
        if (offset + 16 > table.GetContentSize()) {
            outMessage = "header offset out of bounds";
            return false;
        }
        uint8_t* header = live + offset;
        if (!EnsureWritable(header, 16)) {
            outMessage = "live memory is not writable";
            return false;
        }
        int32_t newNum = str->empty() ? 0 : static_cast<int32_t>(str->size()) + 1;
        if (newNum == 0) {
            int32_t zero = 0;
            std::memcpy(header + 8, &zero, 4);
            std::memcpy(header + 12, &zero, 4);
        } else {
            int32_t oldNum = 0;
            std::memcpy(&oldNum, header + 8, 4);
            size_t padded = static_cast<size_t>(newNum > oldNum ? newNum : oldNum) * 2;
            uint8_t* side = static_cast<uint8_t*>(::operator new(padded));
            std::memset(side, 0, padded);
            std::memcpy(side, str->data(), str->size() * 2);
            s_blobs.push_back(side);
            WritePackedPtrAt(header, side);
            std::memcpy(header + 8, &newNum, 4);
            std::memcpy(header + 12, &newNum, 4);
        }
        outMessage = "string patched live";
        return true;
    }

    bool FF7MMAMod::PatchArrayLive(Unreal::UObject* obj, MMADataTable& table, size_t rowIdx, size_t propIdx,
                                   std::string& outMessage) {
        MMAFields fields = ReadMMAFields(obj);
        uint8_t* live = LiveBaseFor(obj, fields.Content);
        if (!live || live != table.GetContentPtr()) {
            outMessage = "live content changed; re-select the table";
            return false;
        }
        auto& rows = table.GetRowsMutable();
        if (rowIdx >= rows.size() || propIdx >= table.GetSchema().size()) {
            outMessage = "row/property out of range";
            return false;
        }
        auto& row = rows[rowIdx];
        if (propIdx >= row.ValueOffsets.size()) {
            outMessage = "row is not in the live image yet (use Apply Changes)";
            return false;
        }
        const auto& prop = table.GetSchema()[propIdx];
        if (!prop.bIsArray) {
            outMessage = "not an array property";
            return false;
        }
        auto* arrPtr = std::get_if<std::shared_ptr<PropertyArray>>(&row.Values[propIdx]);
        auto arr = arrPtr ? *arrPtr : nullptr;
        int32_t newNum = arr ? static_cast<int32_t>(arr->Elements.size()) : 0;
        uint64_t offset = row.ValueOffsets[propIdx];
        if (offset + 16 > table.GetContentSize()) {
            outMessage = "header offset out of bounds";
            return false;
        }
        uint8_t* header = live + offset;
        if (!EnsureWritable(header, 16)) {
            outMessage = "live memory is not writable";
            return false;
        }
        if (newNum == 0) {
            int32_t zero = 0;
            std::memcpy(header + 8, &zero, 4);
            std::memcpy(header + 12, &zero, 4);
        } else {
            int32_t oldNum = 0;
            std::memcpy(&oldNum, header + 8, 4);
            uint8_t* side = BuildArraySideBlock(prop.Type, arr->Elements,
                                                static_cast<size_t>(oldNum > newNum ? oldNum : newNum));
            WritePackedPtrAt(header, side);
            std::memcpy(header + 8, &newNum, 4);
            std::memcpy(header + 12, &newNum, 4);
        }
        outMessage = "array patched live (" + std::to_string(newNum) + " elements)";
        return true;
    }

    auto FF7MMAMod::on_update() -> void {
        if (!m_unrealInitialized) {
            return;
        }
        ProcessPendingOps();
        CheckReloadTrigger();
        if (m_autoDumpDone) {
            return;
        }
        auto now = std::chrono::steady_clock::now();
        if (now - m_lastScan < std::chrono::seconds(5)) {
            return;
        }
        m_lastScan = now;
        AutoDumpTick();
    }

    // ue4ss/MMADumps/reload.trigger, created by an external tool, requests a JSON
    // reload. The file is removed once the reload is queued.
    void FF7MMAMod::CheckReloadTrigger() {
        auto now = std::chrono::steady_clock::now();
        if (now - m_lastTriggerCheck < std::chrono::seconds(2)) {
            return;
        }
        m_lastTriggerCheck = now;
        std::error_code ec;
        auto trigger = m_dumpDir / "reload.trigger";
        if (!std::filesystem::exists(trigger, ec)) {
            return;
        }
        std::filesystem::remove(trigger, ec);
        PendingOp op;
        op.OpKind = PendingOp::Kind::ReloadJson;
        QueueOp(std::move(op));
        m_reloadStatus = "reload queued (trigger file)...";
    }

    void FF7MMAMod::QueueOp(PendingOp op) {
        std::scoped_lock lock(m_opMutex);
        m_pendingOps.push_back(std::move(op));
    }

    // Runs on the game thread: FName creation and content swaps must not happen on
    // the render thread, and swapping at the frame boundary means no game code is
    // mid-iteration over the table.
    void FF7MMAMod::ProcessPendingOps() {
        std::scoped_lock lock(m_opMutex);
        if (m_pendingOps.empty()) {
            return;
        }
        std::vector<PendingOp> ops;
        ops.swap(m_pendingOps);

        for (auto& op : ops) {
            auto tableIt = m_parsedTables.find(op.Object);
            MMADataTable* table = tableIt != m_parsedTables.end() ? tableIt->second.get() : nullptr;
            std::string message;

            switch (op.OpKind) {
                case PendingOp::Kind::PatchValue: {
                    if (!table) break;
                    if (PatchScalarLive(op.Object, *table, op.RowIndex, op.PropIndex, op.Value, message)) {
                        m_applyStatus[op.Object] = message;
                    } else {
                        m_applyStatus[op.Object] = "patch FAILED: " + message;
                    }
                    break;
                }
                case PendingOp::Kind::PatchString: {
                    if (!table) break;
                    if (PatchStringLive(op.Object, *table, op.RowIndex, op.PropIndex, message)) {
                        m_applyStatus[op.Object] = message;
                    } else {
                        m_applyStatus[op.Object] = "patch FAILED: " + message;
                    }
                    break;
                }
                case PendingOp::Kind::PatchArray: {
                    if (!table) break;
                    if (PatchArrayLive(op.Object, *table, op.RowIndex, op.PropIndex, message)) {
                        m_applyStatus[op.Object] = message;
                    } else {
                        m_applyStatus[op.Object] = "patch FAILED: " + message;
                    }
                    break;
                }
                case PendingOp::Kind::NameEdit: {
                    if (!table) break;
                    auto& rows = table->GetRowsMutable();
                    if (op.RowIndex >= rows.size()) break;
                    auto& row = rows[op.RowIndex];
                    if (op.PropIndex >= row.Values.size()) break;
                    FName newName(to_generic_string(op.Text), Unreal::FNAME_Add);
                    if (op.ElemIndex != SIZE_MAX) {
                        auto* arrPtr = std::get_if<std::shared_ptr<PropertyArray>>(&row.Values[op.PropIndex]);
                        if (arrPtr && *arrPtr && op.ElemIndex < (*arrPtr)->Elements.size()) {
                            (*arrPtr)->Elements[op.ElemIndex] = newName;
                            if (PatchArrayLive(op.Object, *table, op.RowIndex, op.PropIndex, message)) {
                                m_applyStatus[op.Object] = message;
                            } else {
                                m_applyStatus[op.Object] = "patch FAILED: " + message;
                            }
                        }
                    } else if (std::get_if<FName>(&row.Values[op.PropIndex])) {
                        if (PatchScalarLive(op.Object, *table, op.RowIndex, op.PropIndex,
                                            PropertyValue(newName), message)) {
                            m_applyStatus[op.Object] = message;
                        } else {
                            m_applyStatus[op.Object] = "patch FAILED: " + message;
                        }
                    }
                    break;
                }
                case PendingOp::Kind::AddRow: {
                    if (!table) break;
                    auto& rows = table->GetRowsMutable();
                    const auto& schema = table->GetSchema();
                    bool exists = false;
                    for (const auto& r : rows) {
                        if (to_utf8_string(r.RowName.ToString()) == op.Text) { exists = true; break; }
                    }
                    if (exists) {
                        m_applyStatus[op.Object] = "row '" + op.Text + "' already exists";
                        break;
                    }
                    MMARow newRow;
                    newRow.RowName = FName(to_generic_string(op.Text), Unreal::FNAME_Add);
                    if (!op.CloneFrom.empty()) {
                        for (const auto& r : rows) {
                            if (to_utf8_string(r.RowName.ToString()) == op.CloneFrom) {
                                newRow.Values = CloneRowValues(r.Values);
                                break;
                            }
                        }
                        if (newRow.Values.empty()) {
                            m_applyStatus[op.Object] = "clone source '" + op.CloneFrom + "' not found";
                            break;
                        }
                    } else {
                        for (const auto& prop : schema) {
                            newRow.Values.push_back(DefaultValue(prop));
                        }
                    }
                    int32_t maxPriority = 0;
                    for (const auto& r : rows) maxPriority = std::max(maxPriority, r.Priority);
                    newRow.Priority = maxPriority + 1;
                    rows.push_back(std::move(newRow));
                    m_dirtyTables.insert(op.Object);
                    m_applyStatus[op.Object] = "added row '" + op.Text + "' (Apply Changes to commit)";
                    break;
                }
                case PendingOp::Kind::CloneDO:
                case PendingOp::Kind::NewEmptyDO: {
                    Unreal::UObject* created = CreateDataObject(op.Object, op.Text,
                                                                op.OpKind == PendingOp::Kind::CloneDO, message);
                    m_applyStatus[op.Object] = message;
                    if (created) {
                        m_needsRefresh = true;
                        m_selectedObject = created;
                        m_applyStatus[created] = message;
                    }
                    break;
                }
                case PendingOp::Kind::Apply: {
                    if (!table) {
                        m_applyStatus[op.Object] = "nothing to apply";
                        break;
                    }
                    if (RebuildAndSwap(op.Object, *table, message)) {
                        m_applyStatus[op.Object] = "applied: " + message;
                    } else {
                        m_applyStatus[op.Object] = "apply FAILED: " + message;
                    }
                    break;
                }
                case PendingOp::Kind::Restore: {
                    if (RestoreOriginal(op.Object)) {
                        m_applyStatus[op.Object] = "root headers restored (value patches are not reverted)";
                    } else {
                        m_applyStatus[op.Object] = "no saved root headers for this table";
                    }
                    break;
                }
                case PendingOp::Kind::ReloadJson: {
                    m_reloadStatus = ReloadDataMods();
                    break;
                }
            }
        }
    }

    void FF7MMAMod::AutoDumpTick() {
        auto objects = FindAllMMAObjects();

        for (auto* obj : objects) {
            if (!obj) continue;
            std::string name = to_utf8_string(obj->GetName());
            std::string lowerName = ToLower(name);
            m_seen.insert(name);

            ApplyTableMods(obj, name, lowerName);

            // Schema catalog: cheap schema-only parse of every data table once
            if (!s_schemaCatalog.count(name) && IsDataObject(obj)) {
                MMAFields schemaFields = ReadMMAFields(obj);
                if (schemaFields.Content && schemaFields.Layout && schemaFields.FrozenContentSize) {
                    MMADataTable schemaTable;
                    if (schemaTable.ParseFromLayout(schemaFields.Content, schemaFields.FrozenContentSize,
                                                    schemaFields.Layout, true)) {
                        auto& props = s_schemaCatalog[name];
                        for (const auto& prop : schemaTable.GetSchema()) {
                            props.push_back({to_utf8_string(prop.Name.ToString()),
                                             PropertyTypeName(prop.Type), prop.bIsArray});
                        }
                        s_schemaCatalogDirty = true;
                    }
                }
            }

            // Dumping is opt-in: nothing is written without a targets.txt entry.
            // A "*" line dumps every data table (creator use).
            if (m_dumped.count(name) || m_failed.count(name)) continue;
            if (m_targets.empty()) continue;
            bool dumpAll = m_targets.count("*") != 0;
            if (!dumpAll && !m_targets.count(lowerName)) continue;
            if (dumpAll && !IsDataObject(obj)) continue;

            MMAFields fields = ReadMMAFields(obj);
            if (!fields.Content || !fields.Layout || fields.FrozenContentSize == 0) {
                // Not loaded yet (or fields not located); log one raw dump for diagnosis
                static bool s_dumpedRaw = false;
                if (!s_dumpedRaw && s_layoutFieldOffset == 0) {
                    s_dumpedRaw = true;
                    uint8_t* base = reinterpret_cast<uint8_t*>(obj);
                    std::wstring hex;
                    for (size_t i = 0x20; i < 0x58 && IsReadable(base + i, 1); ++i) {
                        wchar_t buf[8];
                        swprintf(buf, 8, L"%02X ", base[i]);
                        hex += buf;
                        if ((i - 0x1F) % 16 == 0) hex += L"| ";
                    }
                    Output::send<LogLevel::Default>(STR("[FF7MMAMod] {} raw bytes 0x20-0x58: {}\n"),
                                                    obj->GetName(), hex);
                }
                continue;
            }

            uint8_t* parseContent = fields.Content;
            size_t parseSize = fields.FrozenContentSize;
            GetParseSource(obj, parseContent, parseSize);

            Output::send<LogLevel::Default>(STR("[FF7MMAMod] Dumping {} (content size {})\n"),
                                            obj->GetName(), parseSize);

            auto table = std::make_unique<MMADataTable>();
            bool parsed = false;
            try {
                parsed = table->ParseFromLayout(parseContent, parseSize, fields.Layout);
            } catch (const std::exception& e) {
                m_failed[name] = e.what();
                continue;
            } catch (...) {
                m_failed[name] = "unknown exception";
                continue;
            }

            if (!parsed) {
                m_failed[name] = table->GetError();
                continue;
            }

            std::string fileName = SanitizeFileName(name) + ".json";
            auto filePath = m_dumpDir / fileName;
            if (table->ExportToJsonFile(filePath, name)) {
                std::error_code tec;
                std::filesystem::create_directories(m_dumpDir / "templates", tec);
                WriteTemplateFile(m_dumpDir / "templates" / (SanitizeFileName(name) + ".template.json"),
                                  name, *table);
                m_dumped[name] = fileName;
                Output::send<LogLevel::Default>(STR("[FF7MMAMod] Dumped {} rows to {}\n"),
                                                table->GetRows().size(),
                                                to_generic_string(fileName));
            } else {
                m_failed[name] = "failed to write " + fileName;
            }
        }

        // "$create" tables: mod entries for objects that do not exist get created
        // from a named source object once that source is loaded. Deferred past the
        // first seconds of boot so object construction happens on a settled engine.
        bool createReady = std::chrono::steady_clock::now() - m_startTime >= std::chrono::seconds(5);
        for (auto& [lowerName, mods] : s_tableMods) {
            if (!createReady) break;
            if (s_appliedTables.count(lowerName)) continue;
            auto createIt = mods.find("$create");
            if (createIt == mods.end() || !createIt->second.holds<std::string>()) continue;
            std::string sourceName = createIt->second.get<std::string>();
            Unreal::UObject* sourceObj = nullptr;
            for (auto* candidate : objects) {
                if (candidate && ToLower(to_utf8_string(candidate->GetName())) == NormalizeTableKey(sourceName)) {
                    sourceObj = candidate;
                    break;
                }
            }
            if (!sourceObj) continue; // source not loaded yet
            std::string displayName = s_tableModNames.count(lowerName) ? s_tableModNames[lowerName] : lowerName;
            std::string message;
            Unreal::UObject* created = CreateDataObject(sourceObj, displayName, false, message);
            if (!created) {
                s_appliedTables.insert(lowerName);
                s_modResults[displayName] = "create failed: " + message;
                continue;
            }
            ApplyTableMods(created, displayName, lowerName);
            if (s_modResults.count(displayName)) {
                s_modResults[displayName] = "created from " + sourceName + "; " + s_modResults[displayName];
            }
        }

        WriteStatus();

        if (s_schemaCatalogDirty) {
            s_schemaCatalogDirty = false;
            std::ofstream out(m_dumpDir / "schemas.json", std::ios::binary | std::ios::trunc);
            if (out.is_open()) {
                out << "{";
                bool firstTable = true;
                for (const auto& [tableName, props] : s_schemaCatalog) {
                    out << (firstTable ? "\n" : ",\n") << " \"" << EscapeJson(tableName) << "\": [";
                    firstTable = false;
                    for (size_t i = 0; i < props.size(); ++i) {
                        if (i) out << ", ";
                        out << "{\"name\": \"" << EscapeJson(props[i].Name) << "\", \"type\": \""
                            << props[i].Type << "\", \"isArray\": " << (props[i].bIsArray ? "true" : "false") << "}";
                    }
                    out << "]";
                }
                out << "\n}\n";
            }
        }

        // With explicit targets, finish once every target is dumped or failed
        // and every table that has JSON mods staged has had them applied
        if (!m_targets.empty()) {
            size_t resolved = 0;
            size_t explicitTargets = 0;
            for (const auto& t : m_targets) {
                if (t == "*") continue; // dump-all never completes on its own
                ++explicitTargets;
                for (const auto& [name, _] : m_dumped) {
                    if (ToLower(name) == t) { ++resolved; break; }
                }
                for (const auto& [name, _] : m_failed) {
                    if (ToLower(name) == t) { ++resolved; break; }
                }
            }
            bool modsPending = false;
            for (const auto& [tableName, _] : s_tableMods) {
                if (!s_appliedTables.count(tableName)) { modsPending = true; break; }
            }
            if (explicitTargets > 0 && resolved >= explicitTargets &&
                !m_targets.count("*") && !modsPending) {
                std::ofstream marker(m_dumpDir / "COMPLETE");
                marker << "done\n";
                m_autoDumpDone = true;
                Output::send<LogLevel::Default>(STR("[FF7MMAMod] All targets dumped, wrote COMPLETE\n"));
            }
        }
    }

    void FF7MMAMod::WriteStatus() {
        std::ofstream status(m_dumpDir / "status.json", std::ios::trunc);
        if (!status.is_open()) return;

        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - m_startTime).count();

        status << "{\n\"elapsedSeconds\": " << elapsed << ",\n\"seen\": [";
        bool first = true;
        for (const auto& name : m_seen) {
            if (!first) status << ", ";
            status << '"' << EscapeJson(name) << '"';
            first = false;
        }
        status << "],\n\"dumped\": {";
        first = true;
        for (const auto& [name, file] : m_dumped) {
            if (!first) status << ", ";
            status << '"' << EscapeJson(name) << "\": \"" << EscapeJson(file) << '"';
            first = false;
        }
        status << "},\n\"failed\": {";
        first = true;
        for (const auto& [name, error] : m_failed) {
            if (!first) status << ", ";
            status << '"' << EscapeJson(name) << "\": \"" << EscapeJson(error) << '"';
            first = false;
        }
        status << "},\n\"mods\": {";
        first = true;
        for (const auto& [name, result] : s_modResults) {
            if (!first) status << ", ";
            status << '"' << EscapeJson(name) << "\": \"" << EscapeJson(result) << '"';
            first = false;
        }
        status << "}\n}\n";
    }

    void FF7MMAMod::CacheMMABaseClass() {
        if (s_mmaClassSearched) return;
        s_mmaClassSearched = true;

        Unreal::UClass* foundClass = Unreal::UObjectGlobals::StaticFindObject<Unreal::UClass*>(
            nullptr, nullptr, STR("/Script/Engine.MemoryMappedAsset"));

        if (!foundClass) {
            Unreal::UObjectGlobals::ForEachUObject([](Unreal::UObject* object, int32_t, int32_t) {
                if (auto* cls = Unreal::Cast<Unreal::UClass>(object)) {
                    if (cls->GetName() == STR("MemoryMappedAsset")) {
                        s_mmaBaseClass = cls;
                        return LoopAction::Break;
                    }
                }
                return LoopAction::Continue;
            });
        } else {
            s_mmaBaseClass = foundClass;
        }

        if (s_mmaBaseClass) {
            Output::send<LogLevel::Default>(STR("[FF7MMAMod] Found MemoryMappedAsset class\n"));
        } else {
            Output::send<LogLevel::Warning>(STR("[FF7MMAMod] MemoryMappedAsset class not found, using name matching\n"));
        }

    }

    bool FF7MMAMod::IsDataObject(Unreal::UObject* object) {
        if (!object) return false;
        if (!s_dataObjectBaseClass) {
            // Looked up lazily: the EndDataObject script package is not loaded at unreal-init
            s_dataObjectBaseClass = Unreal::UObjectGlobals::StaticFindObject<Unreal::UClass*>(
                nullptr, nullptr, STR("/Script/EndDataObject.EndDataObjectBase"));
        }
        if (s_dataObjectBaseClass) {
            return object->IsA(s_dataObjectBaseClass);
        }
        // Fallback: the table layouts are all named FEndMemoryMappedDataTable*
        MMAFields fields = ReadMMAFields(object);
        if (fields.Layout && fields.Layout->Name) {
            return std::wstring_view(fields.Layout->Name).find(L"MemoryMappedDataTable") != std::wstring_view::npos;
        }
        return false;
    }

    bool FF7MMAMod::IsMMAObject(Unreal::UObject* object) {
        if (!object) return false;

        // Skip CDOs and archetypes
        if (object->HasAnyFlags(static_cast<Unreal::EObjectFlags>(0x00000010 | 0x00000004))) {
            return false;
        }

        if (s_mmaBaseClass) {
            return object->IsA(s_mmaBaseClass);
        }

        Unreal::UClass* objClass = object->GetClassPrivate();
        if (!objClass) return false;
        for (Unreal::UStruct* cls = objClass; cls; cls = cls->GetSuperStruct()) {
            if (cls->GetName().find(STR("MemoryMapped")) != StringType::npos) {
                return true;
            }
        }
        return false;
    }

    std::vector<Unreal::UObject*> FF7MMAMod::FindAllMMAObjects() {
        std::vector<Unreal::UObject*> result;
        if (!s_mmaClassSearched) {
            CacheMMABaseClass();
        }
        Unreal::UObjectGlobals::ForEachUObject([&](Unreal::UObject* object, int32_t, int32_t) {
            if (IsMMAObject(object)) {
                result.push_back(object);
            }
            return LoopAction::Continue;
        });
        return result;
    }

    FF7MMAMod::MMAFields FF7MMAMod::ReadMMAFields(Unreal::UObject* object) {
        MMAFields fields;
        if (!object) return fields;
        uint8_t* base = reinterpret_cast<uint8_t*>(object);
        if (!IsReadable(base, 0x58)) return fields;

        if (s_layoutFieldOffset != 0) {
            TryReadFieldsAt(base, s_layoutFieldOffset, fields.Layout, fields.SomeBools,
                            fields.FrozenContentSize, fields.Content);
            return fields;
        }

        for (size_t offset = 0x28; offset <= 0x40; offset += 8) {
            if (TryReadFieldsAt(base, offset, fields.Layout, fields.SomeBools,
                                fields.FrozenContentSize, fields.Content)) {
                s_layoutFieldOffset = offset;
                Output::send<LogLevel::Default>(STR("[FF7MMAMod] Located layout field at object offset 0x{:X} (layout '{}', content size {})\n"),
                                                offset,
                                                std::wstring(fields.Layout->Name),
                                                fields.FrozenContentSize);
                return fields;
            }
        }
        return fields;
    }

    MMADataTable* FF7MMAMod::GetParsedTable(Unreal::UObject* mmaObject) {
        if (!mmaObject) return nullptr;
        if (!IsDataObject(mmaObject)) return nullptr;

        auto it = m_parsedTables.find(mmaObject);
        if (it != m_parsedTables.end()) {
            return it->second.get();
        }

        MMAFields fields = ReadMMAFields(mmaObject);
        if (!fields.Content || !fields.Layout || fields.FrozenContentSize == 0) {
            return nullptr;
        }
        uint8_t* parseContent = fields.Content;
        size_t parseSize = fields.FrozenContentSize;
        GetParseSource(mmaObject, parseContent, parseSize);

        auto table = std::make_unique<MMADataTable>();
        if (!table->ParseFromLayout(parseContent, parseSize, fields.Layout)) {
            Output::send<LogLevel::Warning>(STR("[FF7MMAMod] Parse failed for {}: {}\n"),
                                            mmaObject->GetName(),
                                            to_generic_string(table->GetError()));
            return nullptr;
        }

        MMADataTable* result = table.get();
        m_parsedTables[mmaObject] = std::move(table);
        s_pristineRows.emplace(mmaObject, DeepCopyRows(result->GetRows()));
        return result;
    }

    // ---------------- UI ----------------

    void FF7MMAMod::RenderUI() {
        if (s_mmaBaseClass) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "MemoryMappedAsset class found");
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "MemoryMappedAsset class not found");
        }
        ImGui::SameLine();
        ImGui::Text("| dumped %zu, failed %zu", m_dumped.size(), m_failed.size());

        if (ImGui::Button("Refresh")) {
            m_needsRefresh = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Reload JSON")) {
            PendingOp op;
            op.OpKind = PendingOp::Kind::ReloadJson;
            QueueOp(std::move(op));
            m_reloadStatus = "reload queued...";
        }
        if (!m_reloadStatus.empty()) {
            ImGui::SameLine();
            ImGui::TextUnformatted(m_reloadStatus.c_str());
        }
        if (m_needsRefresh) {
            m_mmaObjects.clear();
            for (auto* obj : FindAllMMAObjects()) {
                if (!obj) continue;
                m_mmaObjects.push_back({obj, to_utf8_string(obj->GetName()), IsDataObject(obj)});
            }
            m_selectedObject = nullptr; // stale after any refresh
            m_needsRefresh = false;
        }
        ImGui::SameLine();
        ImGui::Text("%zu MMA objects", m_mmaObjects.size());

        ImGui::Separator();
        ImGui::InputText("Search", m_searchBuffer, sizeof(m_searchBuffer));
        ImGui::SameLine();
        ImGui::Checkbox("Show all MMA", &m_showAllMMA);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Also list non-table memory mapped assets (animations etc); those do not parse");
        }

        ImGui::BeginChild("ObjectList", ImVec2(300, 0), true);
        for (const auto& entry : m_mmaObjects) {
            if (!entry.Object) continue;
            if (!m_showAllMMA && !entry.bIsDataObject) {
                continue;
            }
            if (m_searchBuffer[0] != '\0' && entry.Name.find(m_searchBuffer) == std::string::npos) {
                continue;
            }
            if (ImGui::Selectable(entry.Name.c_str(), entry.Object == m_selectedObject)) {
                m_selectedObject = entry.Object;
            }
        }
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild("Details", ImVec2(0, 0), true);
        std::scoped_lock detailsLock(m_opMutex);
        if (m_selectedObject) {
            std::string objName = to_utf8_string(m_selectedObject->GetName());
            std::string className = to_utf8_string(m_selectedObject->GetClassPrivate()->GetName());
            ImGui::Text("%s (%s) at %p", objName.c_str(), className.c_str(), m_selectedObject);

            MMAFields fields = ReadMMAFields(m_selectedObject);
            ImGui::Text("Content: %p, size %u, flags 0x%08X",
                        static_cast<void*>(fields.Content), fields.FrozenContentSize, fields.SomeBools);
            if (fields.Layout && fields.Layout->Name) {
                std::string layoutName = to_utf8_string(std::wstring(fields.Layout->Name));
                ImGui::Text("Layout: %s (size %u, align %u)",
                            layoutName.c_str(), fields.Layout->Size, fields.Layout->Alignment);
            }

            ImGui::Separator();

            MMADataTable* table = GetParsedTable(m_selectedObject);
            if (table) {
                if (ImGui::Button("Re-parse")) {
                    m_parsedTables.erase(m_selectedObject);
                    m_dirtyTables.erase(m_selectedObject);
                    table = GetParsedTable(m_selectedObject);
                }
                ImGui::SameLine();
                if (ImGui::Button("Export JSON")) {
                    std::string fileName = SanitizeFileName(objName) + ".json";
                    table->ExportToJsonFile(m_dumpDir / fileName, objName);
                    m_applyStatus[m_selectedObject] = "dump written: " + fileName;
                }
                ImGui::SameLine();
                if (ImGui::Button("Export Template")) {
                    std::error_code tec;
                    std::filesystem::create_directories(m_dumpDir / "templates", tec);
                    auto path = m_dumpDir / "templates" / (SanitizeFileName(objName) + ".template.json");
                    if (WriteTemplateFile(path, objName, *table)) {
                        m_applyStatus[m_selectedObject] = "template written: " + to_utf8_string(path.wstring());
                    } else {
                        m_applyStatus[m_selectedObject] = "template write failed";
                    }
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Starter data-mod JSON with the schema and one example row");
                }
                ImGui::SameLine();
                if (ImGui::Button("Export Mod JSON")) {
                    auto pristineIt = s_pristineRows.find(m_selectedObject);
                    std::string written = pristineIt != s_pristineRows.end()
                        ? ExportSessionDiff(m_dumpDir, objName, *table, pristineIt->second)
                        : std::string();
                    m_applyStatus[m_selectedObject] = written.empty()
                        ? "no session edits to export"
                        : "mod json written: " + written;
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Data-mod JSON containing only what you changed this session; copy it into Mods/FF7MMAMod/data/");
                }
                if (table) {
                    RenderTableView(m_selectedObject, table);
                }
            } else {
                ImGui::TextColored(ImVec4(0.9f, 0.4f, 0.4f, 1.0f), "Parse failed or content not loaded");
            }
        } else {
            ImGui::Text("Select an MMA object");
        }
        ImGui::EndChild();
    }

    namespace {
        // Deep-copies row values so cloned rows don't share PropertyArray storage
        std::vector<PropertyValue> CloneRowValues(const std::vector<PropertyValue>& values) {
            std::vector<PropertyValue> out;
            out.reserve(values.size());
            for (const auto& v : values) {
                if (auto* arr = std::get_if<std::shared_ptr<PropertyArray>>(&v)) {
                    auto copy = std::make_shared<PropertyArray>();
                    if (*arr) *copy = **arr;
                    out.push_back(copy);
                } else {
                    out.push_back(v);
                }
            }
            return out;
        }

        enum class EditResult { None, Edited, NameCommit };

        // ImGui editor for one scalar property value. Numeric/string/bool edits are
        // applied in place; FName commits are reported so the caller can queue them
        // for the game thread (FName registration is not render-thread safe).
        EditResult RenderValueEditor(const char* label, EFF7PropertyType type, PropertyValue& value,
                                     std::string& outNameText) {
            switch (type) {
                case EFF7PropertyType::BoolProperty: {
                    auto* v = std::get_if<bool>(&value);
                    return (v && ImGui::Checkbox(label, v)) ? EditResult::Edited : EditResult::None;
                }
                case EFF7PropertyType::ByteProperty: {
                    auto* v = std::get_if<uint8_t>(&value);
                    return (v && ImGui::InputScalar(label, ImGuiDataType_U8, v)) ? EditResult::Edited : EditResult::None;
                }
                case EFF7PropertyType::Int8Property: {
                    auto* v = std::get_if<int8_t>(&value);
                    return (v && ImGui::InputScalar(label, ImGuiDataType_S8, v)) ? EditResult::Edited : EditResult::None;
                }
                case EFF7PropertyType::UInt16Property: {
                    auto* v = std::get_if<uint16_t>(&value);
                    return (v && ImGui::InputScalar(label, ImGuiDataType_U16, v)) ? EditResult::Edited : EditResult::None;
                }
                case EFF7PropertyType::Int16Property: {
                    auto* v = std::get_if<int16_t>(&value);
                    return (v && ImGui::InputScalar(label, ImGuiDataType_S16, v)) ? EditResult::Edited : EditResult::None;
                }
                case EFF7PropertyType::UIntProperty: {
                    auto* v = std::get_if<uint32_t>(&value);
                    return (v && ImGui::InputScalar(label, ImGuiDataType_U32, v)) ? EditResult::Edited : EditResult::None;
                }
                case EFF7PropertyType::IntProperty: {
                    auto* v = std::get_if<int32_t>(&value);
                    return (v && ImGui::InputScalar(label, ImGuiDataType_S32, v)) ? EditResult::Edited : EditResult::None;
                }
                case EFF7PropertyType::Int64Property: {
                    auto* v = std::get_if<int64_t>(&value);
                    return (v && ImGui::InputScalar(label, ImGuiDataType_S64, v)) ? EditResult::Edited : EditResult::None;
                }
                case EFF7PropertyType::FloatProperty: {
                    auto* v = std::get_if<float>(&value);
                    return (v && ImGui::InputFloat(label, v, 0.0f, 0.0f, "%.6f")) ? EditResult::Edited : EditResult::None;
                }
                case EFF7PropertyType::StrProperty: {
                    auto* v = std::get_if<StringType>(&value);
                    if (!v) return EditResult::None;
                    std::string utf8 = to_utf8_string(*v);
                    char buf[512];
                    snprintf(buf, sizeof(buf), "%s", utf8.c_str());
                    if (ImGui::InputText(label, buf, sizeof(buf))) {
                        *v = to_generic_string(std::string(buf));
                        return EditResult::Edited;
                    }
                    return EditResult::None;
                }
                case EFF7PropertyType::NameProperty: {
                    // FNames only register on commit, so the in-progress text lives in a
                    // single static edit buffer keyed by the active widget id
                    auto* v = std::get_if<FName>(&value);
                    if (!v) return EditResult::None;
                    static ImGuiID s_activeNameEdit = 0;
                    static char s_nameEditBuf[256];
                    ImGuiID id = ImGui::GetID(label);
                    std::string current = to_utf8_string(v->ToString());
                    char buf[256];
                    snprintf(buf, sizeof(buf), "%s", s_activeNameEdit == id ? s_nameEditBuf : current.c_str());
                    if (ImGui::InputText(label, buf, sizeof(buf))) {
                        s_activeNameEdit = id;
                        snprintf(s_nameEditBuf, sizeof(s_nameEditBuf), "%s", buf);
                    }
                    if (ImGui::IsItemDeactivatedAfterEdit() && s_activeNameEdit == id) {
                        outNameText = s_nameEditBuf;
                        s_activeNameEdit = 0;
                        return EditResult::NameCommit;
                    }
                    if (ImGui::IsItemDeactivated() && s_activeNameEdit == id) {
                        s_activeNameEdit = 0;
                    }
                    return EditResult::None;
                }
                default:
                    ImGui::Text("%s: <unsupported type %d>", label, static_cast<int>(type));
                    return EditResult::None;
            }
        }
    }

    void FF7MMAMod::RenderTableView(Unreal::UObject* obj, MMADataTable* table) {
        const auto& schema = table->GetSchema();
        auto& rows = table->GetRowsMutable();
        bool dirty = m_dirtyTables.count(obj) != 0;

        ImGui::Text("Schema: %zu properties, Rows: %zu, Warnings: %zu",
                    schema.size(), rows.size(), table->GetWarnings().size());
        if (dirty) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "[modified]");
        }

        // Apply / discard / restore. Apply and restore run on the game thread at a
        // frame boundary so the game is never mid-read during the content swap.
        if (ImGui::Button("Apply Changes")) {
            PendingOp op;
            op.OpKind = PendingOp::Kind::Apply;
            op.Object = obj;
            QueueOp(std::move(op));
            m_applyStatus[obj] = "apply queued...";
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Rebuild the frozen image with the edits and swap it into the asset");
        }
        ImGui::SameLine();
        if (ImGui::Button("Discard Edits")) {
            m_dirtyTables.erase(obj);
            m_parsedTables.erase(obj);
            return;
        }
        if (s_savedRoots.count(obj)) {
            ImGui::SameLine();
            if (ImGui::Button("Restore Original")) {
                PendingOp op;
                op.OpKind = PendingOp::Kind::Restore;
                op.Object = obj;
                QueueOp(std::move(op));
                m_applyStatus[obj] = "restore queued...";
            }
        }
        auto statusIt = m_applyStatus.find(obj);
        if (statusIt != m_applyStatus.end()) {
            ImGui::TextWrapped("%s", statusIt->second.c_str());
        }

        // Row creation
        ImGui::SetNextItemWidth(220);
        ImGui::InputText("##newrow", m_newRowName, sizeof(m_newRowName));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(220);
        ImGui::InputText("##clonesrc", m_cloneSource, sizeof(m_cloneSource));
        ImGui::SameLine();
        if (ImGui::Button("Add Row") && m_newRowName[0] != '\0') {
            PendingOp op;
            op.OpKind = PendingOp::Kind::AddRow;
            op.Object = obj;
            op.Text = m_newRowName;
            op.CloneFrom = m_cloneSource;
            QueueOp(std::move(op));
            m_applyStatus[obj] = "add row queued...";
        }
        ImGui::SameLine();
        ImGui::TextDisabled("name / clone-from");

        // New data object of this table's class
        ImGui::SetNextItemWidth(220);
        ImGui::InputText("##newdo", m_newDOName, sizeof(m_newDOName));
        ImGui::SameLine();
        if (ImGui::Button("Clone DO") && m_newDOName[0] != '\0') {
            PendingOp op;
            op.OpKind = PendingOp::Kind::CloneDO;
            op.Object = obj;
            op.Text = m_newDOName;
            QueueOp(std::move(op));
            m_applyStatus[obj] = "clone data object queued...";
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("New data object of the same class with a copy of all rows");
        }
        ImGui::SameLine();
        if (ImGui::Button("New Empty DO") && m_newDOName[0] != '\0') {
            PendingOp op;
            op.OpKind = PendingOp::Kind::NewEmptyDO;
            op.Object = obj;
            op.Text = m_newDOName;
            QueueOp(std::move(op));
            m_applyStatus[obj] = "new data object queued...";
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("New data object of the same class with this schema and no rows");
        }
        ImGui::SameLine();
        ImGui::TextDisabled("new DO name");

        if (!table->GetWarnings().empty() && ImGui::CollapsingHeader("Warnings")) {
            for (const auto& warning : table->GetWarnings()) {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s", warning.c_str());
            }
        }

        if (ImGui::CollapsingHeader("Schema")) {
            for (const auto& prop : schema) {
                std::string propName = to_utf8_string(prop.Name.ToString());
                ImGui::Text("  %s : %s%s", propName.c_str(), PropertyTypeName(prop.Type),
                            prop.bIsArray ? " (Array)" : "");
            }
        }

        if (ImGui::CollapsingHeader("Rows", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::InputText("Filter", m_rowFilter, sizeof(m_rowFilter));

            int shown = 0;
            int deleteRowAt = -1;
            for (size_t ri = 0; ri < rows.size(); ++ri) {
                MMARow& row = rows[ri];
                if (shown >= 500) {
                    ImGui::TextDisabled("... (more rows hidden, use filter)");
                    break;
                }
                std::string rowName = to_utf8_string(row.RowName.ToString());
                if (m_rowFilter[0] != '\0' && rowName.find(m_rowFilter) == std::string::npos) {
                    continue;
                }
                ++shown;
                ImGui::PushID(static_cast<int>(ri));
                if (ImGui::TreeNode(rowName.c_str())) {
                    ImGui::Text("Index: %d, Priority: %d, Slot: %d", row.Index, row.Priority, row.SlotIndex);
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Delete Row")) {
                        deleteRowAt = static_cast<int>(ri);
                    }

                    for (size_t i = 0; i < schema.size() && i < row.Values.size(); ++i) {
                        const auto& prop = schema[i];
                        std::string propName = to_utf8_string(prop.Name.ToString());
                        ImGui::PushID(static_cast<int>(i));

                        if (prop.bIsArray) {
                            auto* arrPtr = std::get_if<std::shared_ptr<PropertyArray>>(&row.Values[i]);
                            auto arr = arrPtr ? *arrPtr : nullptr;
                            std::string header = propName + " [" +
                                                 std::to_string(arr ? arr->Elements.size() : 0) + "]";
                            if (ImGui::TreeNode(header.c_str())) {
                                if (!arr) {
                                    arr = std::make_shared<PropertyArray>();
                                    arr->ElementType = prop.Type;
                                    row.Values[i] = arr;
                                }
                                int removeAt = -1;
                                for (size_t ai = 0; ai < arr->Elements.size(); ++ai) {
                                    ImGui::PushID(static_cast<int>(ai));
                                    std::string elemLabel = "[" + std::to_string(ai) + "]";
                                    std::string nameText;
                                    EditResult res = RenderValueEditor(elemLabel.c_str(), prop.Type, arr->Elements[ai], nameText);
                                    if (res == EditResult::Edited) {
                                        PendingOp op;
                                        op.OpKind = PendingOp::Kind::PatchArray;
                                        op.Object = obj;
                                        op.RowIndex = ri;
                                        op.PropIndex = i;
                                        QueueOp(std::move(op));
                                    } else if (res == EditResult::NameCommit) {
                                        PendingOp op;
                                        op.OpKind = PendingOp::Kind::NameEdit;
                                        op.Object = obj;
                                        op.RowIndex = ri;
                                        op.PropIndex = i;
                                        op.ElemIndex = ai;
                                        op.Text = nameText;
                                        QueueOp(std::move(op));
                                    }
                                    ImGui::SameLine();
                                    if (ImGui::SmallButton("X")) {
                                        removeAt = static_cast<int>(ai);
                                    }
                                    ImGui::PopID();
                                }
                                bool arrayChanged = false;
                                if (removeAt >= 0) {
                                    arr->Elements.erase(arr->Elements.begin() + removeAt);
                                    arrayChanged = true;
                                }
                                if (ImGui::SmallButton("+ Add Element")) {
                                    FFF7Property scalarProp = prop;
                                    scalarProp.bIsArray = false;
                                    arr->Elements.push_back(DefaultValue(scalarProp));
                                    arrayChanged = true;
                                }
                                if (arrayChanged) {
                                    PendingOp op;
                                    op.OpKind = PendingOp::Kind::PatchArray;
                                    op.Object = obj;
                                    op.RowIndex = ri;
                                    op.PropIndex = i;
                                    QueueOp(std::move(op));
                                }
                                ImGui::TreePop();
                            }
                        } else {
                            std::string nameText;
                            EditResult res = RenderValueEditor(propName.c_str(), prop.Type, row.Values[i], nameText);
                            if (res == EditResult::Edited) {
                                PendingOp op;
                                op.OpKind = prop.Type == EFF7PropertyType::StrProperty
                                                ? PendingOp::Kind::PatchString
                                                : PendingOp::Kind::PatchValue;
                                op.Object = obj;
                                op.RowIndex = ri;
                                op.PropIndex = i;
                                op.Value = row.Values[i];
                                QueueOp(std::move(op));
                            } else if (res == EditResult::NameCommit) {
                                PendingOp op;
                                op.OpKind = PendingOp::Kind::NameEdit;
                                op.Object = obj;
                                op.RowIndex = ri;
                                op.PropIndex = i;
                                op.Text = nameText;
                                QueueOp(std::move(op));
                            }
                        }

                        ImGui::PopID();
                    }
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }

            if (deleteRowAt >= 0) {
                rows.erase(rows.begin() + deleteRowAt);
                m_dirtyTables.insert(obj);
            }
        }
    }

} // namespace RC::FF7MMA

#define FF7MMA_MOD_API __declspec(dllexport)

extern "C" {
    FF7MMA_MOD_API RC::CppUserModBase* start_mod() {
        return new RC::FF7MMA::FF7MMAMod();
    }

    FF7MMA_MOD_API void uninstall_mod(RC::CppUserModBase* mod) {
        delete mod;
    }
}
