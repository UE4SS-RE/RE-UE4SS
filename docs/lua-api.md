# Lua API

These are the Lua API functions available in UE4SS, on top of the standard libraries that Lua comes with by defualt.

For version: **3.1.0**

Current status: **mostly complete**

## Full API Overview

This is an overall list of API definitions available in UE4SS. For more readable information, see the individual API definition pages in the collapsible sections 4.1, 4.2 and 4.3.

> **Warning: This API list is not updated since 2.5.2, so it out of date. Please refer to the individual API definition pages for the most up-to-date information.**

### Table Definitions
- The definitions appear as: FieldName | FieldValueType
- Fields that only have numeric field names have '#' as their name in this definition for clarity
- All fields are required unless otherwise specified
```    
    ModifierKeys
        # | string (Microsoft Virtual Key-Code)

    PropertyTypes
        ObjectProperty      | internal_value
        ObjectPtrProperty   | internal_value
        Int8Property        | internal_value
        Int16Property       | internal_value
        IntProperty         | internal_value
        Int64Property       | internal_value
        NameProperty        | internal_value
        FloatProperty       | internal_value
        StrProperty         | internal_value
        ByteProperty        | internal_value
        UInt16Property      | internal_value
        UIntProperty        | internal_value
        UInt64Property      | internal_value
        BoolProperty        | internal_value
        ArrayProperty       | internal_value
        MapProperty         | internal_value
        StructProperty      | internal_value
        ClassProperty       | internal_value
        WeakObjectProperty  | internal_value
        EnumProperty        | internal_value
        TextProperty        | internal_value

    OffsetInternalInfo
        Property        | string (Name of the property to use as relative start instead of base)
        RelativeOffset  | integer (Offset from relative start to this property)

    ArrayPropertyInfo
        Type | table (PropertyTypes)

    CustomPropertyInfo
        Name            | string (Name to use with the __index metamethod)
        Type            | table (PropertyTypes)
        BelongsToClass  | string (Full class name without type that this property belongs to)
        OffsetInternal  | integer or table (if table: OffsetInternalInfo, otherwise: offset from base to this property)
        ArrayProperty   | table (Optional, ArrayPropertyInfo)

    EObjectFlags
        - A table of object flags that can be or'd together by using |.
        RF_NoFlags                       | 0x00000000
        RF_Public                        | 0x00000001
        RF_Standalone                    | 0x00000002
        RF_MarkAsNative                  | 0x00000004
        RF_Transactional                 | 0x00000008
        RF_ClassDefaultObject            | 0x00000010
        RF_ArchetypeObject               | 0x00000020
        RF_Transient                     | 0x00000040
        RF_MarkAsRootSet                 | 0x00000080
        RF_TagGarbageTemp                | 0x00000100
        RF_NeedInitialization            | 0x00000200
        RF_NeedLoad                      | 0x00000400
        RF_KeepForCooker                 | 0x00000800
        RF_NeedPostLoad                  | 0x00001000
        RF_NeedPostLoadSubobjects        | 0x00002000
        RF_NewerVersionExists            | 0x00004000
        RF_BeginDestroyed                | 0x00008000
        RF_FinishDestroyed               | 0x00010000
        RF_BeingRegenerated              | 0x00020000
        RF_DefaultSubObject              | 0x00040000
        RF_WasLoaded                     | 0x00080000
        RF_TextExportTransient           | 0x00100000
        RF_LoadCompleted                 | 0x00200000
        RF_InheritableComponentTemplate  | 0x00400000
        RF_DuplicateTransient            | 0x00800000
        RF_StrongRefOnFrame              | 0x01000000
        RF_NonPIEDuplicateTransient      | 0x01000000
        RF_Dynamic                       | 0x02000000
        RF_WillBeLoaded                  | 0x04000000
        RF_HasExternalPackage            | 0x08000000
        RF_AllFlags                      | 0x0FFFFFFF

    EInternalObjectFlags
        - A table of internal object flags that can be or'd together by using |.
        ReachableInCluster               | 0x00800000
        ClusterRoot                      | 0x01000000
        Native                           | 0x02000000
        Async                            | 0x04000000
        AsyncLoading                     | 0x08000000
        Unreachable                      | 0x10000000
        PendingKill                      | 0x20000000
        RootSet                          | 0x40000000
        GarbageCollectionKeepFlags       | 0x0E000000
        AllFlags                         | 0x7F800000
```

