---@meta

---@enum Key
Key = {
    LEFT_MOUSE_BUTTON = 0x1,
    RIGHT_MOUSE_BUTTON = 0x2,
    CANCEL = 0x3,
    MIDDLE_MOUSE_BUTTON = 0x4,
    XBUTTON_ONE = 0x5,
    XBUTTON_TWO = 0x6,
    BACKSPACE = 0x8,
    TAB = 0x9,
    CLEAR = 0x0C,
    RETURN = 0x0D,
    PAUSE = 0x13,
    CAPS_LOCK = 0x14,
    IME_KANA = 0x15,
    IME_HANGUEL = 0x15,
    IME_HANGUL = 0x15,
    IME_ON = 0x16,
    IME_JUNJA = 0x17,
    IME_FINAL = 0x18,
    IME_HANJA = 0x19,
    IME_KANJI = 0x19,
    IME_OFF = 0x1A,
    ESCAPE = 0x1B,
    IME_CONVERT = 0x1C,
    IME_NONCONVERT = 0x1D,
    IME_ACCEPT = 0x1E,
    IME_MODECHANGE = 0x1F,
    SPACE = 0x20,
    PAGE_UP = 0x21,
    PAGE_DOWN = 0x22,
    END = 0x23,
    HOME = 0x24,
    LEFT_ARROW = 0x25,
    UP_ARROW = 0x26,
    RIGHT_ARROW = 0x27,
    DOWN_ARROW = 0x28,
    SELECT = 0x29,
    PRINT = 0x2A,
    EXECUTE = 0x2B,
    PRINT_SCREEN = 0x2C,
    INS = 0x2D,
    DEL = 0x2E,
    HELP = 0x2F,
    ZERO = 0x30,
    ONE = 0x31,
    TWO = 0x32,
    THREE = 0x33,
    FOUR = 0x34,
    FIVE = 0x35,
    SIX = 0x36,
    SEVEN = 0x37,
    EIGHT = 0x38,
    NINE = 0x39,
    A = 0x41,
    B = 0x42,
    C = 0x43,
    D = 0x44,
    E = 0x45,
    F = 0x46,
    G = 0x47,
    H = 0x48,
    I = 0x49,
    J = 0x4A,
    K = 0x4B,
    L = 0x4C,
    M = 0x4D,
    N = 0x4E,
    O = 0x4F,
    P = 0x50,
    Q = 0x51,
    R = 0x52,
    S = 0x53,
    T = 0x54,
    U = 0x55,
    V = 0x56,
    W = 0x57,
    X = 0x58,
    Y = 0x59,
    Z = 0x5A,
    LEFT_WIN = 0x5B,
    RIGHT_WIN = 0x5C,
    APPS = 0x5D,
    SLEEP = 0x5F,
    NUM_ZERO = 0x60,
    NUM_ONE = 0x61,
    NUM_TWO = 0x62,
    NUM_THREE = 0x63,
    NUM_FOUR = 0x64,
    NUM_FIVE = 0x65,
    NUM_SIX = 0x66,
    NUM_SEVEN = 0x67,
    NUM_EIGHT = 0x68,
    NUM_NINE = 0x69,
    MULTIPLY = 0x6A,
    ADD = 0x6B,
    SEPARATOR = 0x6C,
    SUBTRACT = 0x6D,
    DECIMAL = 0x6E,
    DIVIDE = 0x6F,
    F1 = 0x70,
    F2 = 0x71,
    F3 = 0x72,
    F4 = 0x73,
    F5 = 0x74,
    F6 = 0x75,
    F7 = 0x76,
    F8 = 0x77,
    F9 = 0x78,
    F10 = 0x79,
    F11 = 0x7A,
    F12 = 0x7B,
    F13 = 0x7C,
    F14 = 0x7D,
    F15 = 0x7E,
    F16 = 0x7F,
    F17 = 0x80,
    F18 = 0x81,
    F19 = 0x82,
    F20 = 0x83,
    F21 = 0x84,
    F22 = 0x85,
    F23 = 0x86,
    F24 = 0x87,
    NUM_LOCK = 0x90,
    SCROLL_LOCK = 0x91,
    BROWSER_BACK = 0xA6,
    BROWSER_FORWARD = 0xA7,
    BROWSER_REFRESH = 0xA8,
    BROWSER_STOP = 0xA9,
    BROWSER_SEARCH = 0xAA,
    BROWSER_FAVORITES = 0xAB,
    BROWSER_HOME = 0xAC,
    VOLUME_MUTE = 0xAD,
    VOLUME_DOWN = 0xAE,
    VOLUME_UP = 0xAF,
    MEDIA_NEXT_TRACK = 0xB0,
    MEDIA_PREV_TRACK = 0xB1,
    MEDIA_STOP = 0xB2,
    MEDIA_PLAY_PAUSE = 0xB3,
    LAUNCH_MAIL = 0xB4,
    LAUNCH_MEDIA_SELECT = 0xB5,
    LAUNCH_APP1 = 0xB6,
    LAUNCH_APP2 = 0xB7,
    OEM_ONE = 0xBA,
    OEM_PLUS = 0xBB,
    OEM_COMMA = 0xBC,
    OEM_MINUS = 0xBD,
    OEM_PERIOD = 0xBE,
    OEM_TWO = 0xBF,
    OEM_THREE = 0xC0,
    OEM_FOUR = 0xDB,
    OEM_FIVE = 0xDC,
    OEM_SIX = 0xDD,
    OEM_SEVEN = 0xDE,
    OEM_EIGHT = 0xDF,
    OEM_102 = 0xE2,
    IME_PROCESS = 0xE5,
    PACKET = 0xE7,
    ATTN = 0xF6,
    CRSEL = 0xF7,
    EXSEL = 0xF8,
    EREOF = 0xF9,
    PLAY = 0xFA,
    ZOOM = 0xFB,
    PA1 = 0xFD,
    OEM_CLEAR = 0xFE,
}

