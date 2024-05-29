#include <vector>

// writer.h is missing includes and I'd like to keep it unchanged so I'll include the missing files here instead.
#include <Unreal/Common.hpp>
#include <sstream>
#include <string>

#include <DynamicOutput/DynamicOutput.hpp>
#include <USMapGenerator/Generator.hpp>
#include <USMapGenerator/writer.h>
#include <Unreal/NameTypes.hpp>
#include <Unreal/Property/FSetProperty.hpp>
#include <Unreal/Property/FArrayProperty.hpp>
#include <Unreal/Property/FEnumProperty.hpp>
#include <Unreal/Property/FMapProperty.hpp>
#include <Unreal/Property/NumericPropertyTypes.hpp>
#include <Unreal/UClass.hpp>
#include <Unreal/UEnum.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/UScriptStruct.hpp>
#include <Unreal/UnrealVersion.hpp>

#include "UE4SSProgram.hpp"

namespace RC::OutTheShade
{
    using namespace ::RC::Unreal;

    enum class EPropertyType : uint8_t
    {
        ByteProperty,
        BoolProperty,
        IntProperty,
        FloatProperty,
        ObjectProperty,
        NameProperty,
        DelegateProperty,
        DoubleProperty,
        ArrayProperty,
        StructProperty,
        StrProperty,
        TextProperty,
        InterfaceProperty,
        MulticastDelegateProperty,
        WeakObjectProperty,
        LazyObjectProperty,
        AssetObjectProperty,
        SoftObjectProperty,
        UInt64Property,
        UInt32Property,
        UInt16Property,
        Int64Property,
        Int16Property,
        Int8Property,
        MapProperty,
        SetProperty,
        EnumProperty,
        FieldPathProperty,
        EnumAsByteProperty,

        Unknown = 0xFF
    };

