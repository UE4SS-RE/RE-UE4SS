#define NOMINMAX

#include <stdexcept>
#include <format>
#include <thread>
#include <unordered_set>
#include <numeric>

#include <UVTD/UVTD.hpp>
#include <UVTD/ExceptionHandling.hpp>
#include <DynamicOutput/DynamicOutput.hpp>
#include <Input/Handler.hpp>
#include <Helpers/String.hpp>

#include <Windows.h>
#include <Psapi.h>
#include <dbghelp.h>

#include <atlbase.h>
#include <dia2.h>

namespace RC::UVTD
{
    bool processing_events{false};
    Input::Handler input_handler{L"ConsoleWindowClass", L"UnrealWindow"};
    std::unordered_map<File::StringType, EnumEntry> g_enum_entries{};
    std::unordered_map<File::StringType, Classes> g_class_entries{};

    struct ObjectItem
    {
        File::StringType name{};
        ValidForVTable valid_for_vtable{};
        ValidForMemberVars valid_for_member_vars{};
    };
    // TODO: UConsole isn't found in all PDBs by this tool for some reason. Fix it.
    //       For now, to avoid problems, let's not generate for UConsole.
    static std::vector<ObjectItem> s_object_items{
            {STR("UObjectBase"), ValidForVTable::Yes, ValidForMemberVars::Yes},
            {STR("UObjectBaseUtility"), ValidForVTable::Yes, ValidForMemberVars::No},
            {STR("UObject"), ValidForVTable::Yes, ValidForMemberVars::No},
            {STR("UScriptStruct::ICppStructOps"), ValidForVTable::Yes, ValidForMemberVars::Yes},
            {STR("FOutputDevice"), ValidForVTable::Yes, ValidForMemberVars::Yes},
            {STR("UStruct"), ValidForVTable::Yes, ValidForMemberVars::Yes},
            {STR("UGameViewportClient"), ValidForVTable::Yes, ValidForMemberVars::Yes},
            //{STR("UConsole"), ValidForVTable::Yes, ValidForMemberVars::Yes},
            {STR("FMalloc"), ValidForVTable::Yes, ValidForMemberVars::Yes},
            {STR("FArchive"), ValidForVTable::Yes, ValidForMemberVars::Yes},
            {STR("FArchiveState"), ValidForVTable::Yes, ValidForMemberVars::Yes},
            {STR("AGameModeBase"), ValidForVTable::Yes, ValidForMemberVars::Yes},
            {STR("AGameMode"), ValidForVTable::Yes, ValidForMemberVars::Yes},
            {STR("AActor"), ValidForVTable::Yes, ValidForMemberVars::Yes},
            {STR("AHUD"), ValidForVTable::Yes, ValidForMemberVars::Yes},
            {STR("UPlayer"), ValidForVTable::Yes, ValidForMemberVars::Yes},
            {STR("ULocalPlayer"), ValidForVTable::Yes, ValidForMemberVars::Yes},
            {STR("FExec"), ValidForVTable::Yes, ValidForMemberVars::No},
            {STR("FField"), ValidForVTable::Yes, ValidForMemberVars::Yes},
            {STR("UField"), ValidForVTable::Yes, ValidForMemberVars::Yes},
            {STR("FProperty"), ValidForVTable::Yes, ValidForMemberVars::Yes},
            {STR("UProperty"), ValidForVTable::Yes, ValidForMemberVars::Yes},
            {STR("FNumericProperty"), ValidForVTable::Yes, ValidForMemberVars::Yes},
            {STR("UNumericProperty"), ValidForVTable::Yes, ValidForMemberVars::Yes},
            {STR("FMulticastDelegateProperty"), ValidForVTable::Yes, ValidForMemberVars::Yes},
            {STR("UMulticastDelegateProperty"), ValidForVTable::Yes, ValidForMemberVars::Yes},
            {STR("FObjectPropertyBase"), ValidForVTable::Yes, ValidForMemberVars::Yes},
            {STR("UObjectPropertyBase"), ValidForVTable::Yes, ValidForMemberVars::Yes},
            /*{STR("FConsoleManager"), ValidForVTable::Yes, ValidForMemberVars::Yes},
            {STR("UDataTable"), ValidForVTable::Yes, ValidForMemberVars::Yes},
            {STR("FConsoleVariableBase"), ValidForVTable::Yes, ValidForMemberVars::Yes},
            {STR("FConsoleCommandBase"), ValidForVTable::Yes, ValidForMemberVars::Yes},*/

            {STR("UScriptStruct"), ValidForVTable::No, ValidForMemberVars::Yes},
            {STR("UWorld"), ValidForVTable::No, ValidForMemberVars::Yes},
            {STR("UFunction"), ValidForVTable::No, ValidForMemberVars::Yes},
            {STR("UClass"), ValidForVTable::No, ValidForMemberVars::Yes},
            {STR("UEnum"), ValidForVTable::No, ValidForMemberVars::Yes},
            {STR("FStructProperty"), ValidForVTable::No, ValidForMemberVars::Yes},
            {STR("UStructProperty"), ValidForVTable::No, ValidForMemberVars::Yes},
            {STR("FArrayProperty"), ValidForVTable::No, ValidForMemberVars::Yes},
            {STR("UArrayProperty"), ValidForVTable::No, ValidForMemberVars::Yes},
            {STR("FMapProperty"), ValidForVTable::No, ValidForMemberVars::Yes},
            {STR("UMapProperty"), ValidForVTable::No, ValidForMemberVars::Yes},
            {STR("FBoolProperty"), ValidForVTable::No, ValidForMemberVars::Yes},
            {STR("UBoolProperty"), ValidForVTable::No, ValidForMemberVars::Yes},
            {STR("FByteProperty"), ValidForVTable::No, ValidForMemberVars::Yes},
            {STR("UByteProperty"), ValidForVTable::No, ValidForMemberVars::Yes},
            {STR("FEnumProperty"), ValidForVTable::No, ValidForMemberVars::Yes},
            {STR("UEnumProperty"), ValidForVTable::No, ValidForMemberVars::Yes},
            {STR("FClassProperty"), ValidForVTable::No, ValidForMemberVars::Yes},
            {STR("UClassProperty"), ValidForVTable::No, ValidForMemberVars::Yes},
            {STR("FSoftClassProperty"), ValidForVTable::No, ValidForMemberVars::Yes},
            {STR("USoftClassProperty"), ValidForVTable::No, ValidForMemberVars::Yes},
            {STR("FDelegateProperty"), ValidForVTable::No, ValidForMemberVars::Yes},
            {STR("UDelegateProperty"), ValidForVTable::No, ValidForMemberVars::Yes},
            {STR("FInterfaceProperty"), ValidForVTable::No, ValidForMemberVars::Yes},
            {STR("UInterfaceProperty"), ValidForVTable::No, ValidForMemberVars::Yes},
            {STR("FFieldPathProperty"), ValidForVTable::No, ValidForMemberVars::Yes},
            {STR("FSetProperty"), ValidForVTable::No, ValidForMemberVars::Yes},
            {STR("USetProperty"), ValidForVTable::No, ValidForMemberVars::Yes},
    };

    static std::unordered_set<File::StringType> valid_udt_names{
            STR("UScriptStruct::ICppStructOps"),
            STR("UObjectBase"),
            STR("UObjectBaseUtility"),
            STR("UObject"),
            STR("UStruct"),
            STR("UGameViewportClient"),
            STR("UScriptStruct"),
            STR("FOutputDevice"),
            //STR("UConsole"),
            STR("FMalloc"),
            STR("FArchive"),
            STR("FArchiveState"),
            STR("AGameModeBase"),
            STR("AGameMode"),
            STR("AActor"),
            STR("AHUD"),
            STR("UPlayer"),
            STR("ULocalPlayer"),
            STR("FExec"),
            STR("UField"),
            STR("FField"),
            STR("FProperty"),
            STR("UProperty"),
            STR("FNumericProperty"),
            STR("UNumericProperty"),
            STR("FMulticastDelegateProperty"),
            STR("UMulticastDelegateProperty"),
            STR("FObjectPropertyBase"),
            STR("UObjectPropertyBase"),
            STR("UStructProperty"),
            STR("FStructProperty"),
            STR("UArrayProperty"),
            STR("FArrayProperty"),
            STR("UMapProperty"),
            STR("FMapProperty"),
            STR("UWorld"),
            STR("UFunction"),
            STR("FBoolProperty"),
            STR("UClass"),
            STR("UEnum"),
            STR("UBoolProperty"),
            STR("FByteProperty"),
            STR("UByteProperty"),
            STR("FEnumProperty"),
            STR("UEnumProperty"),
            STR("FClassProperty"),
            STR("UClassProperty"),
            STR("FSoftClassProperty"),
            STR("USoftClassProperty"),
            STR("FDelegateProperty"),
            STR("UDelegateProperty"),
            STR("FInterfaceProperty"),
            STR("UInterfaceProperty"),
            STR("FFieldPathProperty"),
            STR("FSetProperty"),
            STR("USetProperty"),
    };

    static std::vector<File::StringType> UPrefixToFPrefix{
            STR("UProperty"),
            STR("UMulticastDelegateProperty"),
            STR("UObjectPropertyBase"),
            STR("UStructProperty"),
            STR("UArrayProperty"),
            STR("UMapProperty"),
            STR("UBoolProperty"),
            STR("UByteProperty"),
            STR("UNumericProperty"),
            STR("UEnumProperty"),
            STR("UClassProperty"),
            STR("USoftClassProperty"),
            STR("UDelegateProperty"),
            STR("UInterfaceProperty"),
            STR("USetProperty"),
    };