---@alias PropertyTypes
---| `PropertyTypes.ObjectProperty`
---| `PropertyTypes.ObjectPtrProperty`
---| `PropertyTypes.Int8Property`
---| `PropertyTypes.Int16Property`
---| `PropertyTypes.IntProperty`
---| `PropertyTypes.Int64Property`
---| `PropertyTypes.NameProperty`
---| `PropertyTypes.FloatProperty`
---| `PropertyTypes.StrProperty`
---| `PropertyTypes.ByteProperty`
---| `PropertyTypes.UInt16Property`
---| `PropertyTypes.UIntProperty`
---| `PropertyTypes.UInt64Property`
---| `PropertyTypes.BoolProperty`
---| `PropertyTypes.ArrayProperty`
---| `PropertyTypes.MapProperty`
---| `PropertyTypes.StructProperty`
---| `PropertyTypes.ClassProperty`
---| `PropertyTypes.WeakObjectProperty`
---| `PropertyTypes.EnumProperty`
---| `PropertyTypes.TextProperty`
PropertyTypes = {}

---@class CustomPropertyInfo
---@field Name string Name to use with the __index metamethod
---@field Type PropertyTypes[] PropertyTypes
---@field BelongsToClass string Full class name without type that this property belongs to
---@field OffsetInternal OffsetInternalInfo|integer OffsetInternalInfo or integer offset from base to this property
---@field ArrayPropertyInfo ArrayPropertyInfo?

---@class OffsetInternalInfo
---@field Property string Name of the property to use as relative start instead of base
---@field RelativeOffset integer Offset from relative start to this property

---@class ArrayPropertyInfo
---@field Type PropertyTypes[]

----A table of object flags that can be or'd together by using |.
---@enum EObjectFlags
EObjectFlags = {
    RF_NoFlags                       = 0x00000000,
    RF_Public                        = 0x00000001,
    RF_Standalone                    = 0x00000002,
    RF_MarkAsNative                  = 0x00000004,
    RF_Transactional                 = 0x00000008,
    RF_ClassDefaultObject            = 0x00000010,
    RF_ArchetypeObject               = 0x00000020,
    RF_Transient                     = 0x00000040,
    RF_MarkAsRootSet                 = 0x00000080,
    RF_TagGarbageTemp                = 0x00000100,
    RF_NeedInitialization            = 0x00000200,
    RF_NeedLoad                      = 0x00000400,
    RF_KeepForCooker                 = 0x00000800,
    RF_NeedPostLoad                  = 0x00001000,
    RF_NeedPostLoadSubobjects        = 0x00002000,
    RF_NewerVersionExists            = 0x00004000,
    RF_BeginDestroyed                = 0x00008000,
    RF_FinishDestroyed               = 0x00010000,
    RF_BeingRegenerated              = 0x00020000,
    RF_DefaultSubObject              = 0x00040000,
    RF_WasLoaded                     = 0x00080000,
    RF_TextExportTransient           = 0x00100000,
    RF_LoadCompleted                 = 0x00200000,
    RF_InheritableComponentTemplate  = 0x00400000,
    RF_DuplicateTransient            = 0x00800000,
    RF_StrongRefOnFrame              = 0x01000000,
    RF_NonPIEDuplicateTransient      = 0x01000000,
    RF_Dynamic                       = 0x02000000,
    RF_WillBeLoaded                  = 0x04000000,
    RF_HasExternalPackage            = 0x08000000,
    RF_AllFlags                      = 0x0FFFFFFF,
}

---@enum ModifierKey
ModifierKey = {
    SHIFT = 0x10,
    CONTROL = 0x11,
    ALT = 0x12,
}


---A table of internal object flags that can be or'd together by using |.
---@enum EInternalObjectFlags
EInternalObjectFlags = {
    ReachableInCluster               = 0x00800000,
    ClusterRoot                      = 0x01000000,
    Native                           = 0x02000000,
    Async                            = 0x04000000,
    AsyncLoading                     = 0x08000000,
    Unreachable                      = 0x10000000,
    PendingKill                      = 0x20000000,
    RootSet                          = 0x40000000,
    GarbageCollectionKeepFlags       = 0x0E000000,
    AllFlags                         = 0x7F800000,
}

---@alias int8 integer
---@alias int16 integer
---@alias int32 integer
---@alias int64 integer
---@alias uint8 integer
---@alias uint16 integer
---@alias uint32 integer
---@alias uint64 integer
---@alias float number
---@alias double number