    static EPropertyType GetPropertyType(FProperty* Prop)
    {
        auto Class = Prop->GetClass();
        if (Class.HasAnyCastFlags(CASTCLASS_FObjectProperty | CASTCLASS_FClassProperty | CASTCLASS_FObjectPtrProperty | CASTCLASS_FClassPtrProperty))
        {
            return EPropertyType::ObjectProperty;
        }
        else if (Class.HasAnyCastFlags(CASTCLASS_FStructProperty))
        {
            return EPropertyType::StructProperty;
        }
        else if (Class.HasAnyCastFlags(CASTCLASS_FInt8Property))
        {
            return EPropertyType::Int8Property;
        }
        else if (Class.HasAnyCastFlags(CASTCLASS_FInt16Property))
        {
            return EPropertyType::Int16Property;
        }
        else if (Class.HasAnyCastFlags(CASTCLASS_FIntProperty))
        {
            return EPropertyType::IntProperty;
        }
        else if (Class.HasAnyCastFlags(CASTCLASS_FInt64Property))
        {
            return EPropertyType::Int64Property;
        }
        else if (Class.HasAnyCastFlags(CASTCLASS_FUInt16Property))
        {
            return EPropertyType::UInt16Property;
        }
        else if (Class.HasAnyCastFlags(CASTCLASS_FUInt32Property))
        {
            return EPropertyType::UInt32Property;
        }
        else if (Class.HasAnyCastFlags(CASTCLASS_FUInt64Property))
        {
            return EPropertyType::UInt64Property;
        }
        else if (Class.HasAnyCastFlags(CASTCLASS_FArrayProperty))
        {
            return EPropertyType::ArrayProperty;
        }
        else if (Class.HasAnyCastFlags(CASTCLASS_FFloatProperty))
        {
            return EPropertyType::FloatProperty;
        }
        else if (Class.HasAnyCastFlags(CASTCLASS_FDoubleProperty))
        {
            return EPropertyType::DoubleProperty;
        }
        else if (Class.HasAnyCastFlags(CASTCLASS_FBoolProperty))
        {
            return EPropertyType::BoolProperty;
        }
        else if (Class.HasAnyCastFlags(CASTCLASS_FStrProperty))
        {
            return EPropertyType::StrProperty;
        }
        else if (Class.HasAnyCastFlags(CASTCLASS_FNameProperty))
        {
            return EPropertyType::NameProperty;
        }
        else if (Class.HasAnyCastFlags(CASTCLASS_FTextProperty))
        {
            return EPropertyType::TextProperty;
        }
        else if (Class.HasAnyCastFlags(CASTCLASS_FEnumProperty))
        {
            return EPropertyType::EnumProperty;
        }
        else if (Class.HasAnyCastFlags(CASTCLASS_FInterfaceProperty))
        {
            return EPropertyType::InterfaceProperty;
        }
        else if (Class.HasAnyCastFlags(CASTCLASS_FMapProperty))
        {
            return EPropertyType::MapProperty;
        }
        else if (Class.HasAnyCastFlags(CASTCLASS_FByteProperty))
        {
            FByteProperty* ByteProp = static_cast<FByteProperty*>(Prop);
            if (ByteProp->GetEnum()) return EPropertyType::EnumAsByteProperty;
            return EPropertyType::ByteProperty;
        }
        else if (Class.HasAnyCastFlags(CASTCLASS_FMulticastDelegateProperty | CASTCLASS_FMulticastInlineDelegateProperty |
                                       CASTCLASS_FMulticastSparseDelegateProperty))
        {
            return EPropertyType::MulticastDelegateProperty;
        }
        else if (Class.HasAnyCastFlags(CASTCLASS_FDelegateProperty))
        {
            return EPropertyType::DelegateProperty;
        }
        else if (Class.HasAnyCastFlags(CASTCLASS_FSoftObjectProperty | CASTCLASS_FSoftClassProperty))
        {
            return EPropertyType::SoftObjectProperty;
        }
        else if (Class.HasAnyCastFlags(CASTCLASS_FWeakObjectProperty))
        {
            return EPropertyType::WeakObjectProperty;
        }
        else if (Class.HasAnyCastFlags(CASTCLASS_FLazyObjectProperty))
        {
            return EPropertyType::LazyObjectProperty;
        }
        else if (Class.HasAnyCastFlags(CASTCLASS_FSetProperty))
        {
            return EPropertyType::SetProperty;
        }
        else if (Class.HasAnyCastFlags(CASTCLASS_FFieldPathProperty))
        {
            return EPropertyType::FieldPathProperty;
        }
        else
        {
            return EPropertyType::Unknown;
        }
    }

    struct FPropertyData
    {
        FProperty* Prop;
        uint16_t Index;
        uint8_t ArrayDim;
        FName Name;
        EPropertyType PropertyType;

        // Warning: converting 'Idx' from int to uint16.
        // Warning: converting 'GetArrayDim()' from int to uint8.
        FPropertyData(FProperty* P, int Idx)
            : Prop(P), Index(static_cast<uint16_t>(Idx)), ArrayDim(static_cast<uint8_t>(P->GetArrayDim())), Name(P->GetFName()), PropertyType(GetPropertyType(P))
        {
        }
    };

