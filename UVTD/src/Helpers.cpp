#include <format>

#include <UVTD/Helpers.hpp>
#include <Helpers/String.hpp>

namespace RC::UVTD
{
	auto HRESULTToString(HRESULT result) -> std::string
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

	auto is_valid_type_to_dump(File::StringType type_name) -> bool
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

	auto sym_tag_to_string(DWORD sym_tag) -> File::StringViewType
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

	auto base_type_to_string(CComPtr<IDiaSymbol> symbol) -> File::StringType
	{
		DWORD sym_tag;
		symbol->get_symTag(&sym_tag);
		if (sym_tag != SymTagBaseType)
		{
			throw std::runtime_error{ "base_type_to_string only works on SymTagBaseType" };
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
			throw std::runtime_error{ "Unsupported SymTagBaseType type." };
			break;
		}

		return name;
	}

	auto kind_to_string(DWORD kind) -> File::StringViewType
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


	auto generate_pointer_type(CComPtr<IDiaSymbol> symbol) -> File::StringType
	{
		HRESULT hr;
		DWORD sym_tag;
		symbol->get_symTag(&sym_tag);
		CComPtr<IDiaSymbol> real_symbol;

		if (sym_tag == SymTagFunctionType)
		{
			if (hr = symbol->get_type(&real_symbol); hr != S_OK)
			{
				throw std::runtime_error{ std::format("Call to 'get_type()' failed with error '{}'", HRESULTToString(hr)) };
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
			throw std::runtime_error{ std::format("Call to 'get_reference(&is_reference)' failed with error '{}'", HRESULTToString(hr)) };
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

	auto get_symbol_name(CComPtr<IDiaSymbol> symbol) -> File::StringType
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
				throw std::runtime_error{ std::format("Call to 'get_type()' failed with error '{}'", HRESULTToString(hr)) };
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

	auto get_type_name(CComPtr<IDiaSymbol> type) -> File::StringType
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
			throw std::runtime_error{ to_string(std::format(STR("Unknown type: {}."), type_tag)) };
		}
	}
}