-- # Global Functions

---Creates an blank UObject whose IsValid function always returns false
---@return UObject
function CreateInvalidObject() end

---Dumps all objects and properties to UE4SS_ObjectDump.txt file
function DumpAllObjects() end

---Generates CXX Headers / CXXHeaderDump
function GenerateSDK() end

---Generates lua types in /Mods/shared/types
function GenerateLuaTypes() end

---Generates UHT Compatible Headers
function GenerateUHTCompatibleHeaders() end

---Dumps Static Meshes to *-ue4ss_static_mesh_data.csv file
function DumpStaticMeshes() end

---Dumps all current existing actors to *-ue4ss_actor_data.csv file
function DumpAllActors() end

--- Generates .usmap file
function DumpUSMAP() end

---@param ObjectName string
---@return UObject
function StaticFindObject(ObjectName) end

---Maps to https://docs.unrealengine.com/4.26/en-US/API/Runtime/CoreUObject/UObject/StaticFindObject/
---@param Class UClass?
---@param InOuter UObject?
---@param ObjectName string
---@param ExactClass boolean?
---@return UObject
function StaticFindObject(Class, InOuter, ObjectName, ExactClass) end

---Find the first non-default instance of the supplied class name
---@param ShortClassName string Should only contains the class name itself without path info
---@return UObject
function FindFirstOf(ShortClassName) end

---Find all non-default instances of the supplied class name
---@param ShortClassName string Should only contains the class name itself without path info
---@return UObject[]?
function FindAllOf(ShortClassName) end

--- Registers a callback for a key-bind
--- Callbacks can only be triggered while the game or debug console is on focus
---@param Key Key
---@param Callback fun()
function RegisterKeyBind(Key, Callback) end

--- Registers a callback for a key-bind
--- Callbacks can only be triggered while the game or debug console is on focus
---@param Key Key
---@param ModifierKeys ModifierKey[]
---@param Callback fun()
function RegisterKeyBind(Key, ModifierKeys, Callback) end

---Checks if, at the time of the invocation, the supplied keys have been registered
---@param Key integer
function IsKeyBindRegistered(Key) end

---Checks if, at the time of the invocation, the supplied keys have been registered
---@param Key integer
---@param ModifierKeys ModifierKey[]
function IsKeyBindRegistered(Key, ModifierKeys) end

--- Registers a callback for a UFunction
--- Callbacks are triggered when a UFunction is executed
--- The callback params are: UObject self, UFunctionParams...
--- Returns two ids, both of which must be passed to `UnregisterHook` if you want to unregister the hook.
---@param UFunctionName string
---@param Callback fun(self: UObject, ...)
---@return integer PreId
---@return integer PostId
function RegisterHook(UFunctionName, Callback) end

---Unregisters a hook.
---@param UFunctionName string
---@param PreId integer
---@param PostId integer
function UnregisterHook(UFunctionName, PreId, PostId) end

---Execute code inside the game thread using ProcessEvent.
---Will execute as soon as the game has time to execute.
---@param Callback fun()
function ExecuteInGameThread(Callback) end

---FName with "None" as value
NAME_None = FName(0)

---@enum EFindName
EFindName = {
    FNAME_Find = 0,
    FNAME_Add = 1
}

---Returns the FName for this string or the FName for "None" if the name doesn't exist
---@param Name string
---@return FName
function FName(Name) end

---Returns the FName for this ComparisonIndex or the FName for "None" if the name doesn't exist
---@param ComparisonIndex integer
---@return FName
function FName(ComparisonIndex) end

---Finds or adds FName for the string, depending on FindType 
---@param Name string
---@param FindType EFindName|integer # Find = 0, Add = 1
---@return FName
function FName(Name, FindType) end

---Finds or adds FName for the ComparisonIndex, depending on FindType 
---@param ComparisonIndex integer
---@param FindType EFindName|integer # Find = 0, Add = 1
---@return FName
function FName(ComparisonIndex, FindType) end

---Returns a new FText object with passed string as content
---@param Text string
---@return FText
function FText(Text) end

---Attempts to construct a UObject of the passed UClass
---(>=4.26) Maps to https://docs.unrealengine.com/4.27/en-US/API/Runtime/CoreUObject/UObject/StaticConstructObject_Internal/1/
---(<4.25) Maps to https://docs.unrealengine.com/4.27/en-US/API/Runtime/CoreUObject/UObject/StaticConstructObject_Internal/2/
---@param Class UClass
---@param Outer UObject
---@param Name FName?
---@param Flags EObjectFlags?
---@param InternalSetFlags EInternalObjectFlags?
---@param CopyTransientsFromClassDefaults boolean?
---@param AssumeTemplateIsArchetype boolean?
---@param Template UObject?
---@param InstanceGraph FObjectInstancingGraph?
---@param ExternalPackage UPackage?
---@param SubobjectOverrides any?
---@return UObject
function StaticConstructObject(Class, Outer, Name, Flags, InternalSetFlags, CopyTransientsFromClassDefaults, AssumeTemplateIsArchetype, Template, InstanceGraph, ExternalPackage, SubobjectOverrides) end

--- Registers a custom property to be used automatically with `UObject.__index`
---@param info CustomPropertyInfo
function RegisterCustomProperty(info) end