    auto static is_valid_type_to_dump(File::StringType type_name) -> bool
    {
        if (type_name.find(STR("FUnversionedStructSchema")) != type_name.npos ||
            type_name.find(STR("ELifetimeCondition")) != type_name.npos ||
            type_name.find(STR("UAISystemBase")) != type_name.npos ||
            type_name.find(STR("FLevelCollection")) != type_name.npos ||
            type_name.find(STR("FThreadSafeCounter")) != type_name.npos ||
            type_name.find(STR("FWorldAsyncTraceState")) != type_name.npos ||
            type_name.find(STR("FDelegateHandle")) != type_name.npos ||
            type_name.find(STR("UAvoidanceManager")) != type_name.npos ||
            type_name.find(STR("FOnBeginTearingDownEvent")) != type_name.npos ||
            type_name.find(STR("UBlueprint")) != type_name.npos ||
            type_name.find(STR("UCanvas")) != type_name.npos ||
            type_name.find(STR("UActorComponent")) != type_name.npos ||
            type_name.find(STR("AController")) != type_name.npos ||
            type_name.find(STR("ULevel")) != type_name.npos ||
            type_name.find(STR("FPhysScene_Chaos")) != type_name.npos ||
            type_name.find(STR("APhysicsVolume")) != type_name.npos ||
            type_name.find(STR("UDemoNetDriver")) != type_name.npos ||
            type_name.find(STR("FEndPhysicsTickFunction")) != type_name.npos ||
            type_name.find(STR("FFXSystemInterface")) != type_name.npos ||
            type_name.find(STR("ERHIFeatureLevel")) != type_name.npos ||
            type_name.find(STR("EFlushLevelStreamingType")) != type_name.npos ||
            type_name.find(STR("ULineBatchComponent")) != type_name.npos ||
            type_name.find(STR("AGameState")) != type_name.npos ||
            type_name.find(STR("FOnGameStateSetEvent")) != type_name.npos ||
            type_name.find(STR("AAudioVolume")) != type_name.npos ||
            type_name.find(STR("FLatentActionManager")) != type_name.npos ||
            type_name.find(STR("FOnLevelsChangedEvent")) != type_name.npos ||
            type_name.find(STR("AParticleEventManager")) != type_name.npos ||
            type_name.find(STR("UNavigationSystem")) != type_name.npos ||
            type_name.find(STR("UNetDriver")) != type_name.npos ||
            type_name.find(STR("AGameNetworkManager")) != type_name.npos ||
            type_name.find(STR("ETravelType")) != type_name.npos ||
            type_name.find(STR("FDefaultDelegateUserPolicy")) != type_name.npos ||
            type_name.find(STR("TMulticastDelegate")) != type_name.npos ||
            type_name.find(STR("FActorsInitializedParams")) != type_name.npos ||
            type_name.find(STR("FOnBeginPostProcessSettings")) != type_name.npos ||
            type_name.find(STR("FIntVector")) != type_name.npos ||
            type_name.find(STR("UGameInstance")) != type_name.npos ||
            type_name.find(STR("FWorldPSCPool")) != type_name.npos ||
            type_name.find(STR("UMaterialParameterCollectionInstance")) != type_name.npos ||
            type_name.find(STR("FParticlePerfStats")) != type_name.npos ||
            type_name.find(STR("FWorldInGamePerformanceTrackers")) != type_name.npos ||
            type_name.find(STR("UPhysicsCollisionHandler")) != type_name.npos ||
            type_name.find(STR("UPhysicsFieldComponent")) != type_name.npos ||
            type_name.find(STR("FPhysScene")) != type_name.npos ||
            type_name.find(STR("APlayerController")) != type_name.npos ||
            type_name.find(STR("IInterface_PostProcessVolume")) != type_name.npos ||
            type_name.find(STR("FOnTickFlushEvent")) != type_name.npos ||
            type_name.find(STR("FSceneInterface")) != type_name.npos ||
            type_name.find(STR("FStartAsyncSimulationFunction")) != type_name.npos ||
            type_name.find(STR("FStartPhysicsTickFunction")) != type_name.npos ||
            type_name.find(STR("FOnNetTickEvent")) != type_name.npos ||
            type_name.find(STR("ETickingGroup")) != type_name.npos ||
            type_name.find(STR("FTickTaskLevel")) != type_name.npos ||
            type_name.find(STR("FTimerManager")) != type_name.npos ||
            type_name.find(STR("FURL")) != type_name.npos ||
            type_name.find(STR("UWorldComposition")) != type_name.npos ||
            type_name.find(STR("EWorldType")) != type_name.npos ||
            type_name.find(STR("FSubsystemCollection")) != type_name.npos ||
            type_name.find(STR("UWorldSubsystem")) != type_name.npos ||
            type_name.find(STR("FStreamingLevelsToConsider")) != type_name.npos ||
            type_name.find(STR("APawn")) != type_name.npos ||
            type_name.find(STR("ACameraActor")) != type_name.npos ||
            type_name.find(STR("EMapPropertyFlags")) != type_name.npos ||
            type_name.find(STR("FScriptMapLayout")) != type_name.npos ||
            type_name.find(STR("EArrayPropertyFlags")) != type_name.npos ||
            type_name.find(STR("ICppClassTypeInfo")) != type_name.npos ||
            type_name.find(STR("FDefaultSetAllocator")) != type_name.npos ||
            type_name.find(STR("TDefaultMapKeyFuncs")) != type_name.npos ||
            type_name.find(STR("FNativeFunctionLookup")) != type_name.npos ||
            type_name.find(STR("FGCReferenceTokenStream")) != type_name.npos ||
            type_name.find(STR("FWindowsCriticalSection")) != type_name.npos ||
            type_name.find(STR("TDefaultMapHashableKeyFuncs")) != type_name.npos ||
            type_name.find(STR("FWindowsRWLock")) != type_name.npos ||
            type_name.find(STR("FRepRecord")) != type_name.npos ||
            type_name.find(STR("EClassCastFlags")) != type_name.npos ||
            type_name.find(STR("FAudioDeviceHandle")) != type_name.npos ||
            type_name.find(STR("TVector")) != type_name.npos ||
            type_name.find(STR("FScriptSetLayout")) != type_name.npos
            )        
        {
            
            // These types are not currently supported in RC::Unreal, so we must prevent code from being generated.
            return false;
        }
        else
        {
            return true;
        }
    }

    static std::unordered_map<File::StringType, std::unordered_set<File::StringType>> g_private_variables{
            {
                    STR("FField"),
                    {
                            STR("ClassPrivate"),
                            STR("NamePrivate"),
                            STR("Next"),
                            STR("Owner"),
                    }
            },
    };

    static std::unordered_set<File::StringType> NonCasePreservingVariants{
            {STR("4_27")},
    };

    static std::unordered_set<File::StringType> CasePreservingVariants{
            {STR("4_27_CasePreserving")},
    };