### Global Functions
```
    print(any... Message)
        - Does not have the capability to format. Use 'string.format' if you require formatting.
    
    StaticFindObject(string ObjectName) -> { UObject | AActor | nil }
    StaticFindObject(UClass Class=nil, UObject InOuter=nil, string ObjectName, bool ExactClass=false)
        - Maps to https://docs.unrealengine.com/4.26/en-US/API/Runtime/CoreUObject/UObject/StaticFindObject/
    
    FindFirstOf(string ShortClassName) -> { UObject | AActor | nil }
        - Find the first non-default instance of the supplied class name
        - Param 'ShortClassName': Should only contains the class name itself without path info
    
    FindAllOf(string ShortClassName) -> table -> { UObject | AActor } | nil
        - Find all non-default instances of the supplied class name
        - Param 'ShortClassName': Should only contains the class name itself without path info
    
    RegisterKeyBind(integer Key, function Callback)
    RegisterKeyBind(integer Key, table ModifierKeys, function callback)
        - Registers a callback for a key-bind
        - Callbacks can only be triggered while the game or debug console is on focus

    IsKeyBindRegistered(integer Key)
    IsKeyBindRegistered(integer Key, table ModifierKeys)
        - Checks if, at the time of the invocation, the supplied keys have been registered
    
    RegisterHook(string UFunctionName, function Callback) -> integer, integer
        - Registers a callback for a UFunction
        - Callbacks are triggered when a UFunction is executed
        - The callback params are: UObject self, UFunctionParams...
        - Returns two ids, both of which must be passed to 'UnregisterHook' if you want to unregister the hook.

    UnregisterHook(string UFunctionName, integer PreId, integer PostId)
        - Unregisters a hook.
		
    ExecuteInGameThread(function Callback)
        - Execute code inside the game thread using ProcessEvent.
        - Will execute as soon as the game has time to execute.

    FName(string Name) -> FName
    FName(integer ComparisonIndex) -> FName
        - Returns the FName for this string/ComparisonIndex or the FName for "None" if the name doesn't exist

    FText(string Text) -> FText
        - Returns the FText representation of this string

    StaticConstructObject(UClass Class,
                          UObject Outer,
                          FName Name, #Optional
                          EObjectFlags Flags, #Optional
                          EInternalObjectFlags InternalSetFlags, #Optional
                          bool CopyTransientsFromClassDefaults, #Optional
                          bool AssumeTemplateIsArchetype, #Optional
                          UObject Template, #Optional
                          FObjectInstancingGraph InstanceGraph, #Optional
                          UPackage ExternalPackage, #Optional
                          void SubobjectOverrides #Optional) -> UObject
        - Attempts to construct a UObject of the passed UClass
        - (>=4.26) Maps to https://docs.unrealengine.com/4.27/en-US/API/Runtime/CoreUObject/UObject/StaticConstructObject_Internal/1/
        - (<4.25) Maps to https://docs.unrealengine.com/4.27/en-US/API/Runtime/CoreUObject/UObject/StaticConstructObject_Internal/2/

    RegisterCustomProperty(table CustomPropertyInfo)
        - Registers a custom property to be used automatically with 'UObject.__index'

    ForEachUObject(function Callback)
        - Execute the callback function for each UObject in GUObjectArray
        - The callback params are: UObject object, integer ChunkIndex, integer ObjectIndex

    NotifyOnNewObject(string UClassName, function Callback)
        - Executes the provided Lua function whenever an instance of the provided class is constructed.
        - Inheritance is taken into account, so if you provide "/Script/Engine.Actor" as the class then it will execute your
        - Lua function when any object is constructed that's either an AActor or is derived from AActor.

    RegisterCustomEvent(string EventName, function Callback)
        - Registers a callback that will get called when a BP function/event is called with the name 'EventName'.
    
    RegisterLoadMapPreHook(function Callback)
        - Registers a callback that will get called before UEngine::LoadMap is called.
        - The callback params are: UEngine Engine, struct FWorldContext& WorldContext, FURL URL, class UPendingNetGame* PendingGame, FString& Error
        - Params (except strings & bools & FOutputDevice) must be retrieved via 'Param:Get()' and set via 'Param:Set()'.
        
    RegisterLoadMapPostHook(function Callback)
        - Registers a callback that will get called after UEngine::LoadMap is called.
        - The callback params are: UEngine Enigne, struct FWorldContext& WorldContext, FURL URL, class UPendingNetGame* PendingGame, FString& Error
        - Params (except strings & bools & FOutputDevice) must be retrieved via 'Param:Get()' and set via 'Param:Set()'.

    RegisterInitGameStatePreHook(function Callback)
        - Registers a callback that will get called before AGameModeBase::InitGameState is called.
        - The callback params are: AGameModeBase Context
        - Params (except strings & bools & FOutputDevice) must be retrieved via 'Param:Get()' and set via 'Param:Set()'.

    RegisterInitGameStatePostHook(function Callback)
        - Registers a callback that will get called after AGameModeBase::InitGameState is called.
        - The callback params are: AGameModeBase Context
        - Params (except strings & bools & FOutputDevice) must be retrieved via 'Param:Get()' and set via 'Param:Set()'.

    RegisterBeginPlayPreHook(function Callback)
        - Registers a callback that will get called before AActor::BeginPlay is called.
        - The callback params are: AActor Context
        - Params (except strings & bools & FOutputDevice) must be retrieved via 'Param:Get()' and set via 'Param:Set()'.

    RegisterBeginPlayPostHook(function Callback)
        - Registers a callback that will get called after AActor::BeginPlay is called.
        - The callback params are: AActor Context
        - Params (except strings & bools & FOutputDevice) must be retrieved via 'Param:Get()' and set via 'Param:Set()'.

    RegisterProcessConsoleExecPreHook(function Callback)
        - Registers a callback that will get called before UObject::ProcessConsoleExec is called.
        - The callback params are: UObject Context, string Cmd, table CommandParts, FOutputDevice Ar, UObject Executor
        - Params (except strings & bools & FOutputDevice) must be retrieved via 'Param:Get()' and set via 'Param:Set()'.
        - If the callback returns nothing (or nil), the original return value of ProcessConsoleExec will be used.
        - If the callback returns true or false, the supplied value will override the original return value of ProcessConsoleExec.

    RegisterProcessConsoleExecPostHook(function Callback)
        - Registers a callback that will get called after UObject::ProcessConsoleExec is called.
        - The callback params are: UObject Context, string Cmd, table CommandParts, FOutputDevice Ar, UObject Executor
        - Params (except strings & bools & FOutputDevice) must be retrieved via 'Param:Get()' and set via 'Param:Set()'.
        - If the callback returns nothing (or nil), the original return value of ProcessConsoleExec will be used.
        - If the callback returns true or false, the supplied value will override the original return value of ProcessConsoleExec.

    RegisterCallFunctionByNameWithArgumentsPreHook(function Callback)
        - Registers a callback that will be called before UObject::CallFunctionByNameWithArguments is called.
        - The callback params are: UObject Context, string Str, FOutputDevice Ar, UObject Executor, bool bForceCallWithNonExec
        - Params (except strings & bools & FOutputDevice) must be retrieved via 'Param:Get()' and set via 'Param:Set()'.
        - If the callback returns nothing (or nil), the original return value of CallFunctionByNameWithArguments will be used.
        - If the callback returns true or false, the supplied value will override the original return value of CallFunctionByNameWithArguments.

    RegisterCallFunctionByNameWithArgumentsPostHook(function Callback)
        - Registers a callback that will be called after UObject::CallFunctionByNameWithArguments is called.
        - The callback params are: UObject Context, string Str, FOutputDevice Ar, UObject Executor, bool bForceCallWithNonExec
        - Params (except strings & bools & FOutputDevice) must be retrieved via 'Param:Get()' and set via 'Param:Set()'.
        - If the callback returns nothing (or nil), the original return value of CallFunctionByNameWithArguments will be used.
        - If the callback returns true or false, the supplied value will override the original return value of CallFunctionByNameWithArguments.

    RegisterULocalPlayerExecPreHook(function Callback)
        - Registers a callback that will be called before ULocalPlayer::Exec is called.
        - The callback params are: ULocalPlayer Context, UWorld InWorld, string Cmd, FOutputDevice Ar
        - Params (except strings & bools & FOutputDevice) must be retrieved via 'Param:Get()' and set via 'Param:Set()'.
        - The callback can have two return values.
        - If the first return value is nothing (or nil), the original return value of Exec will be used.
        - If the first return value is true or false, the supplied value will override the original return value of Exec.
        - The second return value controls whether the original Exec will execute.
        - If the second return value is nil or true, the orginal Exec will execute.
        - If the second return value is false, the original Exec will not execute.

    RegisterULocalPlayerExecPostHook(function Callback)
        - Registers a callback that will be called after ULocalPlayer::Exec is called.
        - The callback params are: ULocalPlayer Context, UWorld InWorld, string Cmd, FOutputDevice Ar
        - Params (except strings & bools & FOutputDevice) must be retrieved via 'Param:Get()' and set via 'Param:Set()'.
        - The callback can have two return values.
        - If the first return value is nothing (or nil), the original return value of Exec will be used.
        - If the first return value is true or false, the supplied value will override the original return value of Exec.
        - The second return value controls whether the original Exec will execute.
        - If the second return value is nil or true, the orginal Exec will execute.
        - If the second return value is false, the original Exec will not execute.

    RegisterConsoleCommandHandler(string CommandName, function Callback)
        - Registers a callback for a custom console commands.
        - The callback only runs in the context of UGameViewportClient.
        - The callback params are: string Cmd, table CommandParts, FOutputDevice Ar
        - Params (except strings & bools & FOutputDevice) must be retrieved via 'Param:Get()' and set via 'Param:Set()'.
        - The callback must return either true or false.
        - If the callback returns true, no further handlers will be called for this command.

    RegisterConsoleCommandGlobalHandler(string CommandName, function Callback)
        - Registers a callback for a custom console command.
        - Unlike 'RegisterConsoleCommandHandler', this global variant runs the callback for all contexts.
        - The callback params are: string Cmd, table CommandParts, FOutputDevice Ar
        - Params (except strings & bools & FOutputDevice) must be retrieved via 'Param:Get()' and set via 'Param:Set()'.
        - The callback must return either true or false.
        - If the callback returns true, no further handlers will be called for this command.

    ExecuteAsync(function Callback)
        - Asynchronously executes the specified function

    ExecuteWithDelay(integer DelayInMilliseconds, function Callback)
        - Asynchronously executes the specified function after the specified delay

    RegisterConsoleCommandHandler(string CommandName, function Callback)
        - Executes the provided Lua function whenever the CommandName is entered into the UE console.
        - The parameters for the callback are the full command (string),
        - and the parameters (table, containing the full command split by spaces), and FOutputDevice.
        - In the callback, return true to prevent other handlers from handling the command, or false to allow other handlers.

    LoadAsset(string AssetPathAndName)
        - Loads an asset by name.
        - Must only be called from within the game thread.
        - For example, from within a UFunction hook or RegisterConsoleCommandHandler callback.

    FindObject(string|FName|nil ClassName, string|FName|nil ObjectShortName, EObjectFlags RequiredFlags, EObjectFlags BannedFlags) -> UObject derivative
        - Finds an object by either class name or short object name.
        - ClassName or ObjectShortName can be nil, but not both.
        - Returns a UObject of a derivative of UObject.

    FindObject(UClass InClass, UObject InOuter, string Name, bool ExactClass)
        - Finds an object. Works the same way as the function by the same name in the UE source.

    FindObjects(integer NumObjectsToFind, string|FName|nil ClassName, string|FName|nil ObjectShortName, EObjectFlags RequiredFlags, EObjectFlags BannedFlags, bool bExactClass) -> table -> { UObject derivative }
        - Finds the first specified number of objects by class name or short object name.
        - To find all objects that match your criteria, set NumObjectsToFind to 0 or nil.
        - Returns a table of UObject derivatives

    LoopAsync(integer DelayInMilliseconds, function Callback)
        - Starts a loop that sleeps for the supplied number of milliseconds and stops when the callback returns true.
    
    IterateGameDirectories() -> table
        - Returns a table of all game directories.
        - An example of an absolute path to Win64: Q:\SteamLibrary\steamapps\common\Deep Rock Galactic\FSD\Binaries\Win64
        - To get to the same directory, do: IterateGameDirectories().Game.Binaries.Win64
        - Note that the game name is replaced by 'Game' to keep things generic.
        - You can use '.__name' and '.__absolute_path' to retrieve values.
        - You can use '.__files' to retrieve a table containing all files in this directory.
        - You also use '.__name' and '.__absolute_path' for files.
```