---Execute the callback function for each UObject in GUObjectArray
---The callback params are: UObject object, integer ChunkIndex, integer ObjectIndex
---@param Callback fun(object: UObject, ChunkIndex: integer, ObjectIndex: integer)
function ForEachUObject(Callback) end

---Executes the provided Lua function whenever an instance of the provided class is constructed.
---Inheritance is taken into account, so if you provide "/Script/Engine.Actor" as the class then it will execute your
---Lua function when any object is constructed that's either an AActor or is derived from AActor.
---@param UClassName string
---@param Callback fun()
function NotifyOnNewObject(UClassName, Callback) end

---Registers a callback that will get called when a BP function/event is called with the name 'EventName'.
---@param EventName string
---@param Callback function
function RegisterCustomEvent(EventName, Callback) end

---Registers a callback that will get called before AGameModeBase::InitGameState is called.
---The callback params are: AGameModeBase Context
---Params (except strings & bools & FOutputDevice) must be retrieved via 'Param:Get()' and set via 'Param:Set()'.
---@param Callback fun(Context: RemoteUnrealParam<AGameModeBase>)
function RegisterInitGameStatePreHook(Callback) end

---Registers a callback that will get called after AGameModeBase::InitGameState is called.
---The callback params are: AGameModeBase Context
---Params (except strings & bools & FOutputDevice) must be retrieved via 'Param:Get()' and set via 'Param:Set()'.
---@param Callback fun(Context: RemoteUnrealParam<AGameModeBase>)
function RegisterInitGameStatePostHook(Callback) end

---Registers a callback that will get called before AActor::BeginPlay is called.
---The callback params are: AActor Context
---Params (except strings & bools & FOutputDevice) must be retrieved via 'Param:Get()' and set via 'Param:Set()'.
---@param Callback fun(Context: RemoteUnrealParam<AActor>)
function RegisterBeginPlayPreHook(Callback) end

---Registers a callback that will get called after AActor::BeginPlay is called.
---The callback params are: AActor Context
---Params (except strings & bools & FOutputDevice) must be retrieved via 'Param:Get()' and set via 'Param:Set()'.
---@param Callback fun(Context: RemoteUnrealParam<AActor>)
function RegisterBeginPlayPostHook(Callback) end

---Registers a callback that will get called before UObject::ProcessConsoleExec is called.
---The callback params are: UObject Context, string Cmd, table CommandParts, FOutputDevice Ar, UObject Executor
---Params (except strings & bools & FOutputDevice) must be retrieved via 'Param:Get()' and set via 'Param:Set()'.
---If the callback returns nothing (or nil), the original return value of ProcessConsoleExec will be used.
---If the callback returns true or false, the supplied value will override the original return value of ProcessConsoleExec.
---@param Callback fun(Context: RemoteUnrealParam<UObject>, Cmd: string, CommandParts: table, Ar: FOutputDevice, Executor: RemoteUnrealParam<UObject>): boolean?
function RegisterProcessConsoleExecPreHook(Callback) end

---Registers a callback that will get called after UObject::ProcessConsoleExec is called.
---The callback params are: UObject Context, string Cmd, table CommandParts, FOutputDevice Ar, UObject Executor
---Params (except strings & bools & FOutputDevice) must be retrieved via 'Param:Get()' and set via 'Param:Set()'.
---If the callback returns nothing (or nil), the original return value of ProcessConsoleExec will be used.
---If the callback returns true or false, the supplied value will override the original return value of ProcessConsoleExec.
---@param Callback fun(Context: RemoteUnrealParam<UObject>, Cmd: string, CommandParts: table, Ar: FOutputDevice, Executor: RemoteUnrealParam<UObject>): boolean?
function RegisterProcessConsoleExecPostHook(Callback) end

---Registers a callback that will be called before UObject::CallFunctionByNameWithArguments is called.
---The callback params are: UObject Context, string Str, FOutputDevice Ar, UObject Executor, bool bForceCallWithNonExec
---Params (except strings & bools & FOutputDevice) must be retrieved via 'Param:Get()' and set via 'Param:Set()'.
---If the callback returns nothing (or nil), the original return value of CallFunctionByNameWithArguments will be used.
---If the callback returns true or false, the supplied value will override the original return value of CallFunctionByNameWithArguments.
---@param Callback fun(Context: RemoteUnrealParam<UObject>, Str: string, Ar: FOutputDevice, Executor: RemoteUnrealParam<UObject>, bForceCallWithNonExec: boolean): boolean?
function RegisterCallFunctionByNameWithArgumentsPreHook(Callback) end

---Registers a callback that will be called after UObject::CallFunctionByNameWithArguments is called.
---The callback params are: UObject Context, string Str, FOutputDevice Ar, UObject Executor, bool bForceCallWithNonExec
---Params (except strings & bools & FOutputDevice) must be retrieved via 'Param:Get()' and set via 'Param:Set()'.
---If the callback returns nothing (or nil), the original return value of CallFunctionByNameWithArguments will be used.
---If the callback returns true or false, the supplied value will override the original return value of CallFunctionByNameWithArguments.
---@param Callback fun(Context: RemoteUnrealParam<UObject>, Str: string, Ar: FOutputDevice, Executor: RemoteUnrealParam<UObject>, bForceCallWithNonExec: boolean): boolean?
function RegisterCallFunctionByNameWithArgumentsPostHook(Callback) end

