if (auto val = parser.get_int64(STR("UObjectBase"), STR("ObjectFlags"), -1); val != -1)
    Unreal::UObjectBase::MemberOffsets.emplace(STR("ObjectFlags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UObjectBase"), STR("InternalIndex"), -1); val != -1)
    Unreal::UObjectBase::MemberOffsets.emplace(STR("InternalIndex"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UObjectBase"), STR("Class"), -1); val != -1)
    Unreal::UObjectBase::MemberOffsets.emplace(STR("ClassPrivate"), static_cast<int32_t>(val));
// Also support using the renamed version in the INI file
if (auto val = parser.get_int64(STR("UObjectBase"), STR("ClassPrivate"), -1); val != -1)
    Unreal::UObjectBase::MemberOffsets.emplace(STR("ClassPrivate"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UObjectBase"), STR("Name"), -1); val != -1)
    Unreal::UObjectBase::MemberOffsets.emplace(STR("NamePrivate"), static_cast<int32_t>(val));
// Also support using the renamed version in the INI file
if (auto val = parser.get_int64(STR("UObjectBase"), STR("NamePrivate"), -1); val != -1)
    Unreal::UObjectBase::MemberOffsets.emplace(STR("NamePrivate"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UObjectBase"), STR("Outer"), -1); val != -1)
    Unreal::UObjectBase::MemberOffsets.emplace(STR("OuterPrivate"), static_cast<int32_t>(val));
// Also support using the renamed version in the INI file
if (auto val = parser.get_int64(STR("UObjectBase"), STR("OuterPrivate"), -1); val != -1)
    Unreal::UObjectBase::MemberOffsets.emplace(STR("OuterPrivate"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UScriptStruct::ICppStructOps"), STR("Size"), -1); val != -1)
    Unreal::UScriptStruct::ICppStructOps::MemberOffsets.emplace(STR("Size"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UScriptStruct::ICppStructOps"), STR("Alignment"), -1); val != -1)
    Unreal::UScriptStruct::ICppStructOps::MemberOffsets.emplace(STR("Alignment"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UStruct"), STR("SuperStruct"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("SuperStruct"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UStruct"), STR("Children"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("Children"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UStruct"), STR("PropertiesSize"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("PropertiesSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UStruct"), STR("MinAlignment"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("MinAlignment"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UStruct"), STR("Script"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("Script"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UStruct"), STR("PropertyLink"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("PropertyLink"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UStruct"), STR("RefLink"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("RefLink"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UStruct"), STR("DestructorLink"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("DestructorLink"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UStruct"), STR("PostConstructLink"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("PostConstructLink"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UStruct"), STR("ScriptObjectReferences"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("ScriptObjectReferences"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UStruct"), STR("ChildProperties"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("ChildProperties"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UStruct"), STR("ScriptAndPropertyObjectReferences"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("ScriptAndPropertyObjectReferences"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UStruct"), STR("UnresolvedScriptProperties"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("UnresolvedScriptProperties"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UStruct"), STR("StructStateFlags"), -1); val != -1)
    Unreal::UStruct::MemberOffsets.emplace(STR("StructStateFlags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UScriptStruct"), STR("StructFlags"), -1); val != -1)
    Unreal::UScriptStruct::MemberOffsets.emplace(STR("StructFlags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UScriptStruct"), STR("bCppStructOpsFromBaseClass"), -1); val != -1)
    Unreal::UScriptStruct::MemberOffsets.emplace(STR("bCppStructOpsFromBaseClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UScriptStruct"), STR("bPrepareCppStructOpsCompleted"), -1); val != -1)
    Unreal::UScriptStruct::MemberOffsets.emplace(STR("bPrepareCppStructOpsCompleted"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UScriptStruct"), STR("CppStructOps"), -1); val != -1)
    Unreal::UScriptStruct::MemberOffsets.emplace(STR("CppStructOps"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("ClassConstructor"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("ClassConstructor"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("ClassVTableHelperCtorCaller"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("ClassVTableHelperCtorCaller"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("ClassAddReferencedObjects"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("ClassAddReferencedObjects"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("ClassUnique"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("ClassUnique"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("ClassFlags"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("ClassFlags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("ClassCastFlags"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("ClassCastFlags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("ClassWithin"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("ClassWithin"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("ClassGeneratedBy"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("ClassGeneratedBy"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("ClassConfigName"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("ClassConfigName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("bCooked"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("bCooked"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("NetFields"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("NetFields"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("ClassDefaultObject"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("ClassDefaultObject"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("FuncMap"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("FuncMap"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("Interfaces"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("Interfaces"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("ParentFuncMap"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("ParentFuncMap"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("InterfaceFuncMap"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("InterfaceFuncMap"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("SuperFuncMap"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("SuperFuncMap"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("UberGraphFramePointerProperty"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("UberGraphFramePointerProperty"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("SparseClassData"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("SparseClassData"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("SparseClassDataStruct"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("SparseClassDataStruct"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("FirstOwnedClassRep"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("FirstOwnedClassRep"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("bLayoutChanging"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("bLayoutChanging"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("AllFunctionsCache"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("AllFunctionsCache"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("ReferenceSchema"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("ReferenceSchema"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("FuncMapLock"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("FuncMapLock"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UClass"), STR("AllFunctionsCacheLock"), -1); val != -1)
    Unreal::UClass::MemberOffsets.emplace(STR("AllFunctionsCacheLock"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEnum"), STR("CppType"), -1); val != -1)
    Unreal::UEnum::MemberOffsets.emplace(STR("CppType"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEnum"), STR("Names"), -1); val != -1)
    Unreal::UEnum::MemberOffsets.emplace(STR("Names"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEnum"), STR("CppForm"), -1); val != -1)
    Unreal::UEnum::MemberOffsets.emplace(STR("CppForm"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEnum"), STR("EnumDisplayNameFn"), -1); val != -1)
    Unreal::UEnum::MemberOffsets.emplace(STR("EnumDisplayNameFn"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEnum"), STR("EnumFlags"), -1); val != -1)
    Unreal::UEnum::MemberOffsets.emplace(STR("EnumFlags_Internal"), static_cast<int32_t>(val));
// Also support using the renamed version in the INI file
if (auto val = parser.get_int64(STR("UEnum"), STR("EnumFlags_Internal"), -1); val != -1)
    Unreal::UEnum::MemberOffsets.emplace(STR("EnumFlags_Internal"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEnum"), STR("EnumPackage"), -1); val != -1)
    Unreal::UEnum::MemberOffsets.emplace(STR("EnumPackage"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UFunction"), STR("FunctionFlags"), -1); val != -1)
    Unreal::UFunction::MemberOffsets.emplace(STR("FunctionFlags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UFunction"), STR("RepOffset"), -1); val != -1)
    Unreal::UFunction::MemberOffsets.emplace(STR("RepOffset"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UFunction"), STR("NumParms"), -1); val != -1)
    Unreal::UFunction::MemberOffsets.emplace(STR("NumParms"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UFunction"), STR("ParmsSize"), -1); val != -1)
    Unreal::UFunction::MemberOffsets.emplace(STR("ParmsSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UFunction"), STR("ReturnValueOffset"), -1); val != -1)
    Unreal::UFunction::MemberOffsets.emplace(STR("ReturnValueOffset"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UFunction"), STR("RPCId"), -1); val != -1)
    Unreal::UFunction::MemberOffsets.emplace(STR("RPCId"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UFunction"), STR("RPCResponseId"), -1); val != -1)
    Unreal::UFunction::MemberOffsets.emplace(STR("RPCResponseId"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UFunction"), STR("FirstPropertyToInit"), -1); val != -1)
    Unreal::UFunction::MemberOffsets.emplace(STR("FirstPropertyToInit"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UFunction"), STR("EventGraphFunction"), -1); val != -1)
    Unreal::UFunction::MemberOffsets.emplace(STR("EventGraphFunction"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UFunction"), STR("EventGraphCallOffset"), -1); val != -1)
    Unreal::UFunction::MemberOffsets.emplace(STR("EventGraphCallOffset"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UFunction"), STR("Func"), -1); val != -1)
    Unreal::UFunction::MemberOffsets.emplace(STR("Func"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UField"), STR("Next"), -1); val != -1)
    Unreal::UField::MemberOffsets.emplace(STR("Next"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FField"), STR("ClassPrivate"), -1); val != -1)
    Unreal::FField::MemberOffsets.emplace(STR("ClassPrivate"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FField"), STR("Owner"), -1); val != -1)
    Unreal::FField::MemberOffsets.emplace(STR("Owner"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FField"), STR("Next"), -1); val != -1)
    Unreal::FField::MemberOffsets.emplace(STR("Next"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FField"), STR("NamePrivate"), -1); val != -1)
    Unreal::FField::MemberOffsets.emplace(STR("NamePrivate"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FField"), STR("FlagsPrivate"), -1); val != -1)
    Unreal::FField::MemberOffsets.emplace(STR("FlagsPrivate"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FProperty"), STR("ArrayDim"), -1); val != -1)
    Unreal::FProperty::MemberOffsets.emplace(STR("ArrayDim"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FProperty"), STR("ElementSize"), -1); val != -1)
    Unreal::FProperty::MemberOffsets.emplace(STR("ElementSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FProperty"), STR("PropertyFlags"), -1); val != -1)
    Unreal::FProperty::MemberOffsets.emplace(STR("PropertyFlags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FProperty"), STR("RepIndex"), -1); val != -1)
    Unreal::FProperty::MemberOffsets.emplace(STR("RepIndex"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FProperty"), STR("RepNotifyFunc"), -1); val != -1)
    Unreal::FProperty::MemberOffsets.emplace(STR("RepNotifyFunc"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FProperty"), STR("Offset_Internal"), -1); val != -1)
    Unreal::FProperty::MemberOffsets.emplace(STR("Offset_Internal"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FProperty"), STR("PropertyLinkNext"), -1); val != -1)
    Unreal::FProperty::MemberOffsets.emplace(STR("PropertyLinkNext"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FProperty"), STR("NextRef"), -1); val != -1)
    Unreal::FProperty::MemberOffsets.emplace(STR("NextRef"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FProperty"), STR("DestructorLinkNext"), -1); val != -1)
    Unreal::FProperty::MemberOffsets.emplace(STR("DestructorLinkNext"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FProperty"), STR("PostConstructLinkNext"), -1); val != -1)
    Unreal::FProperty::MemberOffsets.emplace(STR("PostConstructLinkNext"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FObjectPropertyBase"), STR("PropertyClass"), -1); val != -1)
    Unreal::FObjectPropertyBase::MemberOffsets.emplace(STR("PropertyClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FStructProperty"), STR("Struct"), -1); val != -1)
    Unreal::FStructProperty::MemberOffsets.emplace(STR("Struct"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArrayProperty"), STR("Inner"), -1); val != -1)
    Unreal::FArrayProperty::MemberOffsets.emplace(STR("Inner"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArrayProperty"), STR("ArrayFlags"), -1); val != -1)
    Unreal::FArrayProperty::MemberOffsets.emplace(STR("ArrayFlags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FMapProperty"), STR("KeyProp"), -1); val != -1)
    Unreal::FMapProperty::MemberOffsets.emplace(STR("KeyProp"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FMapProperty"), STR("ValueProp"), -1); val != -1)
    Unreal::FMapProperty::MemberOffsets.emplace(STR("ValueProp"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FMapProperty"), STR("MapLayout"), -1); val != -1)
    Unreal::FMapProperty::MemberOffsets.emplace(STR("MapLayout"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FMapProperty"), STR("MapFlags"), -1); val != -1)
    Unreal::FMapProperty::MemberOffsets.emplace(STR("MapFlags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FSetProperty"), STR("ElementProp"), -1); val != -1)
    Unreal::FSetProperty::MemberOffsets.emplace(STR("ElementProp"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FSetProperty"), STR("SetLayout"), -1); val != -1)
    Unreal::FSetProperty::MemberOffsets.emplace(STR("SetLayout"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FBoolProperty"), STR("FieldSize"), -1); val != -1)
    Unreal::FBoolProperty::MemberOffsets.emplace(STR("FieldSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FBoolProperty"), STR("ByteOffset"), -1); val != -1)
    Unreal::FBoolProperty::MemberOffsets.emplace(STR("ByteOffset"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FBoolProperty"), STR("ByteMask"), -1); val != -1)
    Unreal::FBoolProperty::MemberOffsets.emplace(STR("ByteMask"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FBoolProperty"), STR("FieldMask"), -1); val != -1)
    Unreal::FBoolProperty::MemberOffsets.emplace(STR("FieldMask"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FByteProperty"), STR("Enum"), -1); val != -1)
    Unreal::FByteProperty::MemberOffsets.emplace(STR("Enum"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FEnumProperty"), STR("UnderlyingProp"), -1); val != -1)
    Unreal::FEnumProperty::MemberOffsets.emplace(STR("UnderlyingProp"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FEnumProperty"), STR("Enum"), -1); val != -1)
    Unreal::FEnumProperty::MemberOffsets.emplace(STR("Enum"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FClassProperty"), STR("MetaClass"), -1); val != -1)
    Unreal::FClassProperty::MemberOffsets.emplace(STR("MetaClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FSoftClassProperty"), STR("MetaClass"), -1); val != -1)
    Unreal::FSoftClassProperty::MemberOffsets.emplace(STR("MetaClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FDelegateProperty"), STR("SignatureFunction"), -1); val != -1)
    Unreal::FDelegateProperty::MemberOffsets.emplace(STR("SignatureFunction"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FMulticastDelegateProperty"), STR("SignatureFunction"), -1); val != -1)
    Unreal::FMulticastDelegateProperty::MemberOffsets.emplace(STR("SignatureFunction"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FInterfaceProperty"), STR("InterfaceClass"), -1); val != -1)
    Unreal::FInterfaceProperty::MemberOffsets.emplace(STR("InterfaceClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FFieldPathProperty"), STR("PropertyClass"), -1); val != -1)
    Unreal::FFieldPathProperty::MemberOffsets.emplace(STR("PropertyClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FWorldContext"), STR("ContextHandle"), -1); val != -1)
    Unreal::FWorldContext::MemberOffsets.emplace(STR("ContextHandle"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FWorldContext"), STR("TravelURL"), -1); val != -1)
    Unreal::FWorldContext::MemberOffsets.emplace(STR("TravelURL"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FWorldContext"), STR("TravelType"), -1); val != -1)
    Unreal::FWorldContext::MemberOffsets.emplace(STR("TravelType"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FWorldContext"), STR("LastURL"), -1); val != -1)
    Unreal::FWorldContext::MemberOffsets.emplace(STR("LastURL"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FWorldContext"), STR("LastRemoteURL"), -1); val != -1)
    Unreal::FWorldContext::MemberOffsets.emplace(STR("LastRemoteURL"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FWorldContext"), STR("LevelsToLoadForPendingMapChange"), -1); val != -1)
    Unreal::FWorldContext::MemberOffsets.emplace(STR("LevelsToLoadForPendingMapChange"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FWorldContext"), STR("PendingMapChangeFailureDescription"), -1); val != -1)
    Unreal::FWorldContext::MemberOffsets.emplace(STR("PendingMapChangeFailureDescription"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FWorldContext"), STR("bShouldCommitPendingMapChange"), -1); val != -1)
    Unreal::FWorldContext::MemberOffsets.emplace(STR("bShouldCommitPendingMapChange"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FWorldContext"), STR("GameViewport"), -1); val != -1)
    Unreal::FWorldContext::MemberOffsets.emplace(STR("GameViewport"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FWorldContext"), STR("PIEInstance"), -1); val != -1)
    Unreal::FWorldContext::MemberOffsets.emplace(STR("PIEInstance"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FWorldContext"), STR("PIEPrefix"), -1); val != -1)
    Unreal::FWorldContext::MemberOffsets.emplace(STR("PIEPrefix"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FWorldContext"), STR("PIERemapPrefix"), -1); val != -1)
    Unreal::FWorldContext::MemberOffsets.emplace(STR("PIERemapPrefix"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FWorldContext"), STR("RunAsDedicated"), -1); val != -1)
    Unreal::FWorldContext::MemberOffsets.emplace(STR("RunAsDedicated"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FWorldContext"), STR("bWaitingOnOnlineSubsystem"), -1); val != -1)
    Unreal::FWorldContext::MemberOffsets.emplace(STR("bWaitingOnOnlineSubsystem"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FWorldContext"), STR("AudioDeviceHandle"), -1); val != -1)
    Unreal::FWorldContext::MemberOffsets.emplace(STR("AudioDeviceHandle"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FWorldContext"), STR("ExternalReferences"), -1); val != -1)
    Unreal::FWorldContext::MemberOffsets.emplace(STR("ExternalReferences"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FWorldContext"), STR("ThisCurrentWorld"), -1); val != -1)
    Unreal::FWorldContext::MemberOffsets.emplace(STR("ThisCurrentWorld"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FWorldContext"), STR("CustomDescription"), -1); val != -1)
    Unreal::FWorldContext::MemberOffsets.emplace(STR("CustomDescription"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FWorldContext"), STR("AudioDeviceID"), -1); val != -1)
    Unreal::FWorldContext::MemberOffsets.emplace(STR("AudioDeviceID"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FWorldContext"), STR("PIEFixedTickSeconds"), -1); val != -1)
    Unreal::FWorldContext::MemberOffsets.emplace(STR("PIEFixedTickSeconds"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FWorldContext"), STR("PIEAccumulatedTickSeconds"), -1); val != -1)
    Unreal::FWorldContext::MemberOffsets.emplace(STR("PIEAccumulatedTickSeconds"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FWorldContext"), STR("bIsPrimaryPIEInstance"), -1); val != -1)
    Unreal::FWorldContext::MemberOffsets.emplace(STR("bIsPrimaryPIEInstance"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FWorldContext"), STR("GarbageObjectsToVerify"), -1); val != -1)
    Unreal::FWorldContext::MemberOffsets.emplace(STR("GarbageObjectsToVerify"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FOutputDevice"), STR("bSuppressEventTag"), -1); val != -1)
    Unreal::FOutputDevice::MemberOffsets.emplace(STR("bSuppressEventTag"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FOutputDevice"), STR("bAutoEmitLineTerminator"), -1); val != -1)
    Unreal::FOutputDevice::MemberOffsets.emplace(STR("bAutoEmitLineTerminator"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArIsLoading"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIsLoading"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArIsSaving"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIsSaving"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArIsTransacting"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIsTransacting"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArIsTextFormat"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIsTextFormat"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArWantBinaryPropertySerialization"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArWantBinaryPropertySerialization"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArUseUnversionedPropertySerialization"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArUseUnversionedPropertySerialization"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArForceUnicode"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArForceUnicode"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArIsPersistent"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIsPersistent"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArIsError"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIsError"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArIsCriticalError"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIsCriticalError"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArContainsCode"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArContainsCode"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArContainsMap"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArContainsMap"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArRequiresLocalizationGather"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArRequiresLocalizationGather"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArForceByteSwapping"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArForceByteSwapping"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArIgnoreArchetypeRef"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIgnoreArchetypeRef"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArNoDelta"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArNoDelta"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArNoIntraPropertyDelta"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArNoIntraPropertyDelta"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArIgnoreOuterRef"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIgnoreOuterRef"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArIgnoreClassGeneratedByRef"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIgnoreClassGeneratedByRef"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArIgnoreClassRef"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIgnoreClassRef"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArAllowLazyLoading"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArAllowLazyLoading"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArIsObjectReferenceCollector"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIsObjectReferenceCollector"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArIsModifyingWeakAndStrongReferences"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIsModifyingWeakAndStrongReferences"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArIsCountingMemory"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIsCountingMemory"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArShouldSkipBulkData"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArShouldSkipBulkData"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArIsFilterEditorOnly"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIsFilterEditorOnly"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArIsSaveGame"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIsSaveGame"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArIsNetArchive"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIsNetArchive"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArUseCustomPropertyList"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArUseCustomPropertyList"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArSerializingDefaults"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArSerializingDefaults"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArPortFlags"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArPortFlags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArMaxSerializeSize"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArMaxSerializeSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArUE4Ver"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArUE4Ver"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArLicenseeUE4Ver"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArLicenseeUE4Ver"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArEngineVer"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArEngineVer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArEngineNetVer"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArEngineNetVer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArGameNetVer"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArGameNetVer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("CustomVersionContainer"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("CustomVersionContainer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArCustomPropertyList"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArCustomPropertyList"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("CookingTargetPlatform"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("CookingTargetPlatform"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("SerializedProperty"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("SerializedProperty"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("bCustomVersionsAreReset"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("bCustomVersionsAreReset"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("NextProxy"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("NextProxy"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArIsLoadingFromCookedPackage"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArIsLoadingFromCookedPackage"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArShouldSkipCompilingAssets"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArShouldSkipCompilingAssets"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArUEVer"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArUEVer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArLicenseeUEVer"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArLicenseeUEVer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArShouldSkipUpdateCustomVersion"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArShouldSkipUpdateCustomVersion"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArMergeOverrides"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArMergeOverrides"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("SavePackageData"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("SavePackageData"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchiveState"), STR("ArPreserveArrayElements"), -1); val != -1)
    Unreal::FArchiveState::MemberOffsets.emplace(STR("ArPreserveArrayElements"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArNetVer"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArNetVer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArUE4Ver"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArUE4Ver"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArLicenseeUE4Ver"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArLicenseeUE4Ver"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArEngineVer"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArEngineVer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("CustomVersionContainer"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("CustomVersionContainer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArIsLoading"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIsLoading"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArIsSaving"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIsSaving"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArIsTransacting"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIsTransacting"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArWantBinaryPropertySerialization"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArWantBinaryPropertySerialization"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArForceUnicode"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArForceUnicode"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArIsPersistent"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIsPersistent"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArIsError"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIsError"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArIsCriticalError"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIsCriticalError"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArContainsCode"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArContainsCode"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArContainsMap"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArContainsMap"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArRequiresLocalizationGather"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArRequiresLocalizationGather"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArForceByteSwapping"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArForceByteSwapping"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArIgnoreArchetypeRef"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIgnoreArchetypeRef"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArNoDelta"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArNoDelta"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArIgnoreOuterRef"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIgnoreOuterRef"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArIgnoreClassGeneratedByRef"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIgnoreClassGeneratedByRef"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArIgnoreClassRef"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIgnoreClassRef"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArAllowLazyLoading"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArAllowLazyLoading"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArIsObjectReferenceCollector"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIsObjectReferenceCollector"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArIsModifyingWeakAndStrongReferences"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIsModifyingWeakAndStrongReferences"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArIsCountingMemory"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIsCountingMemory"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArShouldSkipBulkData"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArShouldSkipBulkData"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArIsFilterEditorOnly"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIsFilterEditorOnly"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArIsSaveGame"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIsSaveGame"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArSerializingDefaults"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArSerializingDefaults"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArPortFlags"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArPortFlags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArMaxSerializeSize"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArMaxSerializeSize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("CookingTargetPlatform"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("CookingTargetPlatform"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("SerializedProperty"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("SerializedProperty"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("bCustomVersionsAreReset"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("bCustomVersionsAreReset"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArCustomPropertyList"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArCustomPropertyList"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArUseCustomPropertyList"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArUseCustomPropertyList"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArEngineNetVer"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArEngineNetVer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArGameNetVer"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArGameNetVer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArIsTextFormat"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIsTextFormat"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArIsNetArchive"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArIsNetArchive"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("FArchive"), STR("ArNoIntraPropertyDelta"), -1); val != -1)
    Unreal::FArchive::MemberOffsets.emplace(STR("ArNoIntraPropertyDelta"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("PrimaryActorTick"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("PrimaryActorTick"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("CustomTimeDilation"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("CustomTimeDilation"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bHidden"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bHidden"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bNetTemporary"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bNetTemporary"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bNetStartup"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bNetStartup"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bOnlyRelevantToOwner"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bOnlyRelevantToOwner"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bAlwaysRelevant"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bAlwaysRelevant"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bReplicateMovement"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bReplicateMovement"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bTearOff"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bTearOff"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bExchangedRoles"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bExchangedRoles"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bPendingNetUpdate"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bPendingNetUpdate"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bNetLoadOnClient"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bNetLoadOnClient"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bNetUseOwnerRelevancy"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bNetUseOwnerRelevancy"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bBlockInput"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bBlockInput"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bRunningUserConstructionScript"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bRunningUserConstructionScript"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bHasFinishedSpawning"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bHasFinishedSpawning"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bActorEnableCollision"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bActorEnableCollision"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bReplicates"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bReplicates"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("RemoteRole"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("RemoteRole"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("Owner"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("Owner"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("ReplicatedMovement"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("ReplicatedMovement"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("AttachmentReplication"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("AttachmentReplication"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("Role"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("Role"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("NetDormancy"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("NetDormancy"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("AutoReceiveInput"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("AutoReceiveInput"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("InputPriority"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("InputPriority"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("InputComponent"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("InputComponent"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("InputConsumeOption_DEPRECATED"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("InputConsumeOption_DEPRECATED"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("NetCullDistanceSquared"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("NetCullDistanceSquared"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("NetTag"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("NetTag"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("NetUpdateTime"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("NetUpdateTime"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("NetUpdateFrequency"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("NetUpdateFrequency"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("NetPriority"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("NetPriority"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("LastNetUpdateTime"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("LastNetUpdateTime"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("NetDriverName"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("NetDriverName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bAutoDestroyWhenFinished"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bAutoDestroyWhenFinished"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bCanBeDamaged"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bCanBeDamaged"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bActorIsBeingDestroyed"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bActorIsBeingDestroyed"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bCollideWhenPlacing"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bCollideWhenPlacing"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bFindCameraComponentWhenViewTarget"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bFindCameraComponentWhenViewTarget"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bRelevantForNetworkReplays"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bRelevantForNetworkReplays"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("SpawnCollisionHandlingMethod"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("SpawnCollisionHandlingMethod"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("CreationTime"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("CreationTime"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("Children"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("Children"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("RootComponent"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("RootComponent"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("ControllingMatineeActors"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("ControllingMatineeActors"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("InitialLifeSpan"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("InitialLifeSpan"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("TimerHandle_LifeSpanExpired"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("TimerHandle_LifeSpanExpired"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bAllowReceiveTickEventOnDedicatedServer"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bAllowReceiveTickEventOnDedicatedServer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("Layers"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("Layers"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("ParentComponentActor"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("ParentComponentActor"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bActorInitialized"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bActorInitialized"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bActorHasBegunPlay"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bActorHasBegunPlay"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bActorSeamlessTraveled"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bActorSeamlessTraveled"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bIgnoresOriginShifting"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bIgnoresOriginShifting"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bEnableAutoLODGeneration"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bEnableAutoLODGeneration"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("Tags"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("Tags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("HiddenEditorViews"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("HiddenEditorViews"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("OnTakeAnyDamage"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnTakeAnyDamage"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("OnTakePointDamage"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnTakePointDamage"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("OnActorBeginOverlap"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnActorBeginOverlap"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("OnActorEndOverlap"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnActorEndOverlap"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("OnBeginCursorOver"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnBeginCursorOver"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("OnEndCursorOver"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnEndCursorOver"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("OnClicked"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnClicked"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("OnReleased"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnReleased"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("OnInputTouchBegin"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnInputTouchBegin"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("OnInputTouchEnd"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnInputTouchEnd"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("OnInputTouchEnter"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnInputTouchEnter"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("OnInputTouchLeave"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnInputTouchLeave"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("OnActorHit"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnActorHit"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("OnDestroyed"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnDestroyed"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("OnEndPlay"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnEndPlay"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("DetachFence"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("DetachFence"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bAllowTickBeforeBeginPlay"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bAllowTickBeforeBeginPlay"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bTickFunctionsRegistered"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bTickFunctionsRegistered"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bNetCheckedInitialPhysicsState"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bNetCheckedInitialPhysicsState"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("ParentComponentActor_DEPRECATED"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("ParentComponentActor_DEPRECATED"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("ParentComponent"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("ParentComponent"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("ActorHasBegunPlay"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("ActorHasBegunPlay"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("MinNetUpdateFrequency"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("MinNetUpdateFrequency"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bGenerateOverlapEventsDuringLevelStreaming"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bGenerateOverlapEventsDuringLevelStreaming"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bHasDeferredComponentRegistration"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bHasDeferredComponentRegistration"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bCanBeInCluster"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bCanBeInCluster"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bReplayRewindable"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bReplayRewindable"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bIsEditorOnlyActor"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bIsEditorOnlyActor"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bActorWantsDestroyDuringBeginPlay"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bActorWantsDestroyDuringBeginPlay"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("OnTakeRadialDamage"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("OnTakeRadialDamage"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("CachedLastRenderTime"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("CachedLastRenderTime"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bRelevantForLevelBounds"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bRelevantForLevelBounds"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("LastRenderTime"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("LastRenderTime"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bActorBeginningPlayFromLevelStreaming"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bActorBeginningPlayFromLevelStreaming"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("UpdateOverlapsMethodDuringLevelStreaming"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("UpdateOverlapsMethodDuringLevelStreaming"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("DefaultUpdateOverlapsMethodDuringLevelStreaming"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("DefaultUpdateOverlapsMethodDuringLevelStreaming"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bActorIsBeingConstructed"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bActorIsBeingConstructed"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bForceNetAddressable"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bForceNetAddressable"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bCallPreReplication"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bCallPreReplication"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bCallPreReplicationForReplay"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bCallPreReplicationForReplay"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("RayTracingGroupId"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("RayTracingGroupId"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bReplicateUsingRegisteredSubObjectList"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bReplicateUsingRegisteredSubObjectList"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bHasRegisteredAllComponents"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bHasRegisteredAllComponents"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bAsyncPhysicsTickEnabled"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bAsyncPhysicsTickEnabled"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("ReplicatedSubObjects"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("ReplicatedSubObjects"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("ReplicatedComponentsInfo"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("ReplicatedComponentsInfo"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("PhysicsReplicationMode"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("PhysicsReplicationMode"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("ActorCategory"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("ActorCategory"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AActor"), STR("bHasPreRegisteredAllComponents"), -1); val != -1)
    Unreal::AActor::MemberOffsets.emplace(STR("bHasPreRegisteredAllComponents"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameModeBase"), STR("OptionsString"), -1); val != -1)
    Unreal::AGameModeBase::MemberOffsets.emplace(STR("OptionsString"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameModeBase"), STR("GameSessionClass"), -1); val != -1)
    Unreal::AGameModeBase::MemberOffsets.emplace(STR("GameSessionClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameModeBase"), STR("PlayerStateClass"), -1); val != -1)
    Unreal::AGameModeBase::MemberOffsets.emplace(STR("PlayerStateClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameModeBase"), STR("HUDClass"), -1); val != -1)
    Unreal::AGameModeBase::MemberOffsets.emplace(STR("HUDClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameModeBase"), STR("SpectatorClass"), -1); val != -1)
    Unreal::AGameModeBase::MemberOffsets.emplace(STR("SpectatorClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameModeBase"), STR("GameSession"), -1); val != -1)
    Unreal::AGameModeBase::MemberOffsets.emplace(STR("GameSession"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameModeBase"), STR("DefaultPlayerName"), -1); val != -1)
    Unreal::AGameModeBase::MemberOffsets.emplace(STR("DefaultPlayerName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameModeBase"), STR("bUseSeamlessTravel"), -1); val != -1)
    Unreal::AGameModeBase::MemberOffsets.emplace(STR("bUseSeamlessTravel"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameModeBase"), STR("bStartPlayersAsSpectators"), -1); val != -1)
    Unreal::AGameModeBase::MemberOffsets.emplace(STR("bStartPlayersAsSpectators"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameModeBase"), STR("bPauseable"), -1); val != -1)
    Unreal::AGameModeBase::MemberOffsets.emplace(STR("bPauseable"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameModeBase"), STR("ServerStatReplicatorClass"), -1); val != -1)
    Unreal::AGameModeBase::MemberOffsets.emplace(STR("ServerStatReplicatorClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameModeBase"), STR("ServerStatReplicator"), -1); val != -1)
    Unreal::AGameModeBase::MemberOffsets.emplace(STR("ServerStatReplicator"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameModeBase"), STR("GameNetDriverReplicationSystem"), -1); val != -1)
    Unreal::AGameModeBase::MemberOffsets.emplace(STR("GameNetDriverReplicationSystem"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("MatchState"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("MatchState"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("bUseSeamlessTravel"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("bUseSeamlessTravel"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("bPauseable"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("bPauseable"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("bStartPlayersAsSpectators"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("bStartPlayersAsSpectators"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("bDelayedStart"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("bDelayedStart"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("OptionsString"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("OptionsString"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("HUDClass"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("HUDClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("NumSpectators"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("NumSpectators"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("NumPlayers"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("NumPlayers"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("NumBots"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("NumBots"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("MinRespawnDelay"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("MinRespawnDelay"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("GameSession"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("GameSession"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("NumTravellingPlayers"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("NumTravellingPlayers"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("CurrentID"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("CurrentID"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("DefaultPlayerName"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("DefaultPlayerName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("EngineMessageClass"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("EngineMessageClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("SpectatorClass"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("SpectatorClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("PlayerStateClass"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("PlayerStateClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("InactivePlayerArray"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("InactivePlayerArray"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("GameModeClassAliases"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("GameModeClassAliases"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("InactivePlayerStateLifeSpan"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("InactivePlayerStateLifeSpan"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("bHandleDedicatedServerReplays"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("bHandleDedicatedServerReplays"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("AGameMode"), STR("MaxInactivePlayers"), -1); val != -1)
    Unreal::AGameMode::MemberOffsets.emplace(STR("MaxInactivePlayers"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("TinyFont"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("TinyFont"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("TinyFontName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("TinyFontName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("SmallFont"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("SmallFont"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("SmallFontName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("SmallFontName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("MediumFont"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("MediumFont"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("MediumFontName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("MediumFontName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("LargeFont"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("LargeFont"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("LargeFontName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("LargeFontName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("SubtitleFont"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("SubtitleFont"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("SubtitleFontName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("SubtitleFontName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("AdditionalFonts"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("AdditionalFonts"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("ActiveMatinee"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("ActiveMatinee"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("AdditionalFontNames"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("AdditionalFontNames"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("ConsoleClass"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("ConsoleClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("ConsoleClassName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("ConsoleClassName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("GameViewportClientClass"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("GameViewportClientClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("GameViewportClientClassName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("GameViewportClientClassName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("LocalPlayerClass"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("LocalPlayerClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("LocalPlayerClassName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("LocalPlayerClassName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("WorldSettingsClass"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("WorldSettingsClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("WorldSettingsClassName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("WorldSettingsClassName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("NavigationSystemClassName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("NavigationSystemClassName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("AvoidanceManagerClassName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("AvoidanceManagerClassName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("PhysicsCollisionHandlerClassName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("PhysicsCollisionHandlerClassName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("GameUserSettingsClassName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("GameUserSettingsClassName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("GameUserSettingsClass"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("GameUserSettingsClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("AIControllerClassName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("AIControllerClassName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("GameUserSettings"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("GameUserSettings"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("LevelScriptActorClass"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("LevelScriptActorClass"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("LevelScriptActorClassName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("LevelScriptActorClassName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("DefaultBlueprintBaseClassName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("DefaultBlueprintBaseClassName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("GameSingletonClassName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("GameSingletonClassName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("GameSingleton"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("GameSingleton"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("DefaultTireType"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("DefaultTireType"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("DefaultTireTypeName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("DefaultTireTypeName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("DefaultPreviewPawnClassName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("DefaultPreviewPawnClassName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("PlayOnConsoleSaveDir"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("PlayOnConsoleSaveDir"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("DefaultTexture"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("DefaultTexture"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("DefaultTextureName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("DefaultTextureName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("DefaultDiffuseTexture"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("DefaultDiffuseTexture"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("DefaultDiffuseTextureName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("DefaultDiffuseTextureName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("DefaultBSPVertexTexture"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("DefaultBSPVertexTexture"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("DefaultBSPVertexTextureName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("DefaultBSPVertexTextureName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("HighFrequencyNoiseTexture"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("HighFrequencyNoiseTexture"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("HighFrequencyNoiseTextureName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("HighFrequencyNoiseTextureName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("DefaultBokehTexture"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("DefaultBokehTexture"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("DefaultBokehTextureName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("DefaultBokehTextureName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("WireframeMaterial"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("WireframeMaterial"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("WireframeMaterialName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("WireframeMaterialName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("DebugMeshMaterial"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("DebugMeshMaterial"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("DebugMeshMaterialName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("DebugMeshMaterialName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("LevelColorationLitMaterial"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("LevelColorationLitMaterial"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("LevelColorationLitMaterialName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("LevelColorationLitMaterialName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("LevelColorationUnlitMaterial"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("LevelColorationUnlitMaterial"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("LevelColorationUnlitMaterialName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("LevelColorationUnlitMaterialName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("LightingTexelDensityMaterial"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("LightingTexelDensityMaterial"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("LightingTexelDensityName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("LightingTexelDensityName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("ShadedLevelColorationLitMaterial"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("ShadedLevelColorationLitMaterial"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("ShadedLevelColorationLitMaterialName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("ShadedLevelColorationLitMaterialName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("ShadedLevelColorationUnlitMaterial"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("ShadedLevelColorationUnlitMaterial"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("ShadedLevelColorationUnlitMaterialName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("ShadedLevelColorationUnlitMaterialName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("RemoveSurfaceMaterial"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("RemoveSurfaceMaterial"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("RemoveSurfaceMaterialName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("RemoveSurfaceMaterialName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("VertexColorMaterial"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("VertexColorMaterial"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("VertexColorMaterialName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("VertexColorMaterialName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("VertexColorViewModeMaterial_ColorOnly"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("VertexColorViewModeMaterial_ColorOnly"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("VertexColorViewModeMaterialName_ColorOnly"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("VertexColorViewModeMaterialName_ColorOnly"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("VertexColorViewModeMaterial_AlphaAsColor"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("VertexColorViewModeMaterial_AlphaAsColor"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("VertexColorViewModeMaterialName_AlphaAsColor"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("VertexColorViewModeMaterialName_AlphaAsColor"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("VertexColorViewModeMaterial_RedOnly"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("VertexColorViewModeMaterial_RedOnly"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("VertexColorViewModeMaterialName_RedOnly"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("VertexColorViewModeMaterialName_RedOnly"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("VertexColorViewModeMaterial_GreenOnly"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("VertexColorViewModeMaterial_GreenOnly"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("VertexColorViewModeMaterialName_GreenOnly"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("VertexColorViewModeMaterialName_GreenOnly"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("VertexColorViewModeMaterial_BlueOnly"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("VertexColorViewModeMaterial_BlueOnly"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("VertexColorViewModeMaterialName_BlueOnly"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("VertexColorViewModeMaterialName_BlueOnly"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("ConstraintLimitMaterial"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("ConstraintLimitMaterial"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("ConstraintLimitMaterialX"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("ConstraintLimitMaterialX"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("ConstraintLimitMaterialY"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("ConstraintLimitMaterialY"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("ConstraintLimitMaterialZ"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("ConstraintLimitMaterialZ"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("ConstraintLimitMaterialName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("ConstraintLimitMaterialName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("InvalidLightmapSettingsMaterial"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("InvalidLightmapSettingsMaterial"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("InvalidLightmapSettingsMaterialName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("InvalidLightmapSettingsMaterialName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("PreviewShadowsIndicatorMaterial"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("PreviewShadowsIndicatorMaterial"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("PreviewShadowsIndicatorMaterialName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("PreviewShadowsIndicatorMaterialName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("ArrowMaterial"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("ArrowMaterial"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("ArrowMaterialName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("ArrowMaterialName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("LightingOnlyBrightness"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("LightingOnlyBrightness"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("LightComplexityColors"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("LightComplexityColors"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("ShaderComplexityColors"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("ShaderComplexityColors"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("StationaryLightOverlapColors"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("StationaryLightOverlapColors"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("LODColorationColors"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("LODColorationColors"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("MaxPixelShaderAdditiveComplexityCount"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("MaxPixelShaderAdditiveComplexityCount"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("MaxES2PixelShaderAdditiveComplexityCount"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("MaxES2PixelShaderAdditiveComplexityCount"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("MinLightMapDensity"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("MinLightMapDensity"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("IdealLightMapDensity"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("IdealLightMapDensity"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("MaxLightMapDensity"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("MaxLightMapDensity"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("bRenderLightMapDensityGrayscale"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("bRenderLightMapDensityGrayscale"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("RenderLightMapDensityGrayscaleScale"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("RenderLightMapDensityGrayscaleScale"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("RenderLightMapDensityColorScale"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("RenderLightMapDensityColorScale"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("LightMapDensityVertexMappedColor"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("LightMapDensityVertexMappedColor"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("LightMapDensitySelectedColor"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("LightMapDensitySelectedColor"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("StatColorMappings"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("StatColorMappings"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("DefaultPhysMaterial"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("DefaultPhysMaterial"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("DefaultPhysMaterialName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("DefaultPhysMaterialName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("ActiveGameNameRedirects"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("ActiveGameNameRedirects"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("ActiveClassRedirects"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("ActiveClassRedirects"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("ActivePluginRedirects"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("ActivePluginRedirects"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("ActiveStructRedirects"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("ActiveStructRedirects"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("PreIntegratedSkinBRDFTexture"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("PreIntegratedSkinBRDFTexture"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("PreIntegratedSkinBRDFTextureName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("PreIntegratedSkinBRDFTextureName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("MiniFontTexture"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("MiniFontTexture"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("MiniFontTextureName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("MiniFontTextureName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("WeightMapPlaceholderTexture"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("WeightMapPlaceholderTexture"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("WeightMapPlaceholderTextureName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("WeightMapPlaceholderTextureName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("LightMapDensityTexture"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("LightMapDensityTexture"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("LightMapDensityTextureName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("LightMapDensityTextureName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("EngineLoop"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("EngineLoop"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("GameViewport"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("GameViewport"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("DeferredCommands"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("DeferredCommands"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("TickCycles"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("TickCycles"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("GameCycles"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("GameCycles"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("ClientCycles"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("ClientCycles"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("NearClipPlane"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("NearClipPlane"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("bHardwareSurveyEnabled_DEPRECATED"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("bHardwareSurveyEnabled_DEPRECATED"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("bSubtitlesEnabled"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("bSubtitlesEnabled"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("bSubtitlesForcedOff"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("bSubtitlesForcedOff"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("MaximumLoopIterationCount"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("MaximumLoopIterationCount"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("bCanBlueprintsTickByDefault"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("bCanBlueprintsTickByDefault"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("bEnableEditorPSysRealtimeLOD"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("bEnableEditorPSysRealtimeLOD"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("bForceDisableFrameRateSmoothing"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("bForceDisableFrameRateSmoothing"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("bSmoothFrameRate"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("bSmoothFrameRate"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("bUseFixedFrameRate"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("bUseFixedFrameRate"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("FixedFrameRate"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("FixedFrameRate"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("SmoothedFrameRateRange"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("SmoothedFrameRateRange"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("bCheckForMultiplePawnsSpawnedInAFrame"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("bCheckForMultiplePawnsSpawnedInAFrame"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("NumPawnsAllowedToBeSpawnedInAFrame"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("NumPawnsAllowedToBeSpawnedInAFrame"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("bShouldGenerateLowQualityLightmaps"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("bShouldGenerateLowQualityLightmaps"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("bUseConsoleInput"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("bUseConsoleInput"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("C_WorldBox"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("C_WorldBox"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("C_BrushWire"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("C_BrushWire"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("C_AddWire"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("C_AddWire"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("C_SubtractWire"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("C_SubtractWire"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("C_SemiSolidWire"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("C_SemiSolidWire"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("C_NonSolidWire"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("C_NonSolidWire"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("C_WireBackground"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("C_WireBackground"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("C_ScaleBoxHi"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("C_ScaleBoxHi"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("C_VolumeCollision"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("C_VolumeCollision"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("C_BSPCollision"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("C_BSPCollision"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("C_OrthoBackground"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("C_OrthoBackground"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("C_Volume"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("C_Volume"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("C_BrushShape"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("C_BrushShape"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("StreamingDistanceFactor"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("StreamingDistanceFactor"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("TransitionType"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("TransitionType"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("TransitionDescription"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("TransitionDescription"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("TransitionGameMode"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("TransitionGameMode"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("MeshLODRange"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("MeshLODRange"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("bAllowMatureLanguage"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("bAllowMatureLanguage"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("CameraRotationThreshold"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("CameraRotationThreshold"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("CameraTranslationThreshold"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("CameraTranslationThreshold"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("PrimitiveProbablyVisibleTime"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("PrimitiveProbablyVisibleTime"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("MaxOcclusionPixelsFraction"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("MaxOcclusionPixelsFraction"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("bPauseOnLossOfFocus"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("bPauseOnLossOfFocus"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("MaxParticleResize"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("MaxParticleResize"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("MaxParticleResizeWarn"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("MaxParticleResizeWarn"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("PendingDroppedNotes"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("PendingDroppedNotes"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("PhysicErrorCorrection"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("PhysicErrorCorrection"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("NetClientTicksPerSecond"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("NetClientTicksPerSecond"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("bHasPendingGlobalReregister"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("bHasPendingGlobalReregister"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("DisplayGamma"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("DisplayGamma"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("MinDesiredFrameRate"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("MinDesiredFrameRate"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("DefaultSelectedMaterialColor"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("DefaultSelectedMaterialColor"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("SelectedMaterialColor"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("SelectedMaterialColor"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("SelectionOutlineColor"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("SelectionOutlineColor"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("SubduedSelectionOutlineColor"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("SubduedSelectionOutlineColor"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("SelectedMaterialColorOverride"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("SelectedMaterialColorOverride"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("bIsOverridingSelectedColor"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("bIsOverridingSelectedColor"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("bEnableOnScreenDebugMessages"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("bEnableOnScreenDebugMessages"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("bEnableOnScreenDebugMessagesDisplay"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("bEnableOnScreenDebugMessagesDisplay"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("bSuppressMapWarnings"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("bSuppressMapWarnings"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("bCookSeparateSharedMPGameContent"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("bCookSeparateSharedMPGameContent"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("bDisableAILogging"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("bDisableAILogging"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("bEnableVisualLogRecordingOnStart"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("bEnableVisualLogRecordingOnStart"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("bUseSound"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("bUseSound"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("ScreenSaverInhibitorSemaphore"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("ScreenSaverInhibitorSemaphore"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("bLockReadOnlyLevels"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("bLockReadOnlyLevels"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("ParticleEventManagerClassPath"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("ParticleEventManagerClassPath"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("PriorityScreenMessages"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("PriorityScreenMessages"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("SelectionHighlightIntensity"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("SelectionHighlightIntensity"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("BSPSelectionHighlightIntensity"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("BSPSelectionHighlightIntensity"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("HoverHighlightIntensity"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("HoverHighlightIntensity"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("SelectionHighlightIntensityBillboards"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("SelectionHighlightIntensityBillboards"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("TravelFailureEvent"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("TravelFailureEvent"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("NetworkFailureEvent"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("NetworkFailureEvent"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("bIsInitialized"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("bIsInitialized"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("AudioDeviceManager"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("AudioDeviceManager"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("MainAudioDeviceHandle"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("MainAudioDeviceHandle"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("ScreenMessages"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("ScreenMessages"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("StereoRenderingDevice"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("StereoRenderingDevice"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("HMDDevice"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("HMDDevice"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("ViewExtensions"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("ViewExtensions"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("MotionControllerDevices"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("MotionControllerDevices"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("RunningAverageDeltaTime"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("RunningAverageDeltaTime"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("WorldAddedEvent"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("WorldAddedEvent"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("WorldDestroyedEvent"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("WorldDestroyedEvent"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("ScreenSaverInhibitor"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("ScreenSaverInhibitor"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("ScreenSaverInhibitorRunnable"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("ScreenSaverInhibitorRunnable"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("bPendingHardwareSurveyResults"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("bPendingHardwareSurveyResults"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("NetDriverDefinitions"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("NetDriverDefinitions"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("ServerActors"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("ServerActors"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("RuntimeServerActors"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("RuntimeServerActors"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("bStartedLoadMapMovie"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("bStartedLoadMapMovie"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("WorldList"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("WorldList"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("NextWorldContextHandle"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("NextWorldContextHandle"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("EngineStats"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("EngineStats"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("ConstraintLimitMaterialPrismatic"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("ConstraintLimitMaterialPrismatic"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("QuadComplexityColors"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("QuadComplexityColors"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("HLODColorationColors"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("HLODColorationColors"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("bOptimizeAnimBlueprintMemberVariableAccess"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("bOptimizeAnimBlueprintMemberVariableAccess"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("PortalRpcClient"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("PortalRpcClient"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("PortalRpcLocator"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("PortalRpcLocator"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("ServiceDependencies"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("ServiceDependencies"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("ServiceLocator"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("ServiceLocator"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("StreamingAccuracyColors"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("StreamingAccuracyColors"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("NetworkLagStateChangedEvent"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("NetworkLagStateChangedEvent"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("ConstraintLimitMaterialXAxis"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("ConstraintLimitMaterialXAxis"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("ConstraintLimitMaterialYAxis"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("ConstraintLimitMaterialYAxis"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("ConstraintLimitMaterialZAxis"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("ConstraintLimitMaterialZAxis"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("bShouldGenerateLowQualityLightmaps_DEPRECATED"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("bShouldGenerateLowQualityLightmaps_DEPRECATED"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("ErrorsAndWarningsCollector"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("ErrorsAndWarningsCollector"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("SelectionMeshSectionHighlightIntensity"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("SelectionMeshSectionHighlightIntensity"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("ActivePerformanceChart"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("ActivePerformanceChart"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("ActivePerformanceDataConsumers"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("ActivePerformanceDataConsumers"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("bIsVanillaProduct"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("bIsVanillaProduct"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("bAllowMultiThreadedAnimationUpdate"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("bAllowMultiThreadedAnimationUpdate"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("AssetManagerClassName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("AssetManagerClassName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("AssetManager"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("AssetManager"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("DefaultBloomKernelTexture"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("DefaultBloomKernelTexture"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("DefaultBloomKernelTextureName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("DefaultBloomKernelTextureName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("DebugEditorMaterialName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("DebugEditorMaterialName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("GameScreenshotSaveDirectory"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("GameScreenshotSaveDirectory"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("LastGCFrame"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("LastGCFrame"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("TimeSinceLastPendingKillPurge"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("TimeSinceLastPendingKillPurge"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("bFullPurgeTriggered"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("bFullPurgeTriggered"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("bShouldDelayGarbageCollect"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("bShouldDelayGarbageCollect"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("XRSystem"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("XRSystem"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("LastDynamicResolutionEvent"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("LastDynamicResolutionEvent"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("DynamicResolutionState"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("DynamicResolutionState"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("NextDynamicResolutionState"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("NextDynamicResolutionState"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("bIsDynamicResolutionPaused"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("bIsDynamicResolutionPaused"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("bDynamicResolutionEnableUserSetting"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("bDynamicResolutionEnableUserSetting"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("NavigationSystemConfigClassName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("NavigationSystemConfigClassName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("MaxES3PixelShaderAdditiveComplexityCount"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("MaxES3PixelShaderAdditiveComplexityCount"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("CustomTimeStep"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("CustomTimeStep"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("CustomTimeStepClassName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("CustomTimeStepClassName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("TimecodeProvider"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("TimecodeProvider"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("TimecodeFrameRateClassName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("TimecodeFrameRateClassName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("DefaultTimecodeFrameRate"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("DefaultTimecodeFrameRate"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("EyeTrackingDevice"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("EyeTrackingDevice"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("DefaultCustomTimeStep"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("DefaultCustomTimeStep"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("CurrentCustomTimeStep"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("CurrentCustomTimeStep"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("DefaultTimecodeProvider"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("DefaultTimecodeProvider"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("CustomTimecodeProvider"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("CustomTimecodeProvider"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("DefaultTimecodeProviderClassName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("DefaultTimecodeProviderClassName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("TimecodeProviderClassName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("TimecodeProviderClassName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("NetworkDDoSEscalationEvent"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("NetworkDDoSEscalationEvent"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("ArrowMaterialYellow"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("ArrowMaterialYellow"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("NetErrorLogInterval"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("NetErrorLogInterval"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("EmissiveMeshMaterial"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("EmissiveMeshMaterial"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("EmissiveMeshMaterialName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("EmissiveMeshMaterialName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("BlueNoiseTexture"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("BlueNoiseTexture"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("BlueNoiseTextureName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("BlueNoiseTextureName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("HairDefaultMaterial"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("HairDefaultMaterial"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("HairDefaultMaterialName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("HairDefaultMaterialName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("HairDebugMaterial"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("HairDebugMaterial"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("HairDebugMaterialName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("HairDebugMaterialName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("bIsCurrentCustomTimeStepInitialized"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("bIsCurrentCustomTimeStepInitialized"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("bIsCurrentTimecodeProviderInitialized"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("bIsCurrentTimecodeProviderInitialized"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("bGenerateDefaultTimecode"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("bGenerateDefaultTimecode"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("GenerateDefaultTimecodeFrameRate"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("GenerateDefaultTimecodeFrameRate"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("GenerateDefaultTimecodeFrameDelay"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("GenerateDefaultTimecodeFrameDelay"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("DefaultFilmGrainTexture"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("DefaultFilmGrainTexture"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("DefaultFilmGrainTextureName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("DefaultFilmGrainTextureName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("DefaultDestructiblePhysMaterial"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("DefaultDestructiblePhysMaterial"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("DefaultDestructiblePhysMaterialName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("DefaultDestructiblePhysMaterialName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("UseStaticMeshMinLODPerQualityLevels"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("UseStaticMeshMinLODPerQualityLevels"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("GPUSkinCacheVisualizationExcludedColor"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("GPUSkinCacheVisualizationExcludedColor"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("GPUSkinCacheVisualizationIncludedColor"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("GPUSkinCacheVisualizationIncludedColor"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("GPUSkinCacheVisualizationRecomputeTangentsColor"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("GPUSkinCacheVisualizationRecomputeTangentsColor"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("GPUSkinCacheVisualizationLowMemoryThresholdInMB"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("GPUSkinCacheVisualizationLowMemoryThresholdInMB"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("GPUSkinCacheVisualizationHighMemoryThresholdInMB"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("GPUSkinCacheVisualizationHighMemoryThresholdInMB"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("GPUSkinCacheVisualizationLowMemoryColor"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("GPUSkinCacheVisualizationLowMemoryColor"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("GPUSkinCacheVisualizationMidMemoryColor"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("GPUSkinCacheVisualizationMidMemoryColor"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("GPUSkinCacheVisualizationHighMemoryColor"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("GPUSkinCacheVisualizationHighMemoryColor"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("GPUSkinCacheVisualizationRayTracingLODOffsetColors"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("GPUSkinCacheVisualizationRayTracingLODOffsetColors"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("BlueNoiseScalarTexture"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("BlueNoiseScalarTexture"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("BlueNoiseVec2Texture"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("BlueNoiseVec2Texture"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("BlueNoiseScalarTextureName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("BlueNoiseScalarTextureName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("BlueNoiseVec2TextureName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("BlueNoiseVec2TextureName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("UseSkeletalMeshMinLODPerQualityLevels"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("UseSkeletalMeshMinLODPerQualityLevels"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("GlobalNetTravelCount"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("GlobalNetTravelCount"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("IrisNetDriverConfigs"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("IrisNetDriverConfigs"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("NaniteHiddenSectionMaterial"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("NaniteHiddenSectionMaterial"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("NaniteHiddenSectionMaterialName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("NaniteHiddenSectionMaterialName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("UseGrassVarityPerQualityLevels"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("UseGrassVarityPerQualityLevels"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("GlintTexture"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("GlintTexture"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("GlintTexture2"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("GlintTexture2"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("GlintTextureName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("GlintTextureName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("GlintTexture2Name"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("GlintTexture2Name"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("SimpleVolumeTexture"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("SimpleVolumeTexture"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("SimpleVolumeTextureName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("SimpleVolumeTextureName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("SimpleVolumeEnvTexture"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("SimpleVolumeEnvTexture"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("SimpleVolumeEnvTextureName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("SimpleVolumeEnvTextureName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("WeightMapArrayPlaceholderTexture"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("WeightMapArrayPlaceholderTexture"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("WeightMapArrayPlaceholderTextureName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("WeightMapArrayPlaceholderTextureName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("TextureColorViewModeMaterial"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("TextureColorViewModeMaterial"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("TextureColorViewModeMaterialName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("TextureColorViewModeMaterialName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("GGXLTCAmpTexture"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("GGXLTCAmpTexture"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("GGXLTCAmpTextureName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("GGXLTCAmpTextureName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("GGXLTCMatTexture"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("GGXLTCMatTexture"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("GGXLTCMatTextureName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("GGXLTCMatTextureName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("SheenLTCTexture"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("SheenLTCTexture"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("SheenLTCTextureName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("SheenLTCTextureName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("GGXReflectionEnergyTexture"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("GGXReflectionEnergyTexture"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("GGXReflectionEnergyTextureName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("GGXReflectionEnergyTextureName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("GGXTransmissionEnergyTexture"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("GGXTransmissionEnergyTexture"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("GGXTransmissionEnergyTextureName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("GGXTransmissionEnergyTextureName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("SheenEnergyTexture"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("SheenEnergyTexture"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("SheenLegacyEnergyTextureName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("SheenLegacyEnergyTextureName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("SheenEnergyTextureName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("SheenEnergyTextureName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("DiffuseEnergyTexture"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("DiffuseEnergyTexture"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("DiffuseEnergyTextureName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("DiffuseEnergyTextureName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("UseClothAssetMinLODPerQualityLevels"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("UseClothAssetMinLODPerQualityLevels"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("BlueNoiseScalarMobileTextureName"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("BlueNoiseScalarMobileTextureName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("bGCPerformingFullPurge"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("bGCPerformingFullPurge"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UEngine"), STR("WorldContextDestroyedEvent"), -1); val != -1)
    Unreal::UEngine::MemberOffsets.emplace(STR("WorldContextDestroyedEvent"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("ViewportConsole"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("ViewportConsole"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("DebugProperties"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("DebugProperties"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("TitleSafeZone"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("TitleSafeZone"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("SplitscreenInfo"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("SplitscreenInfo"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("MaxSplitscreenPlayers"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("MaxSplitscreenPlayers"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("bShowTitleSafeZone"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("bShowTitleSafeZone"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("bIsPlayInEditorViewport"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("bIsPlayInEditorViewport"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("bDisableWorldRendering"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("bDisableWorldRendering"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("ActiveSplitscreenType"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("ActiveSplitscreenType"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("World"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("World"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("bSuppressTransitionMessage"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("bSuppressTransitionMessage"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("ViewModeIndex"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("ViewModeIndex"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("EngineShowFlags"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("EngineShowFlags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("Viewport"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("Viewport"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("ViewportFrame"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("ViewportFrame"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("Window"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("Window"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("ViewportOverlayWidget"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("ViewportOverlayWidget"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("GameLayerManagerPtr"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("GameLayerManagerPtr"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("CurrentBufferVisualizationMode"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("CurrentBufferVisualizationMode"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("HighResScreenshotDialog"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("HighResScreenshotDialog"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("CursorWidgets"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("CursorWidgets"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("StatUnitData"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("StatUnitData"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("StatHitchesData"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("StatHitchesData"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("bDisableSplitScreenOverride"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("bDisableSplitScreenOverride"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("bIgnoreInput"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("bIgnoreInput"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("MouseCaptureMode"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("MouseCaptureMode"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("bHideCursorDuringCapture"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("bHideCursorDuringCapture"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("AudioDeviceHandle"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("AudioDeviceHandle"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("bHasAudioFocus"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("bHasAudioFocus"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("bLockDuringCapture"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("bLockDuringCapture"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("MouseLockMode"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("MouseLockMode"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("bUseSoftwareCursorWidgets"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("bUseSoftwareCursorWidgets"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("HardwareCursorCache"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("HardwareCursorCache"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("HardwareCursors"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("HardwareCursors"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("bIsMouseOverClient"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("bIsMouseOverClient"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("CurrentNaniteVisualizationMode"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("CurrentNaniteVisualizationMode"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("CurrentLumenVisualizationMode"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("CurrentLumenVisualizationMode"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("CurrentVirtualShadowMapVisualizationMode"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("CurrentVirtualShadowMapVisualizationMode"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("CurrentStrataVisualizationMode"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("CurrentStrataVisualizationMode"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("CurrentGroomVisualizationMode"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("CurrentGroomVisualizationMode"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UGameViewportClient"), STR("CurrentSubstrateVisualizationMode"), -1); val != -1)
    Unreal::UGameViewportClient::MemberOffsets.emplace(STR("CurrentSubstrateVisualizationMode"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UPlayer"), STR("CurrentNetSpeed"), -1); val != -1)
    Unreal::UPlayer::MemberOffsets.emplace(STR("CurrentNetSpeed"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UPlayer"), STR("ConfiguredInternetSpeed"), -1); val != -1)
    Unreal::UPlayer::MemberOffsets.emplace(STR("ConfiguredInternetSpeed"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UPlayer"), STR("ConfiguredLanSpeed"), -1); val != -1)
    Unreal::UPlayer::MemberOffsets.emplace(STR("ConfiguredLanSpeed"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("ULocalPlayer"), STR("ViewportClient"), -1); val != -1)
    Unreal::ULocalPlayer::MemberOffsets.emplace(STR("ViewportClient"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("ULocalPlayer"), STR("LastViewLocation"), -1); val != -1)
    Unreal::ULocalPlayer::MemberOffsets.emplace(STR("LastViewLocation"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("ULocalPlayer"), STR("AspectRatioAxisConstraint"), -1); val != -1)
    Unreal::ULocalPlayer::MemberOffsets.emplace(STR("AspectRatioAxisConstraint"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("ULocalPlayer"), STR("bSentSplitJoin"), -1); val != -1)
    Unreal::ULocalPlayer::MemberOffsets.emplace(STR("bSentSplitJoin"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("ULocalPlayer"), STR("ControllerId"), -1); val != -1)
    Unreal::ULocalPlayer::MemberOffsets.emplace(STR("ControllerId"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("LineBatcher"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("LineBatcher"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("PersistentLineBatcher"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("PersistentLineBatcher"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("ForegroundLineBatcher"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("ForegroundLineBatcher"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("ExtraReferencedObjects"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("ExtraReferencedObjects"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("PerModuleDataObjects"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("PerModuleDataObjects"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("StreamingLevelsPrefix"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("StreamingLevelsPrefix"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("ViewLocationsRenderedLastFrame"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("ViewLocationsRenderedLastFrame"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bWorldWasLoadedThisTick"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bWorldWasLoadedThisTick"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bTriggerPostLoadMap"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bTriggerPostLoadMap"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("AuthorityGameMode"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("AuthorityGameMode"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("NetworkActors"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("NetworkActors"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bRequiresHitProxies"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bRequiresHitProxies"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bStreamingDataDirty"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bStreamingDataDirty"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("BuildStreamingDataTimer"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("BuildStreamingDataTimer"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("URL"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("URL"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bInTick"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bInTick"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bIsBuilt"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bIsBuilt"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bTickNewlySpawned"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bTickNewlySpawned"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bPostTickComponentUpdate"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bPostTickComponentUpdate"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("PlayerNum"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("PlayerNum"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("TimeSinceLastPendingKillPurge"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("TimeSinceLastPendingKillPurge"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("FullPurgeTriggered"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("FullPurgeTriggered"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bShouldDelayGarbageCollect"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bShouldDelayGarbageCollect"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bIsWorldInitialized"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bIsWorldInitialized"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("StreamingVolumeUpdateDelay"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("StreamingVolumeUpdateDelay"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bIsLevelStreamingFrozen"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bIsLevelStreamingFrozen"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bShouldForceUnloadStreamingLevels"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bShouldForceUnloadStreamingLevels"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bShouldForceVisibleStreamingLevels"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bShouldForceVisibleStreamingLevels"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bDoDelayedUpdateCullDistanceVolumes"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bDoDelayedUpdateCullDistanceVolumes"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bHack_Force_UsesGameHiddenFlags_True"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bHack_Force_UsesGameHiddenFlags_True"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bIsRunningConstructionScript"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bIsRunningConstructionScript"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bShouldSimulatePhysics"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bShouldSimulatePhysics"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("DebugDrawTraceTag"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("DebugDrawTraceTag"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("AudioDeviceHandle"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("AudioDeviceHandle"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("LastTimeUnbuiltLightingWasEncountered"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("LastTimeUnbuiltLightingWasEncountered"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("TimeSeconds"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("TimeSeconds"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("RealTimeSeconds"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("RealTimeSeconds"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("AudioTimeSeconds"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("AudioTimeSeconds"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("DeltaTimeSeconds"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("DeltaTimeSeconds"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("PauseDelay"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("PauseDelay"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bOriginOffsetThisFrame"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bOriginOffsetThisFrame"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("NextURL"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("NextURL"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("NextSwitchCountdown"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("NextSwitchCountdown"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("PreparingLevelNames"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("PreparingLevelNames"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("CommittedPersistentLevelName"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("CommittedPersistentLevelName"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("NumLightingUnbuiltObjects"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("NumLightingUnbuiltObjects"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bDropDetail"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bDropDetail"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bAggressiveLOD"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bAggressiveLOD"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bIsDefaultLevel"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bIsDefaultLevel"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bRequestedBlockOnAsyncLoading"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bRequestedBlockOnAsyncLoading"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bActorsInitialized"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bActorsInitialized"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bBegunPlay"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bBegunPlay"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bMatchStarted"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bMatchStarted"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bPlayersOnly"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bPlayersOnly"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bPlayersOnlyPending"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bPlayersOnlyPending"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bStartup"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bStartup"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bIsTearingDown"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bIsTearingDown"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bKismetScriptError"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bKismetScriptError"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bDebugPauseExecution"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bDebugPauseExecution"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bAllowAudioPlayback"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bAllowAudioPlayback"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bDebugFrameStepExecution"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bDebugFrameStepExecution"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bAreConstraintsDirty"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bAreConstraintsDirty"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bCreateRenderStateForHiddenComponents"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bCreateRenderStateForHiddenComponents"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bShouldTick"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bShouldTick"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("UnpausedTimeSeconds"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("UnpausedTimeSeconds"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("NumInvalidReflectionCaptureComponents"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("NumInvalidReflectionCaptureComponents"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("NumTextureStreamingUnbuiltComponents"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("NumTextureStreamingUnbuiltComponents"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("NumTextureStreamingDirtyResources"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("NumTextureStreamingDirtyResources"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bIsCameraMoveableWhenPaused"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bIsCameraMoveableWhenPaused"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("OriginOffsetThisFrame"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("OriginOffsetThisFrame"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bDebugDrawAllTraceTags"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bDebugDrawAllTraceTags"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("ActiveLevelCollectionIndex"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("ActiveLevelCollectionIndex"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("NumUnbuiltReflectionCaptures"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("NumUnbuiltReflectionCaptures"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bMaterialParameterCollectionInstanceNeedsDeferredUpdate"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bMaterialParameterCollectionInstanceNeedsDeferredUpdate"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bCleanedUpWorld"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bCleanedUpWorld"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bMarkedObjectsPendingKill"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bMarkedObjectsPendingKill"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("LevelSequenceActors"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("LevelSequenceActors"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("NumStreamingLevelsBeingLoaded"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("NumStreamingLevelsBeingLoaded"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("CleanupWorldTag"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("CleanupWorldTag"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bAllowDeferredPhysicsStateCreation"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bAllowDeferredPhysicsStateCreation"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bInitializedAndNeedsCleanup"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bInitializedAndNeedsCleanup"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("IsInBlockTillLevelStreamingCompleted"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("IsInBlockTillLevelStreamingCompleted"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("BlockTillLevelStreamingCompletedEpoch"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("BlockTillLevelStreamingCompletedEpoch"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("LWILastAssignedUID"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("LWILastAssignedUID"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("DeltaRealTimeSeconds"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("DeltaRealTimeSeconds"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bSupportsMakingVisibleTransactionRequests"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bSupportsMakingVisibleTransactionRequests"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bSupportsMakingInvisibleTransactionRequests"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bSupportsMakingInvisibleTransactionRequests"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("LastRenderTime"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("LastRenderTime"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bHasEverBeenInitialized"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bHasEverBeenInitialized"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("bIsBeingCleanedUp"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("bIsBeingCleanedUp"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("LineBatcher_DEPRECATED"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("LineBatcher_DEPRECATED"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("PersistentLineBatcher_DEPRECATED"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("PersistentLineBatcher_DEPRECATED"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("ForegroundLineBatcher_DEPRECATED"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("ForegroundLineBatcher_DEPRECATED"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("PhysicsQueryHandler"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("PhysicsQueryHandler"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("DeferredComponentMoves"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("DeferredComponentMoves"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("LineBatchers"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("LineBatchers"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("PendingVisibilityLock"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("PendingVisibilityLock"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("AddLevelToWorldExtensionEvent"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("AddLevelToWorldExtensionEvent"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UWorld"), STR("RemoveLevelFromWorldExtensionEvent"), -1); val != -1)
    Unreal::UWorld::MemberOffsets.emplace(STR("RemoveLevelFromWorldExtensionEvent"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UDataTable"), STR("RowStruct"), -1); val != -1)
    Unreal::UDataTable::MemberOffsets.emplace(STR("RowStruct"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UDataTable"), STR("RowMap"), -1); val != -1)
    Unreal::UDataTable::MemberOffsets.emplace(STR("RowMap"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UDataTable"), STR("bStripFromClientBuilds"), -1); val != -1)
    Unreal::UDataTable::MemberOffsets.emplace(STR("bStripFromClientBuilds"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UDataTable"), STR("bIgnoreExtraFields"), -1); val != -1)
    Unreal::UDataTable::MemberOffsets.emplace(STR("bIgnoreExtraFields"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UDataTable"), STR("bIgnoreMissingFields"), -1); val != -1)
    Unreal::UDataTable::MemberOffsets.emplace(STR("bIgnoreMissingFields"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UDataTable"), STR("ImportKeyField"), -1); val != -1)
    Unreal::UDataTable::MemberOffsets.emplace(STR("ImportKeyField"), static_cast<int32_t>(val));
if (auto val = parser.get_int64(STR("UDataTable"), STR("bPreserveExistingValues"), -1); val != -1)
    Unreal::UDataTable::MemberOffsets.emplace(STR("bPreserveExistingValues"), static_cast<int32_t>(val));