### Classes
```
    RemoteObject
        Inheritance:
        - The first of two base objects that all other objects inherits from
        - Contains a pointer to a C/C++ object that's typically owned by the game
        Methods
            IsValid() -> bool
                - Returns whether this object is valid or not

    LocalObject
        Inheritance:
        - The second of two base objects that all other objects inherits from
        - Contains an inlined object which is fully owned by Lua
        Methods

    UnrealVersion
        Inheritance:
        Methods
            GetMajor() -> integer
            GetMinor() -> integer
            IsEqual(number MajorVersion, number MinorVersion) -> bool
            IsAtLeast(number MajorVersion, number MinorVersion) -> bool
            IsAtMost(number MajorVersion, number MinorVersion) -> bool
            IsBelow(number MajorVersion, number MinorVersion) -> bool
            IsAbove(number MajorVersion, number MinorVersion) -> bool

    UE4SS
        Inheritance:
        - Class for interacting with UE4SS metadata
        Methods
            GetVersion() -> 3x integer
                - Returns major, minor and hotfix version numbers
                - To detect version 1.0 or below, check if "UE4SS" or "UE4SS.GetVersion" is nil

    Mod
        Inheritance: RemoteObject
        - Class for interacting with the local mod object
        Methods
            SetSharedVariable(string VariableName, any Value)
                - Sets a variable that can be accessed by any mod.
                - The second parameter (Value) can only be one of the following types: nil, string, number, bool, UObject(+derivatives), lightuserdata.
                - These variables do not get reset when hot-reloading.

            GetSharedVariable(string VariableName) -> any
                - Gets a variable that could've been set from another mod.
                - The return value can only be one of the following types: nil, string, number, bool, UObject(+derivatives), lightuserdata.
        
            type() -> string
                - Returns "ModRef"

    UObject
        Inheritance: RemoteObject
        - This is the base class that most other Unreal Engine game objects inherit from
        Methods
            __index(string MemberVariableName) -> auto
                - Attempts to return either a member variable or a callable UFunction
                - Can return any type, you can use the 'type()' function on the returned value to figure out what Lua class it's using (if non-trivial type)
            
            __newindex(string MemberVariableName, auto NewValue)
                - Attempts to set the value of a member variable
            
            GetFullName() -> string
                - Returns the full name & path info for a UObject & its derivatives
            
            GetFName() -> FName
                - Returns the FName of this object by copy
                - All FNames returned by '__index' are returned by reference
            
            GetAddress() -> integer
                - Returns the memory address of this object
            
            GetClass() -> UClass
                - Returns the class of this object, this is equivalent to 'UObject->ClassPrivate' in Unreal

            GetOuter() -> UObject
                - Returns the Outer of this object

            IsAnyClass() -> bool
                - Returns true if this UObject is a UClass or a derivative of UClass

            Reflection() -> UObjectReflection
                - Returns a reflection object

            GetPropertyValue(string MemberVariableName) -> auto
                - Identical to __index

            SetPropertyValue(string MemberVariableName auto NewValue)
                - Identical to __newindex

            IsClass() -> bool
                - Returns whether this object is a UClass or UClass derivative

            GetWorld() -> UWorld
                - Returns the UWorld that this UObject is contained within.

            CallFunction(UFunction function, auto Params...)
                - Calls the supplied UFunction on this UObject.

            IsA(UClass Class) -> bool
            IsA(string FullClassName) -> bool
                - Returns whether this object is of the specified class.

            HasAllFlags(EObjectFlags FlagsToCheck)
                - Returns whether the object has all of the specified flags.

            HasAnyFlags(EObjectFlags FlagsToCheck)
                - Returns whether the object has any of the specified flags.
            
            HasAnyInternalFlags(EInternalObjectFlags InternalFlagsToCheck)
                - Return whether the object has any of the specified internal flags.

            ProcessConsoleExec(string Cmd, nil Reserved, UObject Executor)
                - Calls UObject::ProcessConsoleExec with the supplied params.

            type() -> string
                - Returns the type of this object as known by UE4SS
                - This does not return the type as known by Unreal

    UStruct
        Inheritance: UObject
        Methods
            GetSuperStruct() -> UClass
                - Returns the SuperStruct of this struct (can be invalid).

            ForEachFunction(function Callback)
                - Iterates every UFunction that belongs to this struct.
                - The callback has one param: UFunction Function.
                - Return true in the callback to stop iterating.

            ForEachProperty(function Callback)
                - Iterates every Property that belongs to this struct.
                - The callback has one param: Property Property.
                - Return true in the callback to stop iterating.

    UClass
        Inheritance: UClass
        Methods
            GetCDO() -> UClass
                - Returns the ClassDefaultObject of a UClass.

            IsChildOf(UClass Class) -> bool
                - Returns whether or not the class is a child of another class.

    AActor
        Inheritance: UObject
        Methods
            GetWorld() -> UObject | nil
                - Returns the UWorld that this actor belongs to
            
            GetLevel() -> UObject | nil
                - Returns the ULevel that this actor belongs to
    
    FName
        Inheritance: LocalObject
        Methods
            ToString() -> string
                - Returns the string for this FName
            
            GetComparisonIndex() -> integer
                - Returns the ComparisonIndex for this FName (index into global names array)
    
    TArray
        Inheritance: RemoteObject
        Methods
            __index(integer ArrayIndex)
                - Attempts to retrieve the value at the specified offset in the array
                - Can return any type, you can use the 'type()' function on the returned value to figure out what Lua class it's using (if non-trivial type)
            
            __newindex(integer ArrayIndex, auto NewValue)
                - Attempts to set the value at the specified offset in the array
            
            GetArrayAddress() -> integer
                - Returns the address in memory where the TArray struct is located
            
            GetArrayNum() -> integer
                - Returns the number of current elements in the array
            
            GetArrayMax() -> integer
                - Returns the maximum number of elements allowed in this array (aka capacity)
            
            GetArrayDataAddress -> integer
                - Returns the address in memory where the data for this array is stored
            
            ForEach(function Callback)
                - Iterates the entire TArray and calls the callback function for each element in the array
                - The callback params are: integer index, RemoteUnrealParam | LocalUnrealParam elem
                - Use 'elem:get()' and 'elem:set()' to access/mutate an array element

    UEnum
        Inheritance: RemoteObject
        Methods
            GetNameByValue(integer Value) -> FName
                - Returns the FName that corresponds to the specified value.
            ForEachName(LuaFunction Callback) -> FName
                - Iterates every FName/Value combination that belongs to this enum.
                - The callback has two params: FName Name, integer Value.
                - Return true in the callback to stop iterating.

    RemoteUnrealParam | LocalUnrealParam
        Inheritance: RemoteObject | LocalObject
            - This is a dynamic wrapper for any and all types & classes
            - Whether the Remote or Local variant is used depends on the requirements of the data but the usage is identical with either param types
        Methods
            get() -> auto
                - Returns the underlying value for this param
            
            set(auto NewValue)
                - Sets the underlying value for this param
            
            type() -> string
                - Returns "RemoteUnrealParam" or "LocalUnrealParam"
    
    UScriptStruct
        Inheritance: LocalObject
        Methods
            __index(string StructMemberVarName) -> auto
                - Attempts to return the value for the supplied variable
                - Can return any type, you can use the 'type()' function on the returned value to figure out what Lua class it's using (if non-trivial type)
            
            __newindex(string StructMemberVarName, auto NewValue)
                - Attempts to set the value for the supplied variable
            
            GetBaseAddress() -> integer
                - Returns the address in memory where the UObject that this UScriptStruct belongs to is located
            
            GetStructAddress() -> integer
                - Returns the address in memory where this UScriptStruct is located
            
            GetPropertyAddress() -> integer
                - Returns the address in memory where the corresponding U/FProperty is located

            IsValid() -> bool
                - Returns whether the struct is valid

            IsMappedToObject() -> bool
                - Returns whether the base object is valid

            IsMappedToProperty() -> bool
                - Returns whether the property is valid
            
            type() -> string
                - Returns "UScriptStruct"
    
    UFunction
        Inheritance: UObject
        Methods
            __call(UFunctionParams...)
                - Attempts to call the UFunction

            GetFunctionFlags() -> integer
                - Returns the flags for the UFunction.

            SetFunctionFlags(integer Flags)
                Sets the flags for the UFuction.

    FString
        Inheritance: RemoteObject
            - A TArray of characters
        Methods
            ToString()
                - Returns a string that Lua can understand

            Clear()
                - Clears the string by setting the number of elements in the TArray to 0

    FieldClass
        Inheritance: LocalObject
        Methods
            GetFName()
                - Returns the FName of this class by copy.

    Property
        Inheritance: RemoteObject
        Methods
            GetFullName() -> string
                - Returns the full name & path for this property.

            GetFName() -> FName
                - Returns the FName of this property by copy.
                - All FNames returned by '__index' are returned by reference.

            IsA(PropertyTypes PropertyType) -> bool
                - Returns true if the property is of type PropertyType.

            GetClass() -> PropertyClass

            ContainerPtrToValuePtr(UObjectDerivative Container, integer ArrayIndex) -> LightUserdata
                - Equivalent to FProperty::ContainerPtrToValuePtr<uint8> in UE.

            ImportText(string Buffer, LightUserdata Data, integer PortFlags, UObject OwnerObject)
                - Equivalent to FProperty::ImportText in UE, except without the 'ErrorText' param.

    ObjectProperty
        Inheritance: Property
        Methods
            GetPropertyClass() -> UClass
                - Returns the class that this property holds.

    BoolProperty
        Inheritance: Property
        Methods
            GetByteMask() -> integer
            GetByteOffset() -> integer
            GetFieldMask() -> integer
            GetFieldSize() -> integer

    StructProperty
        Inheritance: Property
        Methods
            GetStruct() -> UScriptStruct
                - Returns the UScriptStruct that's mapped to this property.

    ArrayProperty
        Inheritance: Property
        Methods
            GetInner() -> Property
                - Returns the inner property of the array.

    UObjectReflection
        Inheritance:
        Methods
            GetProperty(string PropertyName) -> Property
                - Returns a property meta-data object

    FOutputDevice
        Inheritance: RemoteObject
        Methods
            Log(string Message)
                - Logs a message to the output device (i.e: the in-game console)

    FWeakObjectPtr
        Inheritance: LocalObject
        Methods
            Get() -> UObjectDerivative
                - Returns the pointed to UObject or UObject derivative (can be invalid, so call UObject:IsValid after calling Get).
```