---Registers a callback that will be called before ULocalPlayer::Exec is called.
---The callback params are: ULocalPlayer Context, UWorld InWorld, string Cmd, FOutputDevice Ar
---Params (except strings & bools & FOutputDevice) must be retrieved via 'Param:Get()' and set via 'Param:Set()'.
---The callback can have two return values.
---If the first return value is nothing (or nil), the original return value of Exec will be used.
---If the first return value is true or false, the supplied value will override the original return value of Exec.
---The second return value controls whether the original Exec will execute.
---If the second return value is nil or true, the orginal Exec will execute.
---If the second return value is false, the original Exec will not execute.
---@param Callback fun(Context: RemoteUnrealParam<UObject>, Str: string, Ar: FOutputDevice, Executor: UObject, bForceCallWithNonExec: boolean): boolean?, boolean?
function RegisterULocalPlayerExecPreHook(Callback) end

---Registers a callback that will be called after ULocalPlayer::Exec is called.
---The callback params are: ULocalPlayer Context, UWorld InWorld, string Cmd, FOutputDevice Ar
---Params (except strings & bools & FOutputDevice) must be retrieved via 'Param:Get()' and set via 'Param:Set()'.
---The callback can have two return values.
---If the first return value is nothing (or nil), the original return value of Exec will be used.
---If the first return value is true or false, the supplied value will override the original return value of Exec.
---The second return value controls whether the original Exec will execute.
---If the second return value is nil or true, the orginal Exec will execute.
---If the second return value is false, the original Exec will not execute.
---@param Callback fun(Context: RemoteUnrealParam<UObject>, Str: string, Ar: FOutputDevice, Executor: UObject, bForceCallWithNonExec: boolean): boolean?, boolean?
function RegisterULocalPlayerExecPostHook(Callback) end

---Registers a callback for a custom console commands.
---The callback only runs in the context of UGameViewportClient.
---The callback params are: string Cmd, table CommandParts, FOutputDevice Ar
---Params (except strings & bools & FOutputDevice) must be retrieved via 'Param:Get()' and set via 'Param:Set()'.
---The callback must return either true or false.
---If the callback returns true, no further handlers will be called for this command.
---@param CommandName string
---@param Callback fun(Cmd: string, CommandParts: table, Ar: FOutputDevice): boolean?
function RegisterConsoleCommandHandler(CommandName, Callback) end

---Registers a callback for a custom console command.
---Unlike 'RegisterConsoleCommandHandler', this global variant runs the callback for all contexts.
---The callback params are: string Cmd, table CommandParts, FOutputDevice Ar
---Params (except strings & bools & FOutputDevice) must be retrieved via 'Param:Get()' and set via 'Param:Set()'.
---The callback must return either true or false.
---If the callback returns true, no further handlers will be called for this command.
---@param CommandName string
---@param Callback fun(Cmd: string, CommandParts: table, Ar: FOutputDevice): boolean?
function RegisterConsoleCommandGlobalHandler(CommandName, Callback) end

---Asynchronously executes the specified function
---@param Callback fun()
function ExecuteAsync(Callback) end

---Asynchronously executes the specified function after the specified delay
---@param DelayInMilliseconds integer
---@param Callback fun()
function ExecuteWithDelay(DelayInMilliseconds, Callback) end

---Loads an asset by name.
---Must only be called from within the game thread.
---For example, from within a UFunction hook or RegisterConsoleCommandHandler callback.
---@param AssetPathAndName string
function LoadAsset(AssetPathAndName) end

---Finds an object by either class name or short object name.
---ClassName or ObjectShortName can be nil, but not both.
---Returns a UObject of a derivative of UObject.
---@param ClassName (string|FName)?
---@param ObjectShortName (string|FName)?
---@param RequiredFlags EObjectFlags
---@param BannedFlags EObjectFlags
---@return UObject
function FindObject(ClassName, ObjectShortName, RequiredFlags, BannedFlags) end

---Finds an object. Works the same way as the function by the same name in the UE source.
---comment
---@param InClass UClass
---@param InOuter UObject
---@param Name string
---@param ExactClass boolean
---@return UObject
function FindObject(InClass, InOuter, Name, ExactClass) end

---Finds the first specified number of objects by class name or short object name.
---To find all objects that match your criteria, set NumObjectsToFind to 0 or nil.
---Returns a table of UObject derivatives
---@param NumObjectsToFind integer
---@param ClassName (string|FName)?
---@param ObjectShortName (string|FName)?
---@param RequiredFlags EObjectFlags
---@param BannedFlags EObjectFlags
---@return UObject[]
function FindObjects(NumObjectsToFind, ClassName, ObjectShortName, RequiredFlags, BannedFlags, bExactClass) end

---Starts a loop that sleeps for the supplied number of milliseconds and stops when the callback returns true.
---@param DelayInMilliseconds integer
---@param Callback fun(): boolean
function LoopAsync(DelayInMilliseconds, Callback) end