    auto static event_loop_update() -> void
    {
        for (processing_events = true; processing_events;)
        {
            input_handler.process_event();
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    auto static HRESULTToString(HRESULT result) -> std::string
    {
        switch (result)
        {
            case S_OK:
                return "S_OK";
            case S_FALSE:
                return "S_FALSE";
            case E_PDB_NOT_FOUND:
                return "E_PDB_NOT_FOUND: Failed to open the file, or the file has an invalid format.";
            case E_PDB_FORMAT:
                return "E_PDB_FORMAT: Attempted to access a file with an obsolete format.";
            case E_PDB_INVALID_SIG:
                return "E_PDB_INVALID_SIG: Signature does not match.";
            case E_PDB_INVALID_AGE:
                return "E_PDB_INVALID_AGE: Age does not match.";
            case E_INVALIDARG:
                return "E_INVALIDARG: Invalid parameter.";
            case E_UNEXPECTED:
                return "E_UNEXPECTED: Data source has already been prepared.";
            case E_NOINTERFACE:
                return "E_NOINTERFACE";
            case E_POINTER:
                return "E_POINTER";
            case REGDB_E_CLASSNOTREG:
                return "REGDB_E_CLASSNOTREG";
            case CLASS_E_NOAGGREGATION:
                return "CLASS_E_NOAGGREGATION";
            case CO_E_NOTINITIALIZED:
                return "CO_E_NOTINITIALIZED";
            default:
                return std::format("Unknown error. (error: {:X})", result);
        }
    }

    auto static kind_to_string(DWORD kind) -> File::StringViewType
    {
        switch (kind)
        {
            case DataIsUnknown:
                return STR("DataIsUnknown");
            case DataIsLocal:
                return STR("DataIsLocal");
            case DataIsStaticLocal:
                return STR("DataIsStaticLocal");
            case DataIsParam:
                return STR("DataIsParam");
            case DataIsObjectPtr:
                return STR("DataIsObjectPtr");
            case DataIsFileStatic:
                return STR("DataIsFileStatic");
            case DataIsGlobal:
                return STR("DataIsGlobal");
            case DataIsMember:
                return STR("DataIsMember");
            case DataIsStaticMember:
                return STR("DataIsStaticMember");
            case DataIsConstant:
                return STR("DataIsConstant");
        }

        return STR("UnhandledKind");
    }

    auto static sym_tag_to_string(DWORD sym_tag) -> File::StringViewType
    {
        switch (sym_tag)
        {
            case SymTagNull:
                return STR("SymTagNull");
            case SymTagExe:
                return STR("SymTagExe");
            case SymTagCompiland:
                return STR("SymTagCompiland");
            case SymTagCompilandDetails:
                return STR("SymTagCompilandDetails");
            case SymTagCompilandEnv:
                return STR("SymTagCompilandEnv");
            case SymTagFunction:
                return STR("SymTagFunction");
            case SymTagBlock:
                return STR("SymTagBlock");
            case SymTagData:
                return STR("SymTagData");
            case SymTagAnnotation:
                return STR("SymTagAnnotation");
            case SymTagLabel:
                return STR("SymTagLabel");
            case SymTagPublicSymbol:
                return STR("SymTagPublicSymbol");
            case SymTagUDT:
                return STR("SymTagUDT");
            case SymTagEnum:
                return STR("SymTagEnum");
            case SymTagFunctionType:
                return STR("SymTagFunctionType");
            case SymTagPointerType:
                return STR("SymTagPointerType");
            case SymTagArrayType:
                return STR("SymTagArrayType");
            case SymTagBaseType:
                return STR("SymTagBaseType");
            case SymTagTypedef:
                return STR("SymTagTypedef");
            case SymTagBaseClass:
                return STR("SymTagBaseClass");
            case SymTagFriend:
                return STR("SymTagFriend");
            case SymTagFunctionArgType:
                return STR("SymTagFunctionArgType");
            case SymTagFuncDebugStart:
                return STR("SymTagFuncDebugStart");
            case SymTagFuncDebugEnd:
                return STR("SymTagFuncDebugEnd");
            case SymTagUsingNamespace:
                return STR("SymTagUsingNamespace");
            case SymTagVTableShape:
                return STR("SymTagVTableShape");
            case SymTagVTable:
                return STR("SymTagVTable");
            case SymTagCustom:
                return STR("SymTagCustom");
            case SymTagThunk:
                return STR("SymTagThunk");
            case SymTagCustomType:
                return STR("SymTagCustomType");
            case SymTagManagedType:
                return STR("SymTagManagedType");
            case SymTagDimension:
                return STR("SymTagDimension");
            case SymTagCallSite:
                return STR("SymTagCallSite");
            case SymTagInlineSite:
                return STR("SymTagInlineSite");
            case SymTagBaseInterface:
                return STR("SymTagBaseInterface");
            case SymTagVectorType:
                return STR("SymTagVectorType");
            case SymTagMatrixType:
                return STR("SymTagMatrixType");
            case SymTagHLSLType:
                return STR("SymTagHLSLType");
            case SymTagCaller:
                return STR("SymTagCaller");
            case SymTagCallee:
                return STR("SymTagCallee");
            case SymTagExport:
                return STR("SymTagExport");
            case SymTagHeapAllocationSite:
                return STR("SymTagHeapAllocationSite");
            case SymTagCoffGroup:
                return STR("SymTagCoffGroup");
            case SymTagInlinee:
                return STR("SymTagInlinee");
            case SymTagMax:
                return STR("SymTagMax");
            default:
                return STR("Unknown");
        }
    }

    auto constexpr static is_symbol_valid(HRESULT result) -> bool
    {
        if (result != S_OK && result != S_FALSE)
        {
            Output::send(STR("Ran into an error with a symbol, error: {}\n"), to_wstring(HRESULTToString(result)));
            return false;
        }
        else if (result == S_FALSE)
        {
            //Output::send(STR("Symbol not valid but no error\n"));
            return false;
        }
        else
        {
            return true;
        }
    }

    auto static base_type_to_string(CComPtr<IDiaSymbol>& symbol) -> File::StringType
    {
        DWORD sym_tag;
        symbol->get_symTag(&sym_tag);
        if (sym_tag != SymTagBaseType)
        {
            throw std::runtime_error{"base_type_to_string only works on SymTagBaseType"};
        }

        File::StringType name{};

        ULONGLONG type_size;
        symbol->get_length(&type_size);

        BasicType base_type;
        symbol->get_baseType(std::bit_cast<DWORD*>(&base_type));

        switch (base_type)
        {
            case btVoid:
                name.append(STR("void"));
                break;
            case btChar:
            case btWChar:
                name.append(STR("TCHAR"));
                break;
            case btUInt:
                name.append(STR("u"));
            case btInt:
                switch (type_size)
                {
                    case 1:
                        name.append(STR("int8"));
                        break;
                    case 2:
                        name.append(STR("int16"));
                        break;
                    case 4:
                        name.append(STR("int32"));
                        break;
                    case 8:
                        name.append(STR("int64"));
                        break;
                    default:
                        name.append(STR("--unknown-int-size--"));
                        break;
                }
                break;
            case btFloat:
                switch (type_size)
                {
                    case 4:
                        name.append(STR("float"));
                        break;
                    case 8:
                        name.append(STR("double"));
                        break;
                    default:
                        name.append(STR("--unknown-float-size--"));
                        break;
                }
                break;
            case btBool:
                name.append(STR("bool"));
                break;
            case btNoType:
            case btBCD:
            case btLong:
            case btULong:
            case btCurrency:
            case btDate:
            case btVariant:
            case btComplex:
            case btBit:
            case btBSTR:
            case btHresult:
            case btChar16:
            case btChar32:
            case btChar8:
                throw std::runtime_error{"Unsupported SymTagBaseType type."};
                break;
        }

        return name;
    }

    auto static get_symbol_name(CComPtr<IDiaSymbol>& symbol) -> File::StringType
    {
        File::StringType name{};

        DWORD sym_tag;
        symbol->get_symTag(&sym_tag);

        if (sym_tag == SymTagPointerType ||
            sym_tag == SymTagFunctionType)
        {
            CComPtr<IDiaSymbol> type_symbol;
            if (auto hr = symbol->get_type(&type_symbol); hr != S_OK)
            {
                throw std::runtime_error{std::format("Call to 'get_type()' failed with error '{}'", HRESULTToString(hr))};
            }
            return get_symbol_name(type_symbol);
        }
        else if (sym_tag == SymTagBaseType)
        {
            name.append(base_type_to_string(symbol));
        }
        else
        {
            BSTR name_buffer;
            if (auto hr = symbol->get_name(&name_buffer); hr == S_OK)
            {
                name = name_buffer;
                SysFreeString(name_buffer);
            }

            BSTR undecorated_name_buffer;
            if (auto hr = symbol->get_undecoratedName(&undecorated_name_buffer); hr == S_OK)
            {
                name = undecorated_name_buffer;
                SysFreeString(undecorated_name_buffer);
            }
        }

        if (name.empty())
        {
            //Output::send(STR("Name not found. sym_tag: {}\n"), sym_tag_to_string(sym_tag));
            name = STR("NoName");
        }

        return name;
    }

    auto VTableDumper::setup_symbol_loader() -> void
    {
        CoInitialize(nullptr);

        if (!std::filesystem::exists(pdb_file))
        {
            throw std::runtime_error{std::format("PDB '{}' not found", pdb_file.string())};
        }

        auto hr = CoCreateInstance(CLSID_DiaSource, nullptr, CLSCTX_INPROC_SERVER, __uuidof(IDiaDataSource), reinterpret_cast<void**>(&dia_source));
        if (FAILED(hr))
        {
            throw std::runtime_error{std::format("CoCreateInstance failed. Register msdia140.dll. Error: {}", HRESULTToString(hr))};
        }

        if (hr = dia_source->loadDataFromPdb(pdb_file.c_str()); FAILED(hr))
        {
            throw std::runtime_error{std::format("Failed to load symbol data with error: {}", HRESULTToString(hr))};
        }

        if (hr = dia_source->openSession(&dia_session); FAILED(hr))
        {
            throw std::runtime_error{std::format("Call to 'openSession' failed with error: {}", HRESULTToString(hr))};
        }

        if (hr = dia_session->get_globalScope(&dia_global_symbol); FAILED(hr))
        {
            throw std::runtime_error{std::format("Call to 'get_globalScope' failed with error: {}", HRESULTToString(hr))};
        }
    }

    auto static generate_pointer_type(CComPtr<IDiaSymbol>& symbol) -> File::StringType
    {
        HRESULT hr;
        DWORD sym_tag;
        symbol->get_symTag(&sym_tag);
        CComPtr<IDiaSymbol> real_symbol;

        if (sym_tag == SymTagFunctionType)
        {
            if (hr = symbol->get_type(&real_symbol); hr != S_OK)
            {
                throw std::runtime_error{std::format("Call to 'get_type()' failed with error '{}'", HRESULTToString(hr))};
            }
        }
        else
        {
            real_symbol = symbol;
        }

        DWORD real_sym_tag;
        real_symbol->get_symTag(&real_sym_tag);

        if (real_sym_tag != SymTagPointerType)
        {
            return STR("");
        }

        BOOL is_reference = FALSE;
        if (hr = real_symbol->get_reference(&is_reference); hr != S_OK)
        {
            throw std::runtime_error{std::format("Call to 'get_reference(&is_reference)' failed with error '{}'", HRESULTToString(hr))};
        }

        if (is_reference == TRUE)
        {
            return STR("&");
        }
        else
        {
            return STR("*");
        }
    }

    auto static generate_const_qualifier(CComPtr<IDiaSymbol>& symbol) -> File::StringType
    {
        HRESULT hr;
        CComPtr<IDiaSymbol> real_symbol = symbol;
        DWORD sym_tag;
        real_symbol->get_symTag(&sym_tag);

        // TODO: Fix this. It's currently broken for two reasons.
        //       1. If sym_tag == SymTagFunctionType, we assume that we're looking for the return type but we might be looking for the const qualifier on the function itself.
        //       2. Even if we change this to fix the above problem, for some reason, calling 'get_constType' on the SymTagFunctionType sets the BOOL to FALSE, and calling it on SymTagFunction returns S_FALSE.
        if (sym_tag == SymTagFunctionType)
        {
            if (hr = real_symbol->get_type(&real_symbol.p); hr != S_OK)
            {
                throw std::runtime_error{std::format("Call to 'get_type()' failed with error '{}'", HRESULTToString(hr))};
            }

            real_symbol->get_symTag(&sym_tag);
            if (sym_tag == SymTagPointerType)
            {
                if (hr = real_symbol->get_type(&real_symbol.p); hr != S_OK)
                {
                    throw std::runtime_error{std::format("Call to 'get_type()' failed with error '{}'", HRESULTToString(hr))};
                }
            }
        }

        BOOL is_const = FALSE;
        real_symbol->get_constType(&is_const);

        return is_const ? STR("const") : STR("");
    }

    auto static generate_type(CComPtr<IDiaSymbol>& symbol) -> File::StringType
    {
        HRESULT hr;
        CComPtr<IDiaSymbol> function_type_symbol;
        if (hr = symbol->get_type(&function_type_symbol); hr != S_OK)
        {
            function_type_symbol = symbol;
        }

        // TODO: Const for pointers
        //       We only support const for non-pointers and the data the pointer is pointing to
        auto return_type_name = get_symbol_name(function_type_symbol);
        auto pointer_type = generate_pointer_type(function_type_symbol);
        auto const_qualifier = generate_const_qualifier(function_type_symbol);

        return std::format(STR("{}{}{}"), const_qualifier.empty() ? STR("") : std::format(STR("{} "), const_qualifier), return_type_name, pointer_type);
    }

    auto static generate_function_params(CComPtr<IDiaSymbol>& symbol) -> File::StringType
    {
        HRESULT hr;

        CComPtr<IDiaSymbol> function_type_symbol;
        if (hr = symbol->get_type(&function_type_symbol); hr != S_OK)
        {
            throw std::runtime_error{std::format("Call to 'get_type(&function_type_symbol)' failed with error: {}", HRESULTToString(hr))};
        }

        File::StringType params{};

        DWORD param_count;
        function_type_symbol->get_count(&param_count);

        // TODO: Check if we have a 'this' param.
        //       Right now we're assuming that there is a 'this' param, and subtracting 1 from the count to account for it.
        --param_count;

        params.append(std::format(STR("param_count: {}, "), param_count));

        if (param_count == 0) { return STR(""); }

        DWORD tag;
        symbol->get_symTag(&tag);

        DWORD current_param{1};
        CComPtr<IDiaEnumSymbols> sub_symbols;
        if (hr = symbol->findChildren(SymTagNull, nullptr, NULL, &sub_symbols.p); hr == S_OK)
        {
            CComPtr<IDiaSymbol> sub_symbol;
            ULONG num_symbols_fetched{};
            LONG count;
            hr = sub_symbols->get_Count(&count);
            while (sub_symbols->Next(1, &sub_symbol.p, &num_symbols_fetched) == S_OK && num_symbols_fetched == 1)
            {
                DWORD data_kind;
                sub_symbol->get_dataKind(&data_kind);

                if (data_kind != DataKind::DataIsParam)
                {
                    sub_symbol = nullptr;
                    continue;
                }

                params.append(std::format(STR("{} {}{}"), generate_type(sub_symbol), get_symbol_name(sub_symbol), current_param < param_count ? STR(", ") : STR("")));

                ++current_param;
            }
            sub_symbol = nullptr;
        }
        sub_symbols = nullptr;

        return params;
    }

    auto static generate_function_signature(CComPtr<IDiaSymbol>& symbol) -> File::StringType
    {
        DWORD tag;
        symbol->get_symTag(&tag);
        auto params = generate_function_params(symbol);
        auto return_type = generate_type(symbol);
        File::StringType const_qualifier = generate_const_qualifier(symbol);

        //Output::send(STR("{} {}({}){};\n"), return_type, get_symbol_name(symbol), params, const_qualifier.empty() ? STR("") : std::format(STR(" {}"), const_qualifier));

        return std::format(STR("{} {}({}){};"), return_type, get_symbol_name(symbol), params, const_qualifier.empty() ? STR("") : std::format(STR(" {}"), const_qualifier));
    }

    auto& get_existing_or_create_new_enum_entry(const File::StringType& symbol_name, const File::StringType& symbol_name_clean)
    {
        if (auto it = g_enum_entries.find(symbol_name); it != g_enum_entries.end())
        {
            return it->second;
        }
        else
        {
            return g_enum_entries.emplace(symbol_name, EnumEntry{
                    .name = symbol_name,
                    .name_clean = symbol_name_clean,
                    .variables = {}
            }).first->second;
        }
    }

    auto& get_existing_or_create_new_class_entry(std::filesystem::path& pdb_file, const File::StringType& symbol_name, const File::StringType& symbol_name_clean, const SymbolNameInfo& name_info)
    {
        auto pdb_file_name = pdb_file.filename().stem();
        auto& class_entry = [&]() -> auto& {
            if (g_class_entries.contains(pdb_file_name))
            {
                auto& classes = g_class_entries[pdb_file_name];
                if (classes.entries.contains(symbol_name_clean))
                {
                    return classes.entries[symbol_name_clean];
                }
                else
                {
                    return classes.entries.emplace(symbol_name_clean, Class{
                            .class_name = File::StringType{symbol_name},
                            .class_name_clean = symbol_name_clean
                    }).first->second;
                }
            }
            else
            {
                return g_class_entries[pdb_file_name].entries[symbol_name_clean] = Class{
                        .class_name = File::StringType{symbol_name},
                        .class_name_clean = symbol_name_clean
                };
            }
        }();

        class_entry.valid_for_member_vars = name_info.valid_for_member_vars;
        class_entry.valid_for_vtable = name_info.valid_for_vtable;
        return class_entry;
    }

    auto get_type_name(CComPtr<IDiaSymbol>& type) -> File::StringType
    {
        DWORD type_tag;
        type->get_symTag(&type_tag);

        if (type_tag == SymTagEnum ||
            type_tag == SymTagUDT)
        {
            return get_symbol_name(type);
        }
        else if (type_tag == SymTagBaseType)
        {
            return base_type_to_string(type);
        }
        else if (type_tag == SymTagPointerType)
        {
            auto type_name = get_symbol_name(type);
            auto pointer_type = generate_pointer_type(type);
            return std::format(STR("{}{}"), type_name, pointer_type);
        }
        else if (type_tag == SymTagArrayType)
        {
            return get_symbol_name(type);
        }
        else
        {
            throw std::runtime_error{to_string(std::format(STR("Unknown type: {}."), type_tag))};
        }
    }

    auto VTableDumper::dump_member_variable_layouts(CComPtr<IDiaSymbol>& symbol, const SymbolNameInfo& name_info, EnumEntriesTypeAlias enum_entry, Class* class_entry) -> void
    {
        auto symbol_name = get_symbol_name(symbol);

        bool could_not_replace_prefix{};
        for (const auto& UPrefixed : UPrefixToFPrefix)
        {
            for (size_t i = symbol_name.find(UPrefixed); i != symbol_name.npos; i = symbol_name.find(UPrefixed))
            {
                if (is_425_plus)
                {
                    could_not_replace_prefix = true;
                    break;
                }
                symbol_name.replace(i, 1, STR("F"));
                ++i;
            }
        }

        if (could_not_replace_prefix)
        {
            return;
        }

        File::StringType symbol_name_clean{symbol_name};
        std::replace(symbol_name_clean.begin(), symbol_name_clean.end(), ':', '_');
        std::replace(symbol_name_clean.begin(), symbol_name_clean.end(), '~', '$');

        DWORD sym_tag;
        symbol->get_symTag(&sym_tag);

        if (symbol_name == STR("Names"))
        {
            Output::send(STR("tag for Names: {}\n"), sym_tag_to_string(sym_tag));
        }

        HRESULT hr;
        if (sym_tag == SymTagUDT)
        {
            if (valid_udt_names.find(symbol_name) == valid_udt_names.end()) { return; }
            auto& local_class_entry = get_existing_or_create_new_class_entry(pdb_file, symbol_name, symbol_name_clean, name_info);
            auto& local_enum_entry = get_existing_or_create_new_enum_entry(symbol_name, symbol_name_clean);

            Output::send(STR("UDT symbol name: {}, enum_entry name: {}, enum_entry name clean: {}\n"), symbol_name, local_enum_entry.name, local_enum_entry.name_clean);

            CComPtr<IDiaEnumSymbols> sub_symbols;
            if (hr = symbol->findChildren(SymTagNull, nullptr, NULL, &sub_symbols.p); hr == S_OK)
            {
                CComPtr<IDiaSymbol> sub_symbol;
                ULONG num_symbols_fetched{};
                while (sub_symbols->Next(1, &sub_symbol.p, &num_symbols_fetched) == S_OK && num_symbols_fetched == 1)
                {
                    SymbolNameInfo new_name_info = name_info;
                    dump_member_variable_layouts(sub_symbol, new_name_info, &local_enum_entry, &local_class_entry);
                }
                sub_symbol = nullptr;
            }
            sub_symbols = nullptr;
        }
        else if (sym_tag == SymTagData)
        {
            if (!enum_entry) { throw std::runtime_error{"enum_entries is nullptr"}; }
            if (!class_entry) { throw std::runtime_error{"class_entry is nullptr"}; }

            DWORD kind;
            symbol->get_dataKind(&kind);
            if (kind != DataKind::DataIsMember) { return; }

            CComPtr<IDiaSymbol> type;
            if (hr = symbol->get_type(&type); hr != S_OK) { throw std::runtime_error{"Could not get type\n"}; }

            DWORD type_tag;
            type->get_symTag(&type_tag);

            auto type_name = get_type_name(type);
            if (!is_valid_type_to_dump(type_name)) { return; }

            for (const auto& UPrefixed : UPrefixToFPrefix)
            {
                for (size_t i = type_name.find(UPrefixed); i != type_name.npos; i = type_name.find(UPrefixed))
                {
                    if (is_425_plus)
                    {
                        could_not_replace_prefix = true;
                        break;
                    }
                    type_name.replace(i, 1, STR("F"));
                    ++i;
                }
            }

            if (could_not_replace_prefix)
            {
                return;
            }

            LONG offset;
            symbol->get_offset(&offset);

            auto& enum_entry_variable = enum_entry->variables[symbol_name];
            enum_entry_variable.type = type_name;
            enum_entry_variable.name = symbol_name;
            enum_entry_variable.offset = offset;

            Output::send(STR("{} {} ({}, {}); 0x{:X}\n"), type_name, symbol_name, sym_tag_to_string(type_tag), kind_to_string(kind), offset);
            auto& class_entry_variable = class_entry->variables[symbol_name];
            class_entry_variable.type = type_name;
            class_entry_variable.name = symbol_name;
            class_entry_variable.offset = offset;
        }
    }

    auto VTableDumper::dump_member_variable_layouts(std::unordered_map<File::StringType, SymbolNameInfo>& names) -> void
    {
        Output::send(STR("Dumping {} symbols for {}\n"), names.size(), pdb_file.filename().stem().wstring());

        for (const auto&[name, name_info] : names)
        {
            Output::send(STR("{}...\n"), name);
            HRESULT hr{};
            if (hr = dia_session->findChildren(nullptr, SymTagNull, nullptr, nsNone, &dia_global_symbols_enum.p); hr != S_OK)
            {
                throw std::runtime_error{std::format("Call to 'findChildren' failed with error '{}'", HRESULTToString(hr))};
            }

            CComPtr<IDiaSymbol> symbol;
            ULONG celt_fetched;
            if (hr = dia_global_symbols_enum->Next(1, &symbol.p, &celt_fetched); hr != S_OK)
            {
                throw std::runtime_error{std::format("Ran into an error with a symbol while calling 'Next', error: {}\n", HRESULTToString(hr))};
            }

            CComPtr<IDiaEnumSymbols> sub_symbols;
            hr = symbol->findChildren(SymTagNull, name.c_str(), NULL, &sub_symbols.p);
            if (hr != S_OK)
            {
                throw std::runtime_error{std::format("Ran into a problem while calling 'findChildren', error: {}\n", HRESULTToString(hr))};
            }

            CComPtr<IDiaSymbol> sub_symbol;
            ULONG num_symbols_fetched;

            sub_symbols->Next(1, &sub_symbol.p, &num_symbols_fetched);
            if (!sub_symbol)
            {
                //Output::send(STR("Missed symbol '{}'\n"), name);
                symbol = nullptr;
                sub_symbols = nullptr;
                dia_global_symbols_enum = nullptr;
                continue;
            }

            dump_member_variable_layouts(sub_symbol, name_info);
        }
    }

    auto VTableDumper::dump_vtable_for_symbol(CComPtr<IDiaSymbol>& symbol, const SymbolNameInfo& name_info, EnumEntriesTypeAlias enum_entries, Class* class_entry) -> void
    {
        // Symbol name => Offset in vtable
        static std::unordered_map<File::StringType, uint32_t> functions_already_dumped{};

        auto symbol_name = get_symbol_name(symbol);
        //Output::send(STR("symbol_name: {}\n"), symbol_name);

        bool could_not_replace_prefix{};
        for (const auto& UPrefixed : UPrefixToFPrefix)
        {
            for (size_t i = symbol_name.find(UPrefixed); i != symbol_name.npos; i = symbol_name.find(UPrefixed))
            {
                if (is_425_plus)
                {
                    could_not_replace_prefix = true;
                    break;
                }
                symbol_name.replace(i, 1, STR("F"));
                ++i;
            }
        }

        if (could_not_replace_prefix)
        {
            return;
        }

        bool is_overload{};
        if (auto it = functions_already_dumped.find(symbol_name); it != functions_already_dumped.end())
        {
            symbol_name.append(std::format(STR("_{}"), ++it->second));
            is_overload = true;
        }

        File::StringType symbol_name_clean{symbol_name};
        std::replace(symbol_name_clean.begin(), symbol_name_clean.end(), ':', '_');
        std::replace(symbol_name_clean.begin(), symbol_name_clean.end(), '~', '$');

        DWORD sym_tag;
        symbol->get_symTag(&sym_tag);

        auto process_struct_or_class = [&]()
        {
            if (valid_udt_names.find(symbol_name) == valid_udt_names.end()) { return; }
            functions_already_dumped.clear();
            //Output::send(STR("Dumping vtable for symbol '{}', tag: '{}'\n"), symbol_name, sym_tag_to_string(sym_tag));

            auto& local_enum_entries = get_existing_or_create_new_enum_entry(symbol_name, symbol_name_clean);
            auto& local_class_entry = get_existing_or_create_new_class_entry(pdb_file, symbol_name, symbol_name_clean, name_info);

            HRESULT hr;
            CComPtr<IDiaEnumSymbols> sub_symbols;
            if (hr = symbol->findChildren(SymTagNull, nullptr, NULL, &sub_symbols.p); hr == S_OK)
            {
                CComPtr<IDiaSymbol> sub_symbol;
                ULONG num_symbols_fetched{};
                while (sub_symbols->Next(1, &sub_symbol.p, &num_symbols_fetched) == S_OK && num_symbols_fetched == 1)
                {
                    SymbolNameInfo new_name_info = name_info;
                    dump_vtable_for_symbol(sub_symbol, new_name_info, &local_enum_entries, &local_class_entry);
                }
                sub_symbol = nullptr;
            }
            sub_symbols = nullptr;
        };

        if (sym_tag == SymTagUDT)
        {
            process_struct_or_class();
        }
        else if (sym_tag == SymTagBaseClass)
        {
            process_struct_or_class();
        }
        else if (sym_tag == SymTagFunction)
        {
            if (!enum_entries) { throw std::runtime_error{"enum_entries is nullptr"}; }
            if (!class_entry) { throw std::runtime_error{"class_entry is nullptr"}; }

            BOOL is_virtual = FALSE;
            symbol->get_virtual(&is_virtual);
            if (is_virtual == FALSE) { return; }


            HRESULT hr;
            DWORD offset_in_vtable = 0;
            if (hr = symbol->get_virtualBaseOffset(&offset_in_vtable); hr != S_OK)
            {
                throw std::runtime_error{std::format("Call to 'get_virtualBaseOffset' failed with error '{}'", HRESULTToString(hr))};
            }

            //Output::send(STR("Dumping virtual function for symbol '{}', tag: '{}', offset: '{}'\n"), symbol_name, sym_tag_to_string(sym_tag), offset_in_vtable / 8);

            auto& function = class_entry->functions[offset_in_vtable];
            function.name = symbol_name_clean;
            function.signature = generate_function_signature(symbol);
            function.offset = offset_in_vtable;
            function.is_overload = is_overload;
            functions_already_dumped.emplace(symbol_name, 1);
        }
    }

    auto VTableDumper::dump_vtable_for_symbol(std::unordered_map<File::StringType, SymbolNameInfo>& names) -> void
    {
        Output::send(STR("Dumping {} struct symbols for {}\n"), names.size(), pdb_file.filename().stem().wstring());

        for (const auto&[name, name_info] : names)
        {
            //Output::send(STR("Looking for {}\n"), name);
            HRESULT hr{};
            if (hr = dia_session->findChildren(nullptr, SymTagNull, nullptr, nsNone, &dia_global_symbols_enum.p); hr != S_OK)
            {
                throw std::runtime_error{std::format("Call to 'findChildren' failed with error '{}'", HRESULTToString(hr))};
            }

            CComPtr<IDiaSymbol> symbol;
            ULONG celt_fetched;
            if (hr = dia_global_symbols_enum->Next(1, &symbol.p, &celt_fetched); hr != S_OK)
            {
                throw std::runtime_error{std::format("Ran into an error with a symbol while calling 'Next', error: {}\n", HRESULTToString(hr))};
            }

            CComPtr<IDiaEnumSymbols> sub_symbols;
            hr = symbol->findChildren(SymTagNull, name.c_str(), NULL, &sub_symbols.p);
            if (hr != S_OK)
            {
                throw std::runtime_error{std::format("Ran into a problem while calling 'findChildren', error: {}\n", HRESULTToString(hr))};
            }

            CComPtr<IDiaSymbol> sub_symbol;
            ULONG num_symbols_fetched;

            sub_symbols->Next(1, &sub_symbol.p, &num_symbols_fetched);
            if (name == STR("UConsole"))
            {
                LONG count;
                sub_symbols->get_Count(&count);
                //Output::send(STR("  Symbols count: {}\n"), count);
                //Output::send(STR("  sub_symbol: {}\n"), (void*)sub_symbol);
            }
            if (!sub_symbol)
            {
                //Output::send(STR("Missed symbol '{}'\n"), name);
                symbol = nullptr;
                sub_symbols = nullptr;
                dia_global_symbols_enum = nullptr;
                continue;
            }
            dump_vtable_for_symbol(sub_symbol, name_info);
        }
    }

    auto static generate_files(VTableOrMemberVars vtable_or_member_vars) -> void
    {
        static std::filesystem::path vtable_gen_output_path = "GeneratedVTables";
        static std::filesystem::path vtable_gen_output_include_path = vtable_gen_output_path / "generated_include";
        static std::filesystem::path vtable_gen_output_src_path = vtable_gen_output_path / "generated_src";
        static std::filesystem::path vtable_gen_output_function_bodies_path = vtable_gen_output_include_path / "FunctionBodies";
        static std::filesystem::path vtable_templates_output_path = "VTableLayoutTemplates";
        static std::filesystem::path member_variable_layouts_gen_output_path = "GeneratedMemberVariableLayouts";
        static std::filesystem::path member_variable_layouts_gen_output_include_path = member_variable_layouts_gen_output_path / "generated_include";
        static std::filesystem::path member_variable_layouts_gen_output_src_path = member_variable_layouts_gen_output_path / "generated_src";
        static std::filesystem::path member_variable_layouts_gen_function_bodies_path = member_variable_layouts_gen_output_include_path / "FunctionBodies";
        static std::filesystem::path member_variable_layouts_templates_output_path = "MemberVarLayoutTemplates";
        static std::filesystem::path virtual_gen_output_path = "GeneratedVirtualImplementations";
        static std::filesystem::path virtual_gen_output_include_path = virtual_gen_output_path / "generated_include";
        static std::filesystem::path virtual_gen_function_bodies_path = virtual_gen_output_include_path / "FunctionBodies";
        static std::filesystem::path sol_bindings_output_path = "SolBindings";

        if (vtable_or_member_vars == VTableOrMemberVars::VTable)
        {
            if (std::filesystem::exists(vtable_gen_output_include_path))
            {
                for (const auto& item : std::filesystem::directory_iterator(vtable_gen_output_include_path))
                {
                    if (item.is_directory()) { continue; }
                    if (item.path().extension() != STR(".hpp") && item.path().extension() != STR(".cpp")) { continue; }

                    File::delete_file(item.path());
                }
            }

            if (std::filesystem::exists(vtable_gen_output_function_bodies_path))
            {
                for (const auto& item : std::filesystem::directory_iterator(vtable_gen_output_function_bodies_path))
                {
                    if (item.is_directory()) { continue; }
                    if (item.path().extension() != STR(".cpp")) { continue; }

                    File::delete_file(item.path());
                }
            }

            if (std::filesystem::exists(vtable_gen_output_src_path))
            {
                for (const auto& item : std::filesystem::directory_iterator(vtable_gen_output_src_path))
                {
                    if (item.is_directory()) { continue; }
                    if (item.path().extension() != STR(".cpp")) { continue; }

                    File::delete_file(item.path());
                }
            }

            for (const auto&[pdb_name, classes] : g_class_entries)
            {
                for (const auto&[class_name, class_entry] : classes.entries)
                {
                    Output::send(STR("Generating file '{}_VTableOffsets_{}_FunctionBody.cpp'\n"), pdb_name, class_entry.class_name_clean);
                    Output::Targets<Output::NewFileDevice> function_body_dumper;
                    auto& function_body_file_device = function_body_dumper.get_device<Output::NewFileDevice>();
                    function_body_file_device.set_file_name_and_path(vtable_gen_output_function_bodies_path / std::format(STR("{}_VTableOffsets_{}_FunctionBody.cpp"), pdb_name, class_name));
                    function_body_file_device.set_formatter([](File::StringViewType string) {
                        return File::StringType{string};
                    });

                    for (const auto&[function_index, function_entry] : class_entry.functions)
                    {
                        auto local_class_name = class_entry.class_name;
                        if (auto pos = local_class_name.find(STR("Property")); pos != local_class_name.npos)
                        {
                            local_class_name.replace(0, 1, STR("F"));
                        }

                        function_body_dumper.send(STR("if (auto it = {}::VTableLayoutMap.find(STR(\"{}\")); it == {}::VTableLayoutMap.end())\n"), local_class_name, function_entry.name, local_class_name);
                        function_body_dumper.send(STR("{\n"));
                        function_body_dumper.send(STR("    {}::VTableLayoutMap.emplace(STR(\"{}\"), 0x{:X});\n"), local_class_name, function_entry.name, function_entry.offset);
                        function_body_dumper.send(STR("}\n\n"));
                    }
                }
            }

            for (const auto&[pdb_name, classes] : g_class_entries)
            {
                auto template_file = std::format(STR("VTableLayout_{}_Template.ini"), pdb_name);
                Output::send(STR("Generating file '{}'\n"), template_file);
                Output::Targets<Output::NewFileDevice> ini_dumper;
                auto& ini_file_device = ini_dumper.get_device<Output::NewFileDevice>();
                ini_file_device.set_file_name_and_path(vtable_templates_output_path / template_file);
                ini_file_device.set_formatter([](File::StringViewType string) {
                    return File::StringType{string};
                });

                for (const auto&[class_name, class_entry] : classes.entries)
                {
                    ini_dumper.send(STR("[{}]\n"), class_entry.class_name);

                    for (const auto&[function_index, function_entry] : class_entry.functions)
                    {
                        if (function_entry.is_overload)
                        {
                            ini_dumper.send(STR("; {}\n"), function_entry.signature);
                        }
                        ini_dumper.send(STR("{}\n"), function_entry.name);
                    }

                    ini_dumper.send(STR("\n"));
                }
            }
        }
        else if (vtable_or_member_vars == VTableOrMemberVars::MemberVars)
        {
            if (std::filesystem::exists(member_variable_layouts_gen_output_include_path))
            {
                for (const auto& item : std::filesystem::directory_iterator(member_variable_layouts_gen_output_include_path))
                {
                    if (item.is_directory()) { continue; }
                    if (item.path().extension() != STR(".hpp")) { continue; }

                    File::delete_file(item.path());
                }
            }

            if (std::filesystem::exists(member_variable_layouts_gen_function_bodies_path))
            {
                for (const auto& item : std::filesystem::directory_iterator(member_variable_layouts_gen_function_bodies_path))
                {
                    if (item.is_directory()) { continue; }
                    if (item.path().extension() != STR(".hpp") && item.path().extension() != STR(".cpp")) { continue; }

                    File::delete_file(item.path());
                }
            }

            if (std::filesystem::exists(member_variable_layouts_gen_output_src_path))
            {
                for (const auto& item : std::filesystem::directory_iterator(member_variable_layouts_gen_output_src_path))
                {
                    if (item.is_directory()) { continue; }
                    if (item.path().extension() != STR(".cpp")) { continue; }

                    File::delete_file(item.path());
                }
            }

            if (std::filesystem::exists(virtual_gen_output_include_path))
            {
                for (const auto& item : std::filesystem::directory_iterator(virtual_gen_output_include_path))
                {
                    if (item.is_directory()) { continue; }
                    if (item.path().extension() != STR(".hpp")) { continue; }

                    File::delete_file(item.path());
                }
            }

            if (std::filesystem::exists(virtual_gen_function_bodies_path))
            {
                for (const auto& item : std::filesystem::directory_iterator(virtual_gen_function_bodies_path))
                {
                    if (item.is_directory()) { continue; }
                    if (item.path().extension() != STR(".cpp")) { continue; }

                    File::delete_file(item.path());
                }
            }

            auto default_template_file = std::filesystem::path{STR("MemberVariableLayout.ini")};
            Output::send(STR("Generating file '{}'\n"), default_template_file.wstring());
            Output::Targets<Output::NewFileDevice> default_ini_dumper;
            auto& default_ini_file_device = default_ini_dumper.get_device<Output::NewFileDevice>();
            default_ini_file_device.set_file_name_and_path(member_variable_layouts_templates_output_path / default_template_file);
            default_ini_file_device.set_formatter([](File::StringViewType string) {
                return File::StringType{string};
            });

            for (const auto&[pdb_name, classes] : g_class_entries)
            {
                auto template_file = std::format(STR("MemberVariableLayout_{}_Template.ini"), pdb_name);
                Output::send(STR("Generating file '{}'\n"), template_file);
                Output::Targets<Output::NewFileDevice> ini_dumper;
                auto& ini_file_device = ini_dumper.get_device<Output::NewFileDevice>();
                ini_file_device.set_file_name_and_path(member_variable_layouts_templates_output_path / template_file);
                ini_file_device.set_formatter([](File::StringViewType string) {
                    return File::StringType{string};
                });

                auto pdb_name_no_underscore = pdb_name;
                pdb_name_no_underscore.replace(pdb_name_no_underscore.find(STR('_')), 1, STR(""));

                auto virtual_header_file = virtual_gen_output_include_path / std::format(STR("UnrealVirtual{}.hpp"), pdb_name_no_underscore);
                Output::send(STR("Generating file '{}'\n"), virtual_header_file.wstring());
                Output::Targets<Output::NewFileDevice> virtual_header_dumper;
                auto& virtual_header_file_device = virtual_header_dumper.get_device<Output::NewFileDevice>();
                virtual_header_file_device.set_file_name_and_path(virtual_header_file);
                virtual_header_file_device.set_formatter([](File::StringViewType string) {
                    return File::StringType{string};
                });

                auto virtual_src_file = virtual_gen_function_bodies_path / std::format(STR("UnrealVirtual{}.cpp"), pdb_name_no_underscore);
                Output::send(STR("Generating file '{}'\n"), virtual_src_file.wstring());
                Output::Targets<Output::NewFileDevice> virtual_src_dumper;
                auto& virtual_src_file_device = virtual_src_dumper.get_device<Output::NewFileDevice>();
                virtual_src_file_device.set_file_name_and_path(virtual_src_file);
                virtual_src_file_device.set_formatter([](File::StringViewType string) {
                    return File::StringType{string};
                });

                bool is_case_preserving_pdb = !(CasePreservingVariants.find(pdb_name) == CasePreservingVariants.end());
                bool is_non_case_preserving_pdb = !(NonCasePreservingVariants.find(pdb_name) == NonCasePreservingVariants.end());

                if (!is_case_preserving_pdb)
                {
                    virtual_header_dumper.send(STR("#ifndef RC_UNREAL_UNREAL_VIRTUAL_{}\n#define RC_UNREAL_UNREAL_VIRTUAL{}_HPP\n\n"), pdb_name_no_underscore, pdb_name_no_underscore);
                    virtual_header_dumper.send(STR("#include <Unreal/VersionedContainer/UnrealVirtualImpl/UnrealVirtualBaseVC.hpp>\n\n"));
                    virtual_header_dumper.send(STR("namespace RC::Unreal\n"));
                    virtual_header_dumper.send(STR("{\n"));
                    virtual_header_dumper.send(STR("    class UnrealVirtual{} : public UnrealVirtualBaseVC\n"), pdb_name_no_underscore);
                    virtual_header_dumper.send(STR("    {\n"));
                    virtual_header_dumper.send(STR("        void set_virtual_offsets() override;\n"));
                    virtual_header_dumper.send(STR("    }\n"));
                    virtual_header_dumper.send(STR("}\n\n\n"));
                    virtual_header_dumper.send(STR("#endif // RC_UNREAL_UNREAL_VIRTUAL{}_HPP\n"), pdb_name_no_underscore);

                    virtual_src_dumper.send(STR("#include <Unreal/VersionedContainer/UnrealVirtualImpl/UnrealVirtual{}.hpp>\n\n"), pdb_name_no_underscore);
                    virtual_src_dumper.send(STR("// These are all the structs that have virtuals that need to have their offset set\n"));
                    virtual_src_dumper.send(STR("#include <Unreal/UObject.hpp>\n"));
                    virtual_src_dumper.send(STR("#include <Unreal/UScriptStruct.hpp>\n"));
                    virtual_src_dumper.send(STR("#include <Unreal/FOutputDevice.hpp>\n"));
                    virtual_src_dumper.send(STR("#include <Unreal/FField.hpp>\n"));
                    virtual_src_dumper.send(STR("#include <Unreal/FProperty.hpp>\n"));
                    virtual_src_dumper.send(STR("#include <Unreal/Property/FNumericProperty.hpp>\n"));
                    virtual_src_dumper.send(STR("#include <Unreal/Property/FObjectProperty.hpp>\n"));
                    virtual_src_dumper.send(STR("#include <Unreal/Property/FMulticastDelegateProperty.hpp>\n"));
                    virtual_src_dumper.send(STR("#include <Unreal/Property/FStructProperty.hpp>\n"));
                    virtual_src_dumper.send(STR("#include <Unreal/Property/FArrayProperty.hpp>\n"));
                    virtual_src_dumper.send(STR("#include <Unreal/Property/FMapProperty.hpp>\n"));
                    virtual_src_dumper.send(STR("#include <Unreal/Property/FBoolProperty.hpp>\n"));
                    virtual_src_dumper.send(STR("#include <Unreal/Property/NumericPropertyTypes.hpp>\n"));
                    virtual_src_dumper.send(STR("#include <Unreal/Property/FSetProperty.hpp>\n"));
                    virtual_src_dumper.send(STR("#include <Unreal/Property/FInterfaceProperty.hpp>\n"));
                    virtual_src_dumper.send(STR("#include <Unreal/Property/FClassProperty.hpp>\n"));
                    virtual_src_dumper.send(STR("#include <Unreal/Property/FSoftClassProperty.hpp>\n"));
                    virtual_src_dumper.send(STR("#include <Unreal/Property/FEnumProperty.hpp>\n"));
                    virtual_src_dumper.send(STR("#include <Unreal/Property/FFieldPathProperty.hpp>\n"));
                    virtual_src_dumper.send(STR("#include <Unreal/UFunction.hpp>\n"));
                    virtual_src_dumper.send(STR("#include <Unreal/UClass.hpp>\n"));
                    virtual_src_dumper.send(STR("#include <Unreal/World.hpp>\n"));
                    virtual_src_dumper.send(STR("#include <Unreal/UEnum.hpp>\n"));
                    virtual_src_dumper.send(STR("#include <Unreal/FArchive.hpp>\n"));
                    virtual_src_dumper.send(STR("#include <Unreal/AGameModeBase.hpp>\n"));
                    virtual_src_dumper.send(STR("#include <Unreal/AGameMode.hpp>\n"));
                    virtual_src_dumper.send(STR("#include <Unreal/UPlayer.hpp>\n"));
                    virtual_src_dumper.send(STR("#include <Unreal/ULocalPlayer.hpp>\n"));
                    //virtual_src_dumper.send(STR("#include <Unreal/UConsole.hpp>\n"));
                    virtual_src_dumper.send(STR("\n"));
                    virtual_src_dumper.send(STR("namespace RC::Unreal\n"));
                    virtual_src_dumper.send(STR("{\n"));
                    virtual_src_dumper.send(STR("    void UnrealVirtual{}::set_virtual_offsets()\n"), pdb_name_no_underscore);
                    virtual_src_dumper.send(STR("    {\n"));
                }

                for (const auto&[class_name, class_entry] : classes.entries)
                {
                    if (!class_entry.functions.empty() && class_entry.valid_for_vtable == ValidForVTable::Yes && !is_case_preserving_pdb)
                    {
                        virtual_src_dumper.send(STR("#include <FunctionBodies/{}_VTableOffsets_{}_FunctionBody.cpp>\n"), pdb_name, class_name);
                    }

                    if (class_entry.variables.empty()) { continue; }

                    auto default_setter_src_file = member_variable_layouts_gen_function_bodies_path / std::format(STR("{}_MemberVariableLayout_DefaultSetter_{}.cpp"), pdb_name, class_entry.class_name_clean);
                    Output::send(STR("Generating file '{}'\n"), default_setter_src_file.wstring());
                    Output::Targets<Output::NewFileDevice> default_setter_src_dumper;
                    auto& default_setter_src_file_device = default_setter_src_dumper.get_device<Output::NewFileDevice>();
                    default_setter_src_file_device.set_file_name_and_path(default_setter_src_file);
                    default_setter_src_file_device.set_formatter([](File::StringViewType string) {
                        return File::StringType{string};
                    });

                    ini_dumper.send(STR("[{}]\n"), class_entry.class_name);
                    default_ini_dumper.send(STR("[{}]\n"), class_entry.class_name);

                    for (const auto&[variable_name, variable] : class_entry.variables)
                    {
                        ini_dumper.send(STR("{} = 0x{:X}\n"), variable.name, variable.offset);
                        default_ini_dumper.send(STR("{} = -1\n"), variable.name);

                        File::StringType final_variable_name = variable.name;

                        if (variable.name == STR("EnumFlags"))
                        {
                            final_variable_name = STR("EnumFlags_Internal");
                        }

                        default_setter_src_dumper.send(STR("if (auto it = {}::MemberOffsets.find(STR(\"{}\")); it == {}::MemberOffsets.end())\n"), class_entry.class_name, final_variable_name, class_entry.class_name);
                        default_setter_src_dumper.send(STR("{\n"));
                        default_setter_src_dumper.send(STR("    {}::MemberOffsets.emplace(STR(\"{}\"), 0x{:X});\n"), class_entry.class_name, final_variable_name, variable.offset);
                        default_setter_src_dumper.send(STR("}\n\n"));
                    }

                    ini_dumper.send(STR("\n"));
                    default_ini_dumper.send(STR("\n"));
                }

                if (!is_case_preserving_pdb)
                {
                    virtual_src_dumper.send(STR("\n"));

                    // Second & third passes just to separate VTable includes and MemberOffsets includes.
                    if (is_non_case_preserving_pdb)
                    {
                        virtual_src_dumper.send(STR("#ifdef WITH_CASE_PRESERVING_NAME\n"));
                        for (const auto&[class_name, class_entry] : classes.entries)
                        {
                            if (class_entry.variables.empty()) { continue; }

                            if (class_entry.valid_for_member_vars == ValidForMemberVars::Yes)
                            {
                                virtual_src_dumper.send(STR("#include <FunctionBodies/{}_CasePreserving_MemberVariableLayout_DefaultSetter_{}.cpp>\n"), pdb_name, class_name);
                            }
                        }
                        virtual_src_dumper.send(STR("#else\n"));
                    }

                    for (const auto&[class_name, class_entry] : classes.entries)
                    {
                        if (class_entry.variables.empty()) { continue; }

                        if (class_entry.valid_for_member_vars == ValidForMemberVars::Yes)
                        {
                            virtual_src_dumper.send(STR("#include <FunctionBodies/{}_MemberVariableLayout_DefaultSetter_{}.cpp>\n"), pdb_name, class_name);
                        }
                    }

                    if (is_non_case_preserving_pdb)
                    {
                        virtual_src_dumper.send(STR("#endif\n"));
                    }

                    virtual_src_dumper.send(STR("    }\n"));
                    virtual_src_dumper.send(STR("}\n"));
                }
            }

            auto macro_setter_file = std::filesystem::path{STR("MacroSetter.hpp")};
            Output::send(STR("Generating file '{}'\n"), macro_setter_file.wstring());
            Output::Targets<Output::NewFileDevice> macro_setter_dumper;
            auto& macro_setter_file_device = macro_setter_dumper.get_device<Output::NewFileDevice>();
            macro_setter_file_device.set_file_name_and_path(macro_setter_file);
            macro_setter_file_device.set_formatter([](File::StringViewType string) {
                return File::StringType{string};
            });

            for (const auto&[class_name, enum_entry] : g_enum_entries)
            {
                if (enum_entry.variables.empty()) { continue; }

                auto wrapper_header_file = member_variable_layouts_gen_output_include_path / std::format(STR("MemberVariableLayout_HeaderWrapper_{}.hpp"), enum_entry.name_clean);
                Output::send(STR("Generating file '{}'\n"), wrapper_header_file.wstring());
                Output::Targets<Output::NewFileDevice> header_wrapper_dumper;
                auto& wrapper_header_file_device = header_wrapper_dumper.get_device<Output::NewFileDevice>();
                wrapper_header_file_device.set_file_name_and_path(wrapper_header_file);
                wrapper_header_file_device.set_formatter([](File::StringViewType string) {
                    return File::StringType{string};
                });

                auto wrapper_src_file = member_variable_layouts_gen_output_include_path / std::format(STR("MemberVariableLayout_SrcWrapper_{}.hpp"), enum_entry.name_clean);
                Output::send(STR("Generating file '{}'\n"), wrapper_src_file.wstring());
                Output::Targets<Output::NewFileDevice> wrapper_src_dumper;
                auto& wrapper_src_file_device = wrapper_src_dumper.get_device<Output::NewFileDevice>();
                wrapper_src_file_device.set_file_name_and_path(wrapper_src_file);
                wrapper_src_file_device.set_formatter([](File::StringViewType string) {
                    return File::StringType{string};
                });

                header_wrapper_dumper.send(STR("static std::unordered_map<File::StringType, int32_t> MemberOffsets;\n\n"));
                wrapper_src_dumper.send(STR("std::unordered_map<File::StringType, int32_t> {}::MemberOffsets{{}};\n\n"), enum_entry.name);

                auto private_variables_for_class = g_private_variables.find(enum_entry.name);

                for (const auto&[variable_name, variable] : enum_entry.variables)
                {
                    if (variable.type.find(STR("TBaseDelegate")) != variable.type.npos) { continue; }
                    if (variable.type.find(STR("FUniqueNetIdRepl")) != variable.type.npos) { continue; }
                    if (variable.type.find(STR("FPlatformUserId")) != variable.type.npos) { continue; }
                    if (variable.type.find(STR("FVector2D")) != variable.type.npos) { continue; }
                    if (variable.type.find(STR("FReply")) != variable.type.npos) { continue; }

                    bool is_private{private_variables_for_class != g_private_variables.end() && private_variables_for_class->second.find(variable.name) != private_variables_for_class->second.end()};

                    File::StringType final_variable_name = variable.name;
                    File::StringType final_type_name = variable.type;

                    if (variable.name == STR("EnumFlags"))
                    {
                        final_variable_name = STR("EnumFlags_Internal");
                        is_private = true;
                    }

                    if (is_private)
                    {
                        header_wrapper_dumper.send(STR("private:\n"));
                    }
                    else
                    {
                        header_wrapper_dumper.send(STR("public:\n"));
                    }

                    header_wrapper_dumper.send(STR("    {}& Get{}();\n"), variable.type, final_variable_name);
                    header_wrapper_dumper.send(STR("    const {}& Get{}() const;\n\n"), variable.type, final_variable_name);
                    wrapper_src_dumper.send(STR("{}& {}::Get{}()\n"), variable.type, enum_entry.name, final_variable_name);
                    if (enum_entry.name == STR("FArchive") || enum_entry.name == STR("FArchiveState"))
                    {
                        wrapper_src_dumper.send(STR("{\n"));
                        wrapper_src_dumper.send(STR("    static auto& offsets = Version::IsBelow(4, 25) ? FArchive::MemberOffsets : FArchiveState::MemberOffsets;\n"));
                        wrapper_src_dumper.send(STR("    static auto offset = offsets.find(STR(\"{}\"));\n"), final_variable_name);
                        wrapper_src_dumper.send(STR("    if (offset == offsets.end()) {{ throw std::runtime_error{{\"Tried getting member variable '{}::{}' that doesn't exist in this engine version.\"}}; }}\n"), enum_entry.name, final_variable_name);
                        wrapper_src_dumper.send(STR("    return *Helper::Casting::ptr_cast<{}*>(this, offset->second);\n"), variable.type);
                        wrapper_src_dumper.send(STR("}\n"));
                        wrapper_src_dumper.send(STR("const {}& {}::Get{}() const\n"), variable.type, enum_entry.name, final_variable_name);
                        wrapper_src_dumper.send(STR("{\n"));
                        wrapper_src_dumper.send(STR("    static auto& offsets = Version::IsBelow(4, 25) ? FArchive::MemberOffsets : FArchiveState::MemberOffsets;\n"));
                        wrapper_src_dumper.send(STR("    static auto offset = offsets.find(STR(\"{}\"));\n"), final_variable_name);
                        wrapper_src_dumper.send(STR("    if (offset == offsets.end()) {{ throw std::runtime_error{{\"Tried getting member variable '{}::{}' that doesn't exist in this engine version.\"}}; }}\n"), enum_entry.name, final_variable_name);
                        wrapper_src_dumper.send(STR("    return *Helper::Casting::ptr_cast<const {}*>(this, offset->second);\n"), variable.type);
                        wrapper_src_dumper.send(STR("}\n\n"));
                    }
                    else
                    {
                        wrapper_src_dumper.send(STR("{\n"));
                        wrapper_src_dumper.send(STR("    static auto offset = MemberOffsets.find(STR(\"{}\"));\n"), final_variable_name);
                        wrapper_src_dumper.send(STR("    if (offset == MemberOffsets.end()) {{ throw std::runtime_error{{\"Tried getting member variable '{}::{}' that doesn't exist in this engine version.\"}}; }}\n"), enum_entry.name, final_variable_name);
                        wrapper_src_dumper.send(STR("    return *Helper::Casting::ptr_cast<{}*>(this, offset->second);\n"), variable.type);
                        wrapper_src_dumper.send(STR("}\n"));
                        wrapper_src_dumper.send(STR("const {}& {}::Get{}() const\n"), variable.type, enum_entry.name, final_variable_name);
                        wrapper_src_dumper.send(STR("{\n"));
                        wrapper_src_dumper.send(STR("    static auto offset = MemberOffsets.find(STR(\"{}\"));\n"), final_variable_name);
                        wrapper_src_dumper.send(STR("    if (offset == MemberOffsets.end()) {{ throw std::runtime_error{{\"Tried getting member variable '{}::{}' that doesn't exist in this engine version.\"}}; }}\n"), enum_entry.name, final_variable_name);
                        wrapper_src_dumper.send(STR("    return *Helper::Casting::ptr_cast<const {}*>(this, offset->second);\n"), variable.type);
                        wrapper_src_dumper.send(STR("}\n\n"));
                    }

                    macro_setter_dumper.send(STR("if (auto val = parser.get_int64(STR(\"{}\"), STR(\"{}\"), -1); val != -1)\n"), enum_entry.name, final_variable_name);
                    macro_setter_dumper.send(STR("    Unreal::{}::MemberOffsets.emplace(STR(\"{}\"), static_cast<int32_t>(val));\n"), enum_entry.name, final_variable_name);
                }
            }
        }
        else
        {
            if (std::filesystem::exists(sol_bindings_output_path))
            {
                for (const auto& item : std::filesystem::directory_iterator(sol_bindings_output_path))
                {
                    if (item.is_directory()) { continue; }
                    if (item.path().extension() != STR(".hpp") && item.path().extension() != STR(".cpp")) { continue; }

                    File::delete_file(item.path());
                }
            }

            for (const auto& [class_name, enum_entry] : g_enum_entries)
            {
                if (enum_entry.variables.empty()) { continue; }

                auto final_class_name_clean = enum_entry.name_clean;
                // Skipping UObject/UObjectBase because it needs to be manually implemented.
                if (enum_entry.name_clean == STR("UObjectBase")) { continue; }

                auto final_class_name = class_name;

                auto wrapper_header_file = sol_bindings_output_path / std::format(STR("SolBindings_{}.hpp"), final_class_name_clean);
                Output::send(STR("Generating file '{}'\n"), wrapper_header_file.wstring());
                Output::Targets<Output::NewFileDevice> header_wrapper_dumper;
                auto& wrapper_header_file_device = header_wrapper_dumper.get_device<Output::NewFileDevice>();
                wrapper_header_file_device.set_file_name_and_path(wrapper_header_file);
                wrapper_header_file_device.set_formatter([](File::StringViewType string) {
                    return File::StringType{string};
                });

                header_wrapper_dumper.send(STR("auto sol_class_{} = sol().new_usertype<{}>(\"{}\""), final_class_name, final_class_name, final_class_name);
                for (const auto&[variable_name, variable] : enum_entry.variables)
                {
                    if (variable.type.find(STR("TBaseDelegate")) != variable.type.npos) { continue; }
                    if (variable.type.find(STR("FUniqueNetIdRepl")) != variable.type.npos) { continue; }
                    if (variable.type.find(STR("FPlatformUserId")) != variable.type.npos) { continue; }
                    if (variable.type.find(STR("FVector2D")) != variable.type.npos) { continue; }
                    if (variable.type.find(STR("FReply")) != variable.type.npos) { continue; }
                    if (variable.type.find(STR("FUObjectCppClassStaticFunctions")) != variable.type.npos) { continue; }

                    File::StringType final_variable_name = variable.name;

                    if (variable.name == STR("EnumFlags"))
                    {
                        final_variable_name = STR("EnumFlags_Internal");
                    }

                    header_wrapper_dumper.send(STR(",\n    \"Get{}\", static_cast<{}&({}::*)()>(&{}::Get{})"), final_variable_name, variable.type, final_class_name, final_class_name, final_variable_name);
                }
                for (const auto&[pdb_name, classes] : g_class_entries)
                {
                }
                header_wrapper_dumper.send(STR("\n);\n"));
            }
        }
    }

    auto VTableDumper::generate_code(VTableOrMemberVars vtable_or_member_vars) -> void
    {
        setup_symbol_loader();

        std::unordered_map<File::StringType, SymbolNameInfo> vtable_names;
        std::unordered_map<File::StringType, SymbolNameInfo> member_vars_names;
        for (const auto& object_item : s_object_items)
        {
            if (object_item.valid_for_vtable == ValidForVTable::Yes)
            {
                vtable_names.emplace(object_item.name, SymbolNameInfo{object_item.valid_for_vtable, object_item.valid_for_member_vars});
            }

            if (object_item.valid_for_member_vars == ValidForMemberVars::Yes)
            {
                member_vars_names.emplace(object_item.name, SymbolNameInfo{object_item.valid_for_vtable, object_item.valid_for_member_vars});
            }
        }

        if (vtable_or_member_vars == VTableOrMemberVars::VTable)
        {
            dump_vtable_for_symbol(vtable_names);
        }
        else if (vtable_or_member_vars == VTableOrMemberVars::MemberVars)
        {
            dump_member_variable_layouts(member_vars_names);
        }
        else
        {
            dump_vtable_for_symbol(vtable_names);
            dump_member_variable_layouts(member_vars_names);
        }
    }

    auto main(VTableOrMemberVars vtable_or_member_vars) -> void
    {
        TRY([&] {
            {
                VTableDumper vtable_dumper{"PDBs/4_11.pdb"};
                vtable_dumper.generate_code(vtable_or_member_vars);
            }
            CoUninitialize();
        });
        TRY([&] {
            {
                VTableDumper vtable_dumper{"PDBs/4_12.pdb"};
                vtable_dumper.generate_code(vtable_or_member_vars);
            }
            CoUninitialize();
        });
        TRY([&] {
            {
                VTableDumper vtable_dumper{"PDBs/4_13.pdb"};
                vtable_dumper.generate_code(vtable_or_member_vars);
            }
            CoUninitialize();
        });
        TRY([&] {
            {
                VTableDumper vtable_dumper{"PDBs/4_14.pdb"};
                vtable_dumper.generate_code(vtable_or_member_vars);
            }
            CoUninitialize();
        });
        TRY([&] {
            {
                VTableDumper vtable_dumper{"PDBs/4_15.pdb"};
                vtable_dumper.generate_code(vtable_or_member_vars);
            }
            CoUninitialize();
        });
        TRY([&] {
            {
                VTableDumper vtable_dumper{"PDBs/4_16.pdb"};
                vtable_dumper.generate_code(vtable_or_member_vars);
            }
            CoUninitialize();
        });
        TRY([&] {
            {
                VTableDumper vtable_dumper{"PDBs/4_17.pdb"};
                vtable_dumper.generate_code(vtable_or_member_vars);
            }
            CoUninitialize();
        });
        TRY([&] {
            {
                VTableDumper vtable_dumper{"PDBs/4_18.pdb"};
                vtable_dumper.generate_code(vtable_or_member_vars);
            }
            CoUninitialize();
        });
        TRY([&] {
            {
                VTableDumper vtable_dumper{"PDBs/4_19.pdb"};
                vtable_dumper.generate_code(vtable_or_member_vars);
            }
            CoUninitialize();
        });
        TRY([&] {
            {
                VTableDumper vtable_dumper{"PDBs/4_20.pdb"};
                vtable_dumper.generate_code(vtable_or_member_vars);
            }
            CoUninitialize();
        });
        TRY([&] {
            {
                VTableDumper vtable_dumper{"PDBs/4_21.pdb"};
                vtable_dumper.generate_code(vtable_or_member_vars);
            }
            CoUninitialize();
        });
        TRY([&] {
            {
                VTableDumper vtable_dumper{"PDBs/4_22.pdb"};
                vtable_dumper.generate_code(vtable_or_member_vars);
            }
            CoUninitialize();
        });
        TRY([&] {
            {
                VTableDumper vtable_dumper{"PDBs/4_23.pdb"};
                vtable_dumper.generate_code(vtable_or_member_vars);
            }
            CoUninitialize();
        });
        TRY([&] {
            {
                VTableDumper vtable_dumper{"PDBs/4_24.pdb"};
                vtable_dumper.generate_code(vtable_or_member_vars);
            }
            CoUninitialize();
        });
        TRY([&] {
            {
                VTableDumper vtable_dumper{"PDBs/4_25.pdb"};
                vtable_dumper.generate_code(vtable_or_member_vars);
            }
            CoUninitialize();
        });
        TRY([&] {
            {
                VTableDumper vtable_dumper{"PDBs/4_26.pdb"};
                vtable_dumper.generate_code(vtable_or_member_vars);
            }
            CoUninitialize();
        });
        TRY([&] {
            {
                VTableDumper vtable_dumper{"PDBs/4_27.pdb"};
                vtable_dumper.generate_code(vtable_or_member_vars);
            }
            CoUninitialize();
        });
        TRY([&] {
            {
                // WITH_CASE_PRESERVING_NAMES
                VTableDumper vtable_dumper{"PDBs/4_27_CasePreserving.pdb"};
                vtable_dumper.generate_code(vtable_or_member_vars);
            }
            CoUninitialize();
        });
        TRY([&] {
            {
                VTableDumper vtable_dumper{"PDBs/5_00.pdb"};
                vtable_dumper.generate_code(vtable_or_member_vars);
            }
            CoUninitialize();
        });
        TRY([&] {
            {
                VTableDumper vtable_dumper{"PDBs/5_01.pdb"};
                vtable_dumper.generate_code(vtable_or_member_vars);
            }
            CoUninitialize();
        });
        TRY([&] {
            {
                VTableDumper vtable_dumper{"PDBs/5_02.pdb"};
                vtable_dumper.generate_code(vtable_or_member_vars);
            }
            CoUninitialize();
        });

        TRY([&] {
            generate_files(vtable_or_member_vars);
            Output::send(STR("Code generated.\n"));
        });
    }
}