    auto generate_usmap() -> void
    {
        Output::send(STR("Mappings Generator by OutTheShade\nAttempting to dump mappings...\nPort of https://github.com/OutTheShade/UnrealMappingsDumper "
                         "Commit SHA 4da8c66\n"));

        StreamWriter Buffer;
        std::unordered_map<FName, int> NameMap;
        std::unordered_map<UObject*, FName> ModulePathsMap;

        std::vector<UEnum*> Enums;
        std::vector<UStruct*> Structs; // TODO: a better way than making this completely dynamic

        std::function<void(class FProperty*, EPropertyType)> WriteProperty = [&](FProperty* Prop, EPropertyType Type) {
            if (Type == EPropertyType::EnumAsByteProperty)
                Buffer.Write(EPropertyType::EnumProperty);
            else
                Buffer.Write(Type);

            switch (Type)
            {
            case EPropertyType::EnumProperty: {
                auto EnumProp = static_cast<FEnumProperty*>(Prop);

                auto Inner = EnumProp->GetUnderlyingProp();
                auto InnerType = GetPropertyType(Inner);
                WriteProperty(static_cast<FProperty*>(Inner), InnerType);
                Buffer.Write(NameMap[EnumProp->GetEnum()->GetNamePrivate()]);

                break;
            }
            case EPropertyType::EnumAsByteProperty: {
                Buffer.Write(EPropertyType::ByteProperty);
                Buffer.Write(NameMap[static_cast<FByteProperty*>(Prop)->GetEnum()->GetNamePrivate()]);

                break;
            }
            case EPropertyType::StructProperty: {
                Buffer.Write(NameMap[static_cast<FStructProperty*>(Prop)->GetStruct()->GetNamePrivate()]);
                break;
            }
            case EPropertyType::SetProperty: {
                auto Inner = static_cast<FSetProperty*>(Prop)->GetElementProp();
                auto InnerType = GetPropertyType(Inner);
                WriteProperty(Inner, InnerType);

                break;
            }
            case EPropertyType::ArrayProperty: {
                auto Inner = static_cast<FArrayProperty*>(Prop)->GetInner();
                auto InnerType = GetPropertyType(Inner);
                WriteProperty(Inner, InnerType);

                break;
            }
            case EPropertyType::MapProperty: {
                auto Inner = static_cast<FMapProperty*>(Prop)->GetKeyProp();
                auto InnerType = GetPropertyType(Inner);
                WriteProperty(Inner, InnerType);

                auto Value = static_cast<FMapProperty*>(Prop)->GetValueProp();
                auto ValueType = GetPropertyType(Value);
                WriteProperty(Value, ValueType);

                break;
            }
            }
        };

        UObjectGlobals::ForEachUObject([&](UObject* Object, ...) {
            if (Object->GetClassPrivate() == UClass::StaticClass() || Object->GetClassPrivate() == UScriptStruct::StaticClass())
            {
                auto Struct = static_cast<UStruct*>(Object);

                Structs.push_back(Struct);

                NameMap.insert_or_assign(Struct->GetNamePrivate(), 0);

                if (Struct->GetSuperStruct() && !NameMap.contains(Struct->GetSuperStruct()->GetNamePrivate()))
                    NameMap.insert_or_assign(Struct->GetSuperStruct()->GetNamePrivate(), 0);

                for (FProperty* Prop : Struct->ForEachProperty())
                {
                    NameMap.insert_or_assign(Prop->GetFName(), 0);
                }
            }
            else if (Object->GetClassPrivate() == UEnum::StaticClass())
            {
                auto Enum = static_cast<UEnum*>(Object);
                Enums.push_back(Enum);

                NameMap.insert_or_assign(Enum->GetNamePrivate(), 0);

                for (auto& [Key, _] : Enum->ForEachName())
                {
                    NameMap.insert_or_assign(Key, 0);
                }
            }

            if (Object->GetClassPrivate() == UClass::StaticClass() || Object->GetClassPrivate() == UScriptStruct::StaticClass() ||
                Object->GetClassPrivate() == UEnum::StaticClass())
            {
                std::wstring RawPathName = Object->GetPathName();
                std::wstring::size_type PathNameStart =
                        0; // include first bit (Script/Game) to avoid ambiguity; to drop it, replace with RawPathName.find_first_of('/', 1) + 1;
                std::wstring::size_type PathNameLength = RawPathName.find_last_of('.') - PathNameStart;
                std::wstring FinalPathStr = RawPathName.substr(PathNameStart, PathNameLength);
                FName FinalPathName = FName(FinalPathStr);

                NameMap.insert_or_assign(FinalPathName, 0);
                ModulePathsMap.insert_or_assign(Object, FinalPathName);
            }

            return LoopAction::Continue;
        });

        // Warning: Converting size_t (uint64) to int.
        Buffer.Write<int>(static_cast<int>(NameMap.size()));

        int CurrentNameIndex = 0;

        for (auto&& N : NameMap)
        {
            NameMap[N.first] = CurrentNameIndex;

            auto Name = to_string(N.first.ToString());
            std::string_view NameView = Name;

            auto Find = Name.find("::");
            if (Find != std::string::npos)
            {
                NameView = NameView.substr(Find + 2);
            }

            // Warning: Converting size_t (uint64) to uint8_t.
            Buffer.Write<uint8_t>(static_cast<uint8_t>(NameView.length()));
            Buffer.WriteString(NameView);

            CurrentNameIndex++;
        }

        // Warning: Converting size_t (uint64) to uint32_t.
        Buffer.Write<uint32_t>(static_cast<uint32_t>(Enums.size()));

        for (auto Enum : Enums)
        {
            Buffer.Write(NameMap[Enum->GetNamePrivate()]);

            // limit to 255 entries; why is this a byte in the first place?
            uint8_t EnumNameCount{};
            for (auto _ : Enum->ForEachName())
            {
                ++EnumNameCount;
                if (EnumNameCount >= std::numeric_limits<uint8_t>::max()) break;
            }
            Buffer.Write<uint8_t>(EnumNameCount);

            int numSoFar = 0;
            for (auto& [Key, _] : Enum->ForEachName())
            {
                Buffer.Write<uint32_t>(NameMap[Key]);
                if (++numSoFar >= EnumNameCount) break;
            }
        }

        // Warning: Converting size_t (uint64) to uint32_t.
        Buffer.Write<uint32_t>(static_cast<uint32_t>(Structs.size()));

        for (auto Struct : Structs)
        {
            Buffer.Write(NameMap[Struct->GetNamePrivate()]);
            Buffer.Write<int32_t>(Struct->GetSuperStruct() ? NameMap[Struct->GetSuperStruct()->GetNamePrivate()] : static_cast<int32_t>(0xffffffff));

            std::vector<FPropertyData> Properties;

            uint16_t PropCount = 0;
            uint16_t SerializablePropCount = 0;

            for (FProperty* Props : Struct->ForEachProperty())
            {
                FPropertyData Data(Props, PropCount);

                Properties.push_back(Data);

                PropCount += Data.ArrayDim;
                SerializablePropCount++;
            }

            Buffer.Write(PropCount);
            Buffer.Write(SerializablePropCount);

            for (auto P : Properties)
            {
                Buffer.Write<uint16_t>(P.Index);
                Buffer.Write(P.ArrayDim);
                Buffer.Write(NameMap[P.Name]);

                WriteProperty(P.Prop, P.PropertyType);
            }
        }

        // extensions //

        Buffer.Write<uint32_t>(0x54584543); // "CEXT"; magic
        Buffer.Write<uint8_t>(0);           // extensions layout version; 0 (Initial)

        Buffer.Write<uint32_t>(3); // number of extensions, 3 right now

        // extension 1: PPTH (object paths)
        Buffer.Write<uint32_t>(0x48545050); // ext id
        Buffer.Write<uint32_t>(0);          // size; unknown for now

        std::streampos extStartPos = Buffer.GetBuffer().tellp();
        Buffer.Write<uint8_t>(0); // PPTH version; 0
        Buffer.Write<uint32_t>(static_cast<uint32_t>(Enums.size()));
        for (auto Enum : Enums)
        {
            Buffer.Write(NameMap[ModulePathsMap[Enum]]);
        }
        Buffer.Write<uint32_t>(static_cast<uint32_t>(Structs.size()));
        for (auto Struct : Structs)
        {
            Buffer.Write(NameMap[ModulePathsMap[Struct]]);
        }
        std::streampos extEndPos = Buffer.GetBuffer().tellp();

        Buffer.GetBuffer().seekp(extStartPos);
        Buffer.GetBuffer().seekp(-(int32)sizeof(uint32), std::ios_base::cur);
        Buffer.Write<uint32_t>(extEndPos - extStartPos);
        Buffer.GetBuffer().seekp(extEndPos);

        // extension 2: EATR (extended attributes)
        Buffer.Write<uint32_t>(0x52544145); // ext id
        Buffer.Write<uint32_t>(0);          // size; unknown for now

        extStartPos = Buffer.GetBuffer().tellp();
        Buffer.Write<uint8_t>(0); // EATR version; 0
        Buffer.Write<uint32_t>(static_cast<uint32_t>(Enums.size()));
        for (auto Enum : Enums)
        {
            Buffer.Write(static_cast<int32_t>(Enum->GetEnumFlags()));
        }
        Buffer.Write<uint32_t>(static_cast<uint32_t>(Structs.size()));
        for (auto Struct : Structs)
        {
            if (Struct->GetClassPrivate() == UScriptStruct::StaticClass())
            {
                Buffer.Write<uint8_t>(1);
                Buffer.Write<int32_t>(((UScriptStruct*)Struct)->GetStructFlags());
            }
            else if (Struct->GetClassPrivate() == UClass::StaticClass())
            {
                Buffer.Write<uint8_t>(2);
                Buffer.Write<int32_t>(((UClass*)Struct)->GetClassFlags());
            }
            else
            {
                Buffer.Write<uint8_t>(0); // ???
                Buffer.Write<int32_t>(0);
            }

            std::vector<uint64_t> propFlags;
            for (FProperty* Props : Struct->ForEachProperty())
                propFlags.push_back(static_cast<uint64_t>(Props->GetPropertyFlags()));
            Buffer.Write<uint32_t>(static_cast<uint32_t>(propFlags.size()));
            for (uint64_t propFlag : propFlags)
                Buffer.Write<uint64_t>(propFlag);
        }
        extEndPos = Buffer.GetBuffer().tellp();

        Buffer.GetBuffer().seekp(extStartPos);
        Buffer.GetBuffer().seekp(-(int32)sizeof(uint32), std::ios_base::cur);
        Buffer.Write<uint32_t>(extEndPos - extStartPos);
        Buffer.GetBuffer().seekp(extEndPos);

        // extension 3: ENVP (enum name/value pairs)
        Buffer.Write<uint32_t>(0x50564E45); // ext id
        Buffer.Write<uint32_t>(0);          // size; unknown for now

        extStartPos = Buffer.GetBuffer().tellp();
        Buffer.Write<uint8_t>(0); // ENVP version; 0
        Buffer.Write<uint32_t>(static_cast<uint32_t>(Enums.size()));
        for (auto Enum : Enums)
        {
            uint32_t EnumNameCount = 0;
            for (auto _ : Enum->ForEachName())
                ++EnumNameCount;
            Buffer.Write<uint32_t>(EnumNameCount);

            for (auto& [Key, val] : Enum->ForEachName())
            {
                Buffer.Write<uint32_t>(NameMap[Key]);
                Buffer.Write<int64_t>(val);
            }
        }
        extEndPos = Buffer.GetBuffer().tellp();

        Buffer.GetBuffer().seekp(extStartPos);
        Buffer.GetBuffer().seekp(-(int32)sizeof(uint32), std::ios_base::cur);
        Buffer.Write<uint32_t>(extEndPos - extStartPos);
        Buffer.GetBuffer().seekp(extEndPos);

        // end of extensions //

        std::vector<uint8_t> UsmapData;
        std::string UncompressedStream = Buffer.GetBuffer().str();
        UsmapData.resize(UncompressedStream.size());
        memcpy(UsmapData.data(), UncompressedStream.data(), UsmapData.size());

        auto filename = to_string(UE4SSProgram::get_program().get_working_directory()) + "//Mappings.usmap";
        auto FileOutput = FileWriter(filename.c_str());

        FileOutput.Write<uint16_t>(0x30C4); // magic
        FileOutput.Write<uint8_t>(0);       // version
        FileOutput.Write<uint8_t>(0);       // compression
        // Warning: Converting size_t (uint64) to int.
        FileOutput.Write<uint32_t>(static_cast<uint32_t>(UsmapData.size())); // compressed size
        FileOutput.Write<uint32_t>(Buffer.Size());                           // decompressed size

        FileOutput.Write(UsmapData.data(), UsmapData.size());

        Output::send(STR("Mappings Generation Completed Successfully!\n"));
    }
} // namespace RC::OutTheShade