---Returns a table of all game directories.
---An example of an absolute path to Win64: Q:\SteamLibrary\steamapps\common\Deep Rock Galactic\FSD\Binaries\Win64
---To get to the same directory, do: IterateGameDirectories().Game.Binaries.Win64
---Note that the game name is replaced by 'Game' to keep things generic.
---You can use `.__name` and `.__absolute_path` to retrieve values.
---You can use `.__files` to retrieve a table containing all files in this directory.
---You also use `.__name` and `.__absolute_path` for files.
function IterateGameDirectories() end


-- # Classes

---Class for interacting with UE4SS metadata
---@class UE4SS
UE4SS = {}

---Returns current version of UE4SS
---@return integer, integer, integer
function UE4SS:GetVersion() end

---Returns current version of UE4SS
---@return integer, integer, integer
function UE4SS.GetVersion() end

---Contains helper functions for retrieving which version of Unreal Engine that is being used.
---@class UnrealVersion
UnrealVersion = {}

---Returns major version of game's Unreal Engine
---@return integer
function UnrealVersion:GetMajor() end

---Returns major version of game's Unreal Engine
---@return integer
function UnrealVersion.GetMajor() end

---Returns minor version of game's Unreal Engine
---@return integer
function UnrealVersion:GetMinor() end

---Returns minor version of game's Unreal Engine
---@return integer
function UnrealVersion.GetMinor() end

---Compares game's Unreal Engine version
---@param MajorVersion integer
---@param MinorVersion integer
---@return boolean
function UnrealVersion:IsEqual(MajorVersion, MinorVersion) end

---Compares game's Unreal Engine version
---@param MajorVersion integer
---@param MinorVersion integer
---@return boolean
function UnrealVersion.IsEqual(MajorVersion, MinorVersion) end

---Compares game's Unreal Engine version
---@param MajorVersion integer
---@param MinorVersion integer
---@return boolean
function UnrealVersion:IsAtLeast(MajorVersion, MinorVersion) end

---Compares game's Unreal Engine version
---@param MajorVersion integer
---@param MinorVersion integer
---@return boolean
function UnrealVersion.IsAtLeast(MajorVersion, MinorVersion) end

---Compares game's Unreal Engine version
---@param MajorVersion integer
---@param MinorVersion integer
---@return boolean
function UnrealVersion:IsAtMost(MajorVersion, MinorVersion) end

---Compares game's Unreal Engine version
---@param MajorVersion integer
---@param MinorVersion integer
---@return boolean
function UnrealVersion.IsAtMost(MajorVersion, MinorVersion) end

---Compares game's Unreal Engine version
---@param MajorVersion integer
---@param MinorVersion integer
---@return boolean
function UnrealVersion:IsBelow(MajorVersion, MinorVersion) end

---Compares game's Unreal Engine version
---@param MajorVersion integer
---@param MinorVersion integer
---@return boolean
function UnrealVersion.IsBelow(MajorVersion, MinorVersion) end

---Compares game's Unreal Engine version
---@param MajorVersion integer
---@param MinorVersion integer
---@return boolean
function UnrealVersion:IsAbove(MajorVersion, MinorVersion) end

---Compares game's Unreal Engine version
---@param MajorVersion integer
---@param MinorVersion integer
---@return boolean
function UnrealVersion.IsAbove(MajorVersion, MinorVersion) end


---@class UFunction : UObject
local UFunction = {}

---Attempts to call the UFunction
---@param ... UFunctionParams
function UFunction:__call(...) end

---Returns the flags for the UFunction.
---@return integer
function UFunction:GetFunctionFlags() end

---Sets the flags for the UFuction.
---@param Flags integer
function UFunction:SetFunctionFlags(Flags) end


---A TArray of characters
---@class FString
local FString = {}

---Returns a string that Lua can understand
---@return string
function FString:ToString() end

---Clears the string by setting the number of elements in the TArray to 0
function FString:Clear() end


---@class FieldClass : LocalObject
local FieldClass = {}

---Returns the FName of this class by copy.
---@return FName
function FieldClass:GetFName() end


---@class FName
FName = {}

---Returns the content as string
---@return string
function FName:ToString() end

---Returns the ComparisonIndex (index in global names array)
---@return integer
function FName:GetComparisonIndex() end

---@class FText
FText = {}

---Returns the content as string
---@return string
function FText:ToString() end

---@class RemoteObject
local RemoteObject = {}

---Returns whether this object is valid or not
---@return boolean
function RemoteObject:IsValid() end


---@class Property : RemoteObject
local Property = {}
---Returns the full name & path for this property.
---@return string
function Property:GetFullName() end

---Returns the FName of this property by copy.
---All FNames returned by `__index` are returned by reference.
---@return FName
function Property:GetFName() end

---Returns true if the property is of type PropertyType.
---@param PropertyType PropertyTypes
---@return boolean
function Property:IsA(PropertyType) end

---@return PropertyClass
function Property:GetClass() end

---Equivalent to FProperty::ContainerPtrToValuePtr<uint8> in UE.
---@param Container UObjectDerivative
---@param ArrayIndex integer
---@return lightuserdata
function Property:ContainerPtrToValuePtr(Container, ArrayIndex) end

---Equivalent to FProperty::ImportText in UE, except without the 'ErrorText' param.
---@param Buffer string
---@param Data lightuserdata
---@param PortFlags integer
---@param OwnerObject UObject
function Property:ImportText(Buffer, Data, PortFlags, OwnerObject) end


---@class ObjectProperty : Property
local ObjectProperty = {}

---Returns the class that this property holds.
---@return UClass
function ObjectProperty:GetPropertyClass() end


---@class BoolProperty : Property
local BoolProperty = {}

---@return integer
function BoolProperty:GetByteMask() end

---@return integer
function BoolProperty:GetByteOffset() end

---@return integer
function BoolProperty:GetFieldMask() end

---@return integer
function BoolProperty:GetFieldSize() end


---@class StructProperty : Property
local StructProperty = {}

---Returns the UScriptStruct that's mapped to this property.
---@return UScriptStruct
function StructProperty:GetStruct() end


---@class ArrayProperty : Property
local ArrayProperty = {}

---Returns the inner property of the array.
---@return Property
function ArrayProperty:GetInner() end


---@class UObjectReflection
local UObjectReflection = {}

---Returns a property meta-data object
---@param PropertyName string
---@return Property
function UObjectReflection:GetProperty(PropertyName) end


---@class UObject : RemoteObject
local UObject = {}

---Attempts to return either a member variable or a callable UFunction
---Can return any type, you can use the `type()` function on the returned value to figure out what Lua class it's using (if non-trivial type)
---@param MemberVariableName string
---@return any
function UObject:__index(MemberVariableName) end

---Attempts to set the value of a member variable
---@param MemberVariableName string
---@param NewValue any
function UObject:__newindex(MemberVariableName, NewValue) end

---Returns the full name & path info for a UObject & its derivatives
---@return string
function UObject:GetFullName() end

---Returns the FName of this object by copy
---All FNames returned by `__index` are returned by reference
---@return FName
function UObject:GetFName() end

---Returns the memory address of this object
---@return integer
function UObject:GetAddress() end

---Returns the class of this object, this is equivalent to `UObject->ClassPrivate` in Unreal
---@return UClass
function UObject:GetClass() end

---Returns the Outer of this object
---@return UObject
function UObject:GetOuter() end

---Returns true if this UObject is a UClass or a derivative of UClass
---@return boolean
function UObject:IsAnyClass() end

---Returns a reflection object
---@return UObjectReflection
function UObject:Reflection() end

---Identical to __index
---@param MemberVariableName string
---@return any
function UObject:GetPropertyValue(MemberVariableName) end

---Identical to __newindex
---@param MemberVariableName string
---@param NewValue any
function UObject:SetPropertyValue(MemberVariableName, NewValue) end

---Returns whether this object is a UClass or UClass derivative
---@return boolean
function UObject:IsClass() end

---Returns the UWorld that this UObject is contained within.
---@return UWorld
function UObject:GetWorld() end

---Calls the supplied UFunction on this UObject.
---@param func UFunction
---@param ... any[]
function UObject:CallFunction(func, ...) end

---Returns whether this object is of the specified class.
---@param Class UClass
---@return boolean
function UObject:IsA(Class) end

---Returns whether this object is of the specified class.
---@param FullClassName string
---@return boolean
function UObject:IsA(FullClassName) end

---Returns whether the object has all of the specified flags.
---@param FlagsToCheck EObjectFlags
---@return boolean
function UObject:HasAllFlags(FlagsToCheck) end

---Returns whether the object has any of the specified flags.
---@param FlagsToCheck EObjectFlags
---@return boolean
function UObject:HasAnyFlags(FlagsToCheck) end

---Return whether the object has any of the specified internal flags.
---@param InternalFlagsToCheck EInternalObjectFlags
---@return boolean
function UObject:HasAnyInternalFlags(InternalFlagsToCheck) end

---Calls UObject::ProcessConsoleExec with the supplied params.
---@param Cmd string
---@param Reserved nil
---@param Executor UObject
function UObject:ProcessConsoleExec(Cmd, Reserved, Executor) end

---Returns the type of this object as known by UE4SS
---This does not return the type as known by Unreal
---@return string
function UObject:type() end


---@class UStruct : UObject
local UStruct = {}

---Returns the SuperStruct of this struct (can be invalid).
---@return UClass
function UStruct:GetSuperStruct() end

---Iterates every UFunction that belongs to this struct.
---The callback has one param: UFunction Function.
---Return true in the callback to stop iterating.
---@param Callback fun(Function: UFunction): boolean?
function UStruct:ForEachFunction(Callback) end

---Iterates every Property that belongs to this struct.
---The callback has one param: Property Property.
---Return true in the callback to stop iterating.
---@param Callback fun(Property: Property): boolean?
function UStruct:ForEachProperty(Callback) end


---@class UClass : UStruct
local UClass = {}

---Returns the ClassDefaultObject of a UClass.
---@return UClass
function UClass:GetCDO() end

---Returns whether or not the class is a child of another class.
---@param Class UClass
---@return boolean
function UClass:IsChildOf(Class) end


---@class TArray<T> : { [integer]: T }
local TArray = {}

---Return the address in memory where the TArray struct is located
---@return integer
function TArray:GetArrayAddress() end

---Return the number of current elements in the array (same as #TArray)
---@return integer
function TArray:GetArrayNum() end

---Return the maximum number of elements allowed in this array (array's capacity)
---@return integer
function TArray:GetArrayMax() end

---Return the address in memory where the data for this array is stored
---@return integer
function TArray:GetArrayDataAddress() end

---Clears the array
function TArray:Empty() end

---Iterates the entire `TArray` and calls the callback function for each element in the array.<br>
---The callback params are: `integer index`, `RemoteUnrealParam element` | `LocalUnrealParam element`.<br>
---Use `element:get()` and `element:set()` to access/mutate an array element.
---@param Callback fun(index: integer, element: RemoteUnrealParam)
function TArray:ForEach(Callback) end

---@class TSet<K> : { [K]: nil }

---@class TMap<K, V> : { [K]: V }
local TMap = {}

---Find the specified key in the map
---Throws an exception if the key is not found
---@generic K
---@generic V
---@param key K
---@return V
function TMap:Find(key) end

---Inserts a key/value pair into the map
---If the key already exists in the map, replaces the value
---@generic K
---@generic V
---@param key K
---@param value V
function TMap:Add(key, value) end

---Checks if a key exists inside of the map
---@generic K
---@param key K
---@return boolean
function TMap:Contains(key) end

---Removes an element from the map
---If the element doesn't exist, does nothing
---@generic K
---@param key K
function TMap:Remove(key) end

---Clears the map
function TMap:Empty() end

--- Iterates the entire `TMap` and calls the callback function for each element in the array
--- The callback params are: `RemoteUnrealParam key`, `RemoteUnrealParam value` | `LocalUnrealParam value`
--- Use `elem:get()` and `elem:set()` to access/mutate the value
--- Mutating the key is undefined behavior
--- @param callback fun(key: RemoteUnrealParam, value: RemoteUnrealParam)
function TMap:ForEach(callback) end

---@class TScriptInterface<K>


---This is a dynamic wrapper for any and all types & classes
---Whether the Remote or Local variant is used depends on the requirements of the data but the usage is identical with either param types
---@generic T
---@class RemoteUnrealParam<T> : RemoteObject<T>
local RemoteUnrealParam = {}

---Returns the underlying value for this param
---@generic T
---@param self RemoteUnrealParam<T>
---@return T
function RemoteUnrealParam:get() end

---Sets the underlying value for this param
---@generic T
---@param self RemoteUnrealParam<T>
---@param NewValue T
function RemoteUnrealParam:set(NewValue) end

---Returns "RemoteUnrealParam"
---@return 'RemoteUnrealParam'
function RemoteUnrealParam:type() end


---@class TSoftClassPtr<T> : string
---@class TSoftObjectPtr<T> : string
---@class TSubclassOf<T> : UClass


---@class UEnum
local UEnum = {}

--- Returns the `FName` that corresponds to the specified value.
---@param Value integer
function UEnum:GetNameByValue(Value) end

--- Iterates every `FName`, `Value` combination that belongs to this enum.
--- The callback has two params: `FName Name`, `integer Value`.
--- Return `true` in the callback to stop iterating.
---@param Callback function
function UEnum:ForEachName(Callback) end

--- Returns the `FName` and `Integer` value that coresponds the given `Index`.
---@param Index integer
function UEnum:GetEnumNameByIndex(Index) end

--- Inserts a `FName`/`Value` combination into a a `UEnum` at the given `Index`.
--- If `ShiftValues = true`, will shift all enum values greater than inserted value by one.
---@param Name string
---@param Value integer
---@param Index integer
function UEnum:InsertIntoNames(Name, Value, Index) end

--- Inserts a `FName`/`Value` combination into a a `UEnum` at the given `Index`.
--- If `ShiftValues = true`, will shift all enum values greater than inserted value by one.
---@param Name string
---@param Value integer
---@param Index integer
---@param ShiftValues boolean
function UEnum:InsertIntoNames(Name, Value, Index, ShiftValues) end

--- At a given `Index`, will modify the found element in the `UEnum` and replace it's `Name` with the given `NewName`.
---@param Index integer
---@param NewName string
function UEnum:EditNameAt(Index, NewName) end

--- At a given `Index`, will modify the found element in the `UEnum` and replace it's `Value` with the given `NewValue`.
---@param Index integer
---@param NewValue integer
function UEnum:EditValueAt(Index, NewValue) end

--- Will remove `Count` element(s) at the given `Index` from a `UEnum`.
--- If `AllowShrinkning = true`, will shrink the enum array when removing elements.
---@param Index integer
function UEnum:RemoveFromNamesAt(Index) end

--- Will remove `Count` element(s) at the given `Index` from a `UEnum`.
--- If `AllowShrinkning = true`, will shrink the enum array when removing elements.
---@param Index integer
---@param Count integer
function UEnum:RemoveFromNamesAt(Index, Count) end

--- Will remove `Count` element(s) at the given `Index` from a `UEnum`.
--- If `AllowShrinkning = true`, will shrink the enum array when removing elements.
---@param Index integer
---@param Count integer
---@param AllowShrinking boolean
function UEnum:RemoveFromNamesAt(Index, Count, AllowShrinking) end
