print("[LinuxReadOnlyConformance] FindFirstOf")
local package = FindFirstOf("Package")
assert(package ~= nil and package:IsValid(), "FindFirstOf(Package) failed")
assert(package:GetClass():GetName() == "Package", "GetClass/GetName failed")

print("[LinuxReadOnlyConformance] StaticFindObject")
local core = StaticFindObject("/Script/CoreUObject")
assert(core ~= nil and core:IsValid(), "StaticFindObject(/Script/CoreUObject) failed")
local invalid = CreateInvalidObject()
assert(type(invalid) == "userdata" and not invalid:IsValid(), "CreateInvalidObject returned a live or nil value")
assert(not StaticFindObject("/Script/DefinitelyMissingUE4SSLinuxObject"):IsValid(),
    "StaticFindObject did not use UE4SS invalid-object semantics")
assert(FindObject("Package", FName("/Script/CoreUObject"), 0, 0) == core,
    "FindObject class/FName overload failed")
assert(FindObject(core:GetClass(), FName("/Script/CoreUObject"), 0, 0) == core,
    "FindObject UClass overload failed")
assert(StaticFindObject(core:GetClass(), nil, "/Script/CoreUObject", true) == core,
    "StaticFindObject full overload failed")

local current_thread = GetCurrentThreadId()
assert(current_thread:type() == "ThreadId" and #current_thread:ToString() > 0,
    "ThreadId userdata is incomplete")
assert(current_thread == GetMainModThreadId() and IsInMainModThread() and not IsInAsyncThread(),
    "main mod thread identity is incorrect")
assert(GetAsyncThreadId() ~= current_thread, "async and main mod thread ids are identical")

local iterated_uobjects = 0
ForEachUObject(function(object, chunk_index, object_index)
    assert(type(object) == "userdata" and type(chunk_index) == "number" and
           type(object_index) == "number" and chunk_index >= 0 and
           object_index >= 0 and object_index < 65536,
        "ForEachUObject callback arguments are invalid")
    iterated_uobjects = iterated_uobjects + 1
    return true -- This API intentionally ignores callback return values, matching upstream UE4SS.
end)
assert(iterated_uobjects > 1000, "ForEachUObject did not traverse the live object registry")
print("[LinuxReadOnlyConformance] object discovery compatibility passed: " ..
    tostring(iterated_uobjects) .. " UObjects")

print("[LinuxReadOnlyConformance] FindAllOf")
local packages = FindAllOf("Package")
assert(type(packages) == "table" and #packages > 0, "FindAllOf(Package) failed")

local game_engine = FindFirstOf("GameEngine")
assert(game_engine ~= nil and game_engine:IsValid(), "live UGameEngine instance was not found")
assert(EngineTickAvailable == true, "validated game-thread scheduler is unavailable")
assert(ProcessEventAvailable == true, "validated ProcessEvent resolver is unavailable")
assert(NativeUFunctionHooksAvailable == true, "native UFunction hook manager is unavailable")
assert(BlueprintHooksAvailable == true, "Blueprint hook manager is unavailable")
assert(BeginPlayHooksAvailable == true, "native AActor::BeginPlay hook manager is unavailable")
assert(EndPlayHooksAvailable == true, "native AActor::EndPlay hook manager is unavailable")
assert(InitGameStateHooksAvailable == true,
    "native AGameModeBase::InitGameState hook manager is unavailable")
assert(ObjectNotificationsAvailable == true, "object notification manager is unavailable")
assert(StaticConstructObjectAvailable == true, "StaticConstructObject resolver is unavailable")
local fixed_frame_rate = game_engine.bUseFixedFrameRate
assert(type(fixed_frame_rate) == "boolean", "reflected UEngine BoolProperty read failed")
local game_state = FindFirstOf("GameStateBase")
assert(game_state ~= nil and game_state:IsValid(), "live GameStateBase instance was not found")
local player_array = game_state.PlayerArray
assert(type(player_array) == "userdata" and player_array:IsValid(),
    "reflected AGameStateBase TArray property did not produce TArray userdata")
assert(#player_array == player_array:GetArrayNum() and player_array:GetArrayMax() >= player_array:GetArrayNum(),
    "TArray length/header methods disagree")
local game_player_count = 0
player_array:ForEach(function(index, player)
    assert(index == game_player_count + 1 and player:get() ~= nil,
        "TArray UObject element wrapper is invalid")
    game_player_count = game_player_count + 1
end)
assert(game_player_count == player_array:GetArrayNum(), "TArray ForEach count disagrees with Num")
local player_array_metadata_seen = false
for _, reflected_object in ipairs({ game_engine, game_state }) do
    reflected_object:GetClass():ForEachProperty(function(property)
        local property_name = property:GetName()
        local property_type = property:GetClass():GetName()
        assert(type(property:GetOffset_Internal()) == "number" and type(property:GetSize()) == "number",
            "FProperty numeric metadata is unavailable")
        if property_name == "PlayerArray" then
            assert(property_type == "ArrayProperty", "PlayerArray FProperty type metadata is incorrect")
            player_array_metadata_seen = true
        end
        if property_type == "SetProperty" or property_type == "MapProperty" then
            print("[LinuxReadOnlyConformance] discovered " .. property_type .. " " ..
                reflected_object:GetClass():GetName() .. "." .. property_name)
        end
    end)
end
assert(player_array_metadata_seen, "UStruct:ForEachProperty did not enumerate inherited PlayerArray metadata")
local readable_set_seen = false
local readable_map_seen = false
local nonempty_set_seen = false
local nonempty_map_seen = false
local readable_weak_seen = false
local readable_delegate_seen = false
local readable_multicast_seen = false
local readable_sparse_multicast_seen = false
local sparse_multicast_object = nil
local sparse_multicast_name = nil
local sparse_multicast_wrapper = nil
local readable_interface_seen = false
local writable_interface_object = nil
local writable_interface_name = nil
local writable_interface_value = nil
local writable_interface_class = nil
local writable_interface_original_nil = false
local interface_live_target_seen = false
local nil_interface_slots = {}
local readable_enum_seen = false
local readable_soft_object_seen = false
local soft_object_property_seen = false
local soft_object_last_error = nil
local readable_text_seen = false
local text_property_seen = false
local text_property_last_error = nil
local writable_text_object = nil
local writable_text_name = nil
local writable_text_value = nil
local writable_set_object = nil
local writable_set_name = nil
local writable_set_snapshot = nil
local writable_set_element = nil
local writable_map_object = nil
local writable_map_name = nil
local writable_map_snapshot = nil
local writable_map_key = nil
local writable_map_value = nil
local writable_enum_object = nil
local writable_enum_name = nil
local writable_enum_value = nil
local visited_container_classes = {}
local visited_container_class_objects = {}
local function probe_container_properties(object, force_class_probe)
    local actor_class_instance = object:GetClass()
    local actor_class_name = actor_class_instance:GetName()
    if force_class_probe or not visited_container_classes[actor_class_name] then
        if not visited_container_classes[actor_class_name] then
            table.insert(visited_container_class_objects, actor_class_instance)
        end
        visited_container_classes[actor_class_name] = true
        actor_class_instance:ForEachProperty(function(property)
            local property_type = property:GetClass():GetName()
            if property_type == "TextProperty" and not readable_text_seen then
                text_property_seen = true
                local property_name = property:GetName()
                local read_ok, text_value = pcall(function() return object[property_name] end)
                if read_ok and type(text_value) == "userdata" and text_value:type() == "FText" and
                   type(text_value:ToString()) == "string" then
                    assert(tostring(text_value) == text_value:ToString(),
                        "FText __tostring and ToString disagree")
                    readable_text_seen = true
                    writable_text_object = object
                    writable_text_name = property_name
                    writable_text_value = text_value
                    print("[LinuxReadOnlyConformance] real TextProperty read: " ..
                        actor_class_name .. "." .. property_name .. " value=" .. text_value:ToString())
                else
                    text_property_last_error = read_ok and
                        "property returned an incompatible Lua value" or tostring(text_value)
                end
            elseif property_type == "EnumProperty" and not readable_enum_seen then
                local property_name = property:GetName()
                local read_ok, enum_value = pcall(function() return object[property_name] end)
                local enum_table = _G["Enum_" .. property_name]
                if read_ok and type(enum_value) == "number" and type(enum_table) == "table" then
                    local entry_count = 0
                    local value_is_declared = false
                    for _, value in pairs(enum_table) do
                        entry_count = entry_count + 1
                        value_is_declared = value_is_declared or value == enum_value
                    end
                    assert(entry_count > 0 and value_is_declared,
                        "Enum_<PropertyName> table does not describe the real EnumProperty value")
                    readable_enum_seen = true
                    writable_enum_object = object
                    writable_enum_name = property_name
                    writable_enum_value = enum_value
                    print("[LinuxReadOnlyConformance] real EnumProperty read: " ..
                        actor_class_name .. "." .. property_name)
                end
            elseif property_type == "SetProperty" or property_type == "MapProperty" then
                local property_name = property:GetName()
                local read_ok, container = pcall(function() return object[property_name] end)
                if read_ok and type(container) == "userdata" then
                    local should_log = false
                    if property_type == "SetProperty" and container:type() == "TSet" then
                        assert(container:IsValid() and type(#container) == "number", "real TSet metadata is invalid")
                        container:ForEach(function(_) end)
                        should_log = not readable_set_seen or (#container > 0 and not nonempty_set_seen)
                        readable_set_seen = true
                        nonempty_set_seen = nonempty_set_seen or #container > 0
                        if #container > 0 and
                           (writable_set_snapshot == nil or #container < #writable_set_snapshot) then
                            writable_set_object = object
                            writable_set_name = property_name
                            writable_set_snapshot = container
                            writable_set_element = nil
                            container:ForEach(function(element)
                                writable_set_element = element
                                return true
                            end)
                        end
                    elseif property_type == "MapProperty" and container:type() == "TMap" then
                        assert(container:IsValid() and type(#container) == "number", "real TMap metadata is invalid")
                        container:ForEach(function(_, _) end)
                        should_log = not readable_map_seen or (#container > 0 and not nonempty_map_seen)
                        readable_map_seen = true
                        nonempty_map_seen = nonempty_map_seen or #container > 0
                        if #container > 0 and
                           (writable_map_snapshot == nil or #container < #writable_map_snapshot) then
                            writable_map_object = object
                            writable_map_name = property_name
                            writable_map_snapshot = container
                            writable_map_key = nil
                            writable_map_value = nil
                            container:ForEach(function(key, value)
                                writable_map_key = key
                                writable_map_value = value
                                return true
                            end)
                        end
                    end
                    if should_log then
                        print("[LinuxReadOnlyConformance] real " .. property_type .. " read: " ..
                            actor_class_name .. "." .. property_name .. " entries=" .. tostring(#container))
                    end
                end
            elseif (property_type == "WeakObjectProperty" and not readable_weak_seen) or
                   (property_type == "DelegateProperty" and not readable_delegate_seen) or
                   ((property_type == "MulticastDelegateProperty" or
                     property_type == "MulticastInlineDelegateProperty") and not readable_multicast_seen) or
                   (property_type == "MulticastSparseDelegateProperty" and
                    not readable_sparse_multicast_seen) or
                   ((property_type == "SoftObjectProperty" or property_type == "SoftClassProperty") and
                    not readable_soft_object_seen) or
                   (property_type == "InterfaceProperty" and not interface_live_target_seen) then
                if property_type == "SoftObjectProperty" or property_type == "SoftClassProperty" then
                    soft_object_property_seen = true
                end
                local read_ok, value = pcall(function() return object[property:GetName()] end)
                if read_ok then
                    if property_type == "WeakObjectProperty" and type(value) == "userdata" and
                       value:type() == "FWeakObjectPtr" then
                        value:Get()
                        readable_weak_seen = true
                    elseif property_type == "DelegateProperty" and type(value) == "table" and
                           value.FunctionName ~= nil then
                        readable_delegate_seen = true
                    elseif (property_type == "MulticastDelegateProperty" or
                            property_type == "MulticastInlineDelegateProperty") and
                           type(value) == "userdata" and
                           value:type() == "MulticastDelegateProperty" then
                        local bindings = value:GetBindings()
                        assert(type(bindings) == "table" and #bindings == #value,
                            "live multicast delegate wrapper disagrees with its binding snapshot")
                        readable_multicast_seen = true
                    elseif property_type == "MulticastSparseDelegateProperty" and
                           type(value) == "userdata" and
                           value:type() == "MulticastSparseDelegateProperty" then
                        local bindings = value:GetBindings()
                        assert(type(bindings) == "table" and #bindings == #value,
                            "live sparse multicast delegate wrapper disagrees with its binding snapshot")
                        readable_sparse_multicast_seen = true
                        sparse_multicast_object = object
                        sparse_multicast_name = property:GetName()
                        sparse_multicast_wrapper = value
                    elseif (property_type == "SoftObjectProperty" or property_type == "SoftClassProperty") and
                           type(value) == "userdata" and value:type() == "TSoftObjectPtrUserdata" then
                        local object_id = value:GetObjectID()
                        assert(value:GetWeakPtr():type() == "FWeakObjectPtr" and
                               type(value:GetTagAtLastTest()) == "number" and
                               object_id:type() == "FSoftObjectPathUserdata" and
                               object_id:GetAssetPathName() ~= nil and
                               object_id:GetSubPathString() ~= nil,
                            "real soft object wrapper metadata is invalid")
                        readable_soft_object_seen = true
                    elseif property_type == "InterfaceProperty" and (value == nil or type(value) == "userdata") then
                        local interface_class = property:GetInterfaceClass()
                        assert(interface_class ~= nil and interface_class:IsValid(),
                            "FInterfaceProperty:GetInterfaceClass returned an invalid UClass")
                        readable_interface_seen = true
                        if type(value) == "userdata" and value:IsValid() then
                            writable_interface_object = object
                            writable_interface_name = property:GetName()
                            writable_interface_value = value
                            writable_interface_class = interface_class
                            writable_interface_original_nil = false
                            interface_live_target_seen = true
                            assert(value:ImplementsInterface(interface_class),
                                "non-null FScriptInterface object does not implement InterfaceClass")
                        else
                            if #nil_interface_slots < 16 then
                                table.insert(nil_interface_slots, {
                                    Object = object,
                                    Name = property:GetName(),
                                    InterfaceClass = interface_class
                                })
                            end
                            if writable_interface_object == nil then
                                writable_interface_object = object
                                writable_interface_name = property:GetName()
                                writable_interface_class = interface_class
                                writable_interface_original_nil = true
                            end
                        end
                    end
                    print("[LinuxReadOnlyConformance] real " .. property_type .. " read: " ..
                        actor_class_name .. "." .. property:GetName())
                    if (property_type == "SoftObjectProperty" or property_type == "SoftClassProperty") and
                       not readable_soft_object_seen then
                        soft_object_last_error = "property returned an incompatible Lua value"
                    end
                elseif property_type == "SoftObjectProperty" or property_type == "SoftClassProperty" then
                    soft_object_last_error = tostring(value)
                end
            end
        end)
    end
end
local function probe_soft_object_properties(object)
    local unreal_class = object:GetClass()
    local class_name = unreal_class:GetName()
    unreal_class:ForEachProperty(function(property)
        local property_type = property:GetClass():GetName()
        if property_type == "SoftObjectProperty" or property_type == "SoftClassProperty" then
            soft_object_property_seen = true
            local property_name = property:GetName()
            local read_ok, value = pcall(function() return object[property_name] end)
            if not read_ok then
                soft_object_last_error = tostring(value)
                return false
            end
            if type(value) ~= "userdata" or value:type() ~= "TSoftObjectPtrUserdata" then
                soft_object_last_error = "property returned an incompatible Lua value"
                return false
            end
            local object_id = value:GetObjectID()
            assert(value:GetWeakPtr():type() == "FWeakObjectPtr" and
                   type(value:GetTagAtLastTest()) == "number" and
                   object_id:type() == "FSoftObjectPathUserdata" and
                   object_id:GetAssetPathName() ~= nil and
                   object_id:GetSubPathString() ~= nil,
                "real soft object wrapper metadata is invalid")
            readable_soft_object_seen = true
            print("[LinuxReadOnlyConformance] real " .. property_type .. " read from CDO: " ..
                class_name .. "." .. property_name)
            return true
        end
        return false
    end)
end
for _, actor in ipairs(FindAllOf("Actor")) do
    probe_container_properties(actor)
    if nonempty_set_seen and nonempty_map_seen and readable_weak_seen and
       interface_live_target_seen and readable_enum_seen and readable_soft_object_seen and
       readable_text_seen and readable_sparse_multicast_seen then break end
end
if not readable_soft_object_seen then
    local known_blueprint_cdo = StaticFindObject(
        "/Game/Pal/Blueprint/UI/BossDemo/WBP_BossDemoBase.Default__WBP_BossDemoBase_C")
    if known_blueprint_cdo ~= nil then
        probe_container_properties(known_blueprint_cdo, true)
    end
end
if not readable_soft_object_seen then
    for _, unreal_class in ipairs(visited_container_class_objects) do
        local cdo_ok, cdo = pcall(function() return unreal_class:GetCDO() end)
        if cdo_ok and cdo ~= nil and cdo:IsValid() then
            probe_container_properties(cdo, true)
            if readable_soft_object_seen then break end
        end
    end
end
if not readable_map_seen then
    for _, class_name in ipairs({
        "PalGameInstance", "PalWorldSubsystem", "PalGameState", "PalGameMode", "PalGroupManager",
        "PalPlayerManager", "PalCharacterManager", "PalBaseCampManager", "PalMapObjectManager",
        "PalItemContainerManager", "PalDynamicItemDataManager", "PalSaveGameManager", "PalQuestManager",
        "PalLocationManager", "PalTimeManager", "PalStageWorldSubsystem", "PalStorageSubsystem",
        "PalWorldObjectRecordWorldSubsystem", "PalArenaWorldSubsystem", "NetDriver"
    }) do
        local candidate = FindFirstOf(class_name)
        if candidate ~= nil then probe_container_properties(candidate) end
        if readable_map_seen then break end
    end
end
if not readable_soft_object_seen or not readable_text_seen or not readable_sparse_multicast_seen or
   not interface_live_target_seen then
    local classes = FindObjects(0, "Class", nil,
        EObjectFlags.RF_NoFlags, EObjectFlags.RF_NoFlags, true)
    print("[LinuxReadOnlyConformance] scanning " .. tostring(#classes) .. " loaded UClass CDOs")
    for _, unreal_class in ipairs(classes) do
        local cdo_ok, cdo = pcall(function() return unreal_class:GetCDO() end)
        if cdo_ok and cdo ~= nil and cdo:IsValid() then
            probe_container_properties(cdo, true)
            probe_soft_object_properties(cdo)
            if readable_soft_object_seen and readable_text_seen and readable_sparse_multicast_seen and
               interface_live_target_seen then break end
        end
    end
end
if not readable_soft_object_seen then
    for _, class_name in ipairs({
        "DataAsset", "PrimaryDataAsset", "DataTable", "CurveTable", "ActorComponent",
        "SceneComponent", "WorldSubsystem", "GameInstanceSubsystem", "EngineSubsystem",
        "DeveloperSettings", "WorldSettings", "GameStateBase", "GameInstance"
    }) do
        local candidates = FindAllOf(class_name) or {}
        for _, candidate in ipairs(candidates) do
            probe_container_properties(candidate)
            if readable_soft_object_seen then break end
        end
        if readable_soft_object_seen then break end
    end
end
if not interface_live_target_seen then
    for _, class_name in ipairs({"Actor", "ActorComponent", "SceneComponent", "Object"}) do
        local candidates = FindAllOf(class_name) or {}
        for _, candidate in ipairs(candidates) do
            for _, slot in ipairs(nil_interface_slots) do
                if candidate:ImplementsInterface(slot.InterfaceClass) then
                    writable_interface_object = slot.Object
                    writable_interface_name = slot.Name
                    writable_interface_value = candidate
                    writable_interface_class = slot.InterfaceClass
                    writable_interface_original_nil = true
                    interface_live_target_seen = true
                    break
                end
            end
            if interface_live_target_seen then break end
        end
        if interface_live_target_seen then break end
    end
end
assert(readable_set_seen, "no readable real Palworld SetProperty was found")
assert(readable_map_seen, "no readable real Palworld MapProperty was found")
assert(nonempty_set_seen, "no non-empty real Palworld SetProperty was found")
assert(nonempty_map_seen, "no non-empty real Palworld MapProperty was found")
assert(writable_set_object ~= nil and writable_set_snapshot ~= nil and writable_set_element ~= nil,
    "no non-empty real SetProperty candidate was retained for the write gate")
assert(writable_map_object ~= nil and writable_map_snapshot ~= nil and writable_map_key ~= nil,
    "no non-empty real MapProperty candidate was retained for the write gate")
assert(readable_weak_seen, "no readable real Palworld WeakObjectProperty was found")
assert(readable_sparse_multicast_seen and sparse_multicast_object ~= nil and
       sparse_multicast_name ~= nil and sparse_multicast_wrapper ~= nil,
    "no readable real Palworld MulticastSparseDelegateProperty was found")
assert(readable_interface_seen and writable_interface_object ~= nil and
       writable_interface_name ~= nil and writable_interface_class ~= nil,
    "no readable real Palworld InterfaceProperty metadata was found")
assert(not soft_object_property_seen or readable_soft_object_seen,
    "a real SoftObjectProperty/SoftClassProperty was found but decoding failed: " ..
    tostring(soft_object_last_error))
assert(readable_soft_object_seen, "no readable real Palworld SoftObjectProperty/SoftClassProperty was found")
assert(not text_property_seen or readable_text_seen,
    "a real TextProperty was found but decoding failed: " .. tostring(text_property_last_error))
assert(readable_text_seen, "no readable real Palworld TextProperty was found")
assert(writable_text_object ~= nil and writable_text_value ~= nil,
    "no real TextProperty candidate was retained for the write gate")
assert(readable_enum_seen and writable_enum_object ~= nil,
    "no readable real Palworld EnumProperty was found")
local enum_objects = FindAllOf("Enum")
assert(type(enum_objects) == "table" and #enum_objects > 0, "no real UEnum object was found")
local real_enum = enum_objects[1]
local real_enum_count = real_enum:NumEnums()
assert(type(real_enum_count) == "number" and real_enum_count > 0, "real UEnum::Names is empty")
local first_enum_name, first_enum_value = real_enum:GetEnumNameByIndex(0)
assert(first_enum_name ~= nil and type(first_enum_value) == "number" and
       real_enum:GetNameByValue(first_enum_value) == first_enum_name,
    "real UEnum index/value lookup disagrees")
local iterated_enum_count = 0
real_enum:ForEachName(function(name, value)
    assert(name ~= nil and type(value) == "number", "UEnum:ForEachName delivered invalid data")
    iterated_enum_count = iterated_enum_count + 1
end)
assert(iterated_enum_count == real_enum_count, "UEnum iteration count disagrees with NumEnums")
local game_mode = FindFirstOf("GameModeBase")
assert(game_mode ~= nil and game_mode:IsValid(), "live GameModeBase instance was not found")
assert(type(game_mode.GetNumPlayers) == "function", "UFunction lookup through UStruct::Children failed")
local gameplay_statics = StaticFindObject("/Script/Engine.Default__GameplayStatics")
assert(gameplay_statics ~= nil and gameplay_statics:IsValid(), "GameplayStatics CDO was not found")
assert(type(gameplay_statics.SpawnObject) == "function", "GameplayStatics.SpawnObject was not found")
local object_class = StaticFindObject("/Script/CoreUObject.Object")
assert(object_class ~= nil and object_class:IsValid(), "CoreUObject.Object UClass was not found")
local actor_class = StaticFindObject("/Script/Engine.Actor")
assert(actor_class ~= nil and actor_class:IsValid(), "Engine.Actor UClass was not found")
local kismet_string_library = StaticFindObject("/Script/Engine.Default__KismetStringLibrary")
assert(kismet_string_library ~= nil and kismet_string_library:IsValid(), "KismetStringLibrary CDO was not found")
local kismet_text_library = StaticFindObject("/Script/Engine.Default__KismetTextLibrary")
assert(kismet_text_library ~= nil and kismet_text_library:IsValid(), "KismetTextLibrary CDO was not found")
local kismet_math_library = StaticFindObject("/Script/Engine.Default__KismetMathLibrary")
assert(kismet_math_library ~= nil and kismet_math_library:IsValid(), "KismetMathLibrary CDO was not found")
assert(type(kismet_math_library.MakeVector) == "function", "KismetMathLibrary.MakeVector was not found")
assert(type(kismet_math_library.BreakVector) == "function", "KismetMathLibrary.BreakVector was not found")
local constructed_name = FName("LinuxConformanceName")
assert(constructed_name:ToString() == "LinuxConformanceName", "FName SysV constructor/ToString failed")
assert(NAME_None == FName("None", EFindName.FNAME_Find), "NAME_None/FNAME_Find compatibility failed")
local constructed_string = FString("Linux UTF-8 ✓")
assert(constructed_string:ToString() == "Linux UTF-8 ✓" and constructed_string:Len() == 13,
    "FString UTF-8 constructor failed")
local constructed_utf8_string = FUtf8String("Linux UTF-8 ✓")
assert(constructed_utf8_string:type() == "FUtf8String" and
       constructed_utf8_string:ToString() == "Linux UTF-8 ✓" and
       constructed_utf8_string:StartsWith("Linux") and
       constructed_utf8_string:EndsWith("✓"),
    "FUtf8String constructor or methods failed")
local constructed_ansi_string = FAnsiString("PalServer")
constructed_ansi_string:Append(" Linux")
assert(constructed_ansi_string:type() == "FAnsiString" and
       constructed_ansi_string:ToString() == "PalServer Linux" and
       constructed_ansi_string:Find("Linux") == 11,
    "FAnsiString constructor or methods failed")
local game_directories = IterateGameDirectories()
assert(type(game_directories) == "table" and
       type(game_directories.Game) == "table" and
       type(game_directories.Game.Binaries.Linux) == "table" and
       type(game_directories.Game.Content) == "table" and
       game_directories.Game.__name == "Pal",
    "IterateGameDirectories did not expose the native PalServer layout")
assert(CreateLogicModsDirectory() == true,
    "CreateLogicModsDirectory could not create or validate Content/Paks/LogicMods")
print("[LinuxReadOnlyConformance] native game-directory and narrow-string APIs passed")
local constructed_text = FText("Linux FText ✓")
assert(constructed_text:type() == "FText" and constructed_text:ToString() == "Linux FText ✓" and
       tostring(constructed_text) == "Linux FText ✓" and constructed_text == FText("Linux FText ✓"),
    "FText UTF-8 constructor/snapshot API failed")
local blueprint_test_cdo = StaticFindObject("/Game/Pal/Blueprint/UI/BossDemo/WBP_BossDemoBase.Default__WBP_BossDemoBase_C")
assert(blueprint_test_cdo ~= nil and blueprint_test_cdo:IsValid(), "Blueprint conformance CDO was not found")
print("[LinuxReadOnlyConformance] Blueprint test CDO: " .. tostring(blueprint_test_cdo))

local immediate_ran = false
local delayed_ran = false
local async_ran = false
local async_thread_identity_ok = false
local loop_count = 0
local new_object_seen = false
local begin_play_seen = false
local begin_play_post_seen = false
local end_play_seen = false
local end_play_post_seen = false
local end_play_target = nil
local escaped_end_play_reason = nil
local init_game_state_pre_count = 0
local init_game_state_post_count = 0
local load_map_pre_count = 0
local load_map_post_count = 0
local process_console_exec_pre_count = 0
local process_console_exec_post_count = 0
local call_function_by_name_pre_count = 0
local call_function_by_name_post_count = 0
local local_player_exec_pre_count = 0
local local_player_exec_post_count = 0
local controlled_delay_count = 0
local explicit_delay_count = 0
local frame_delay_count = 0
local frame_loop_count = 0
local retrigger_count = 0

assert(type(LoadAsset) == "function", "LoadAsset compatibility global is unavailable")
assert(type(LoadExport) == "function", "LoadExport compatibility global is unavailable")
assert(type(RegisterCustomProperty) == "function",
    "RegisterCustomProperty compatibility global is unavailable")
local ue4ss_major, ue4ss_minor, ue4ss_hotfix = UE4SS.GetVersion()
assert(type(ue4ss_major) == "number" and type(ue4ss_minor) == "number" and
       type(ue4ss_hotfix) == "number", "UE4SS.GetVersion is unavailable")
assert(UnrealVersion.GetMajor() == 5 and UnrealVersion.GetMinor() == 1 and
       UnrealVersion.IsEqual(5, 1) and UnrealVersion.IsAtLeast(5, 0),
    "UnrealVersion does not describe the validated UE 5.1 runtime")
assert(FPackageName.IsShortPackageName("Pal") and
       FPackageName.IsValidLongPackageName("/Game/Pal"),
    "FPackageName compatibility helpers are unavailable")
assert(type(ModRef) == "table" and ModRef:type() == "ModRef",
    "ModRef compatibility object is unavailable")
ModRef:SetSharedVariable("__UE4SS_LINUX_CONFORMANCE", "ready")
assert(ModRef:GetSharedVariable("__UE4SS_LINUX_CONFORMANCE") == "ready",
    "ModRef shared-variable storage failed")
assert(CustomPropertiesAvailable == true,
    "validated custom property descriptors are unavailable")
assert(type(PropertyTypes) == "table" and
       PropertyTypes.IntProperty.Name == "IntProperty" and
       PropertyTypes.IntProperty.Size == 4 and
       PropertyTypes.ArrayProperty.Size == 16 and
       PropertyTypes.ObjectProperty.FFieldClassPointer ~= 0,
    "PropertyTypes does not match the UE4SS compatibility contract")
assert(type(loadexport) == "function", "loadexport compatibility alias is unavailable")
assert(type(LoadExport("malloc")) == "number" and LoadExport("malloc") ~= 0,
    "LoadExport could not resolve a globally loaded ELF symbol")
assert(LoadExport("__ue4ss_linux_missing_export__") == 0,
    "LoadExport did not return address 0 for a missing ELF symbol")
assert(LoadExport() == nil, "LoadExport did not preserve UE4SS invalid-argument behavior")
assert(type(DumpAllObjects) == "function", "DumpAllObjects compatibility global is unavailable")
assert(DumpAllObjectsAvailable == true,
    "DumpAllObjects has no validated UE4SS output directory")
assert(type(IsKeyBindRegistered) == "function", "IsKeyBindRegistered compatibility global is absent")
assert(type(RegisterKeyBind) == "function", "RegisterKeyBind compatibility global is absent")
assert(type(RegisterKeyBindAsync) == "function", "RegisterKeyBindAsync compatibility global is absent")
assert(type(GenerateSDK) == "function", "GenerateSDK compatibility global is absent")
assert(type(GenerateLuaTypes) == "function", "GenerateLuaTypes compatibility global is absent")
assert(type(GenerateUHTCompatibleHeaders) == "function",
    "GenerateUHTCompatibleHeaders compatibility global is absent")
assert(type(DumpUSMAP) == "function", "DumpUSMAP compatibility global is absent")
assert(type(DumpStaticMeshes) == "function", "DumpStaticMeshes compatibility global is absent")
assert(type(DumpAllActors) == "function", "DumpAllActors compatibility global is absent")
local input_ok, input_error = pcall(RegisterKeyBind, 0, function() end)
assert(not input_ok and input_error:find("UE4SS_WITH_INPUT=OFF", 1, true),
    "headless input API did not report its compile-time capability boundary")
local sdk_ok, sdk_error = pcall(GenerateSDK)
assert(not sdk_ok and sdk_error:find("UE4SS_WITH_SDK_GENERATOR=OFF", 1, true),
    "headless SDK API did not report its compile-time capability boundary")
local uvtd_ok, uvtd_error = pcall(DumpUSMAP)
assert(not uvtd_ok and uvtd_error:find("UE4SS_WITH_UVTD=OFF", 1, true),
    "headless USMAP API did not report its compile-time capability boundary")
local gui_ok, gui_error = pcall(DumpStaticMeshes)
assert(not gui_ok and gui_error:find("UE4SS_WITH_GUI=OFF", 1, true),
    "headless GUI dumper API did not report its compile-time capability boundary")
assert(InputAvailable == false and GUIAvailable == false and
    SDKGeneratorAvailable == false and UVTDAvailable == false,
    "headless feature capability globals are inconsistent")
assert(type(RunOnGameThread) == "function", "RunOnGameThread compatibility alias is unavailable")
assert(type(RegisterBeginPlayPreHook) == "function", "RegisterBeginPlayPreHook is unavailable")
assert(type(RegisterBeginPlayPostHook) == "function", "RegisterBeginPlayPostHook is unavailable")
assert(type(RegisterEndPlayPreHook) == "function", "RegisterEndPlayPreHook is unavailable")
assert(type(RegisterEndPlayPostHook) == "function", "RegisterEndPlayPostHook is unavailable")
assert(type(RegisterInitGameStatePreHook) == "function",
    "RegisterInitGameStatePreHook is unavailable")
assert(type(RegisterInitGameStatePostHook) == "function",
    "RegisterInitGameStatePostHook is unavailable")
assert(type(RegisterLoadMapPreHook) == "function", "RegisterLoadMapPreHook is unavailable")
assert(type(RegisterLoadMapPostHook) == "function", "RegisterLoadMapPostHook is unavailable")
assert(LoadMapHooksAvailable == true, "native UEngine::LoadMap hook manager is unavailable")
assert(type(RegisterProcessConsoleExecPreHook) == "function",
    "RegisterProcessConsoleExecPreHook is unavailable")
assert(type(RegisterProcessConsoleExecPostHook) == "function",
    "RegisterProcessConsoleExecPostHook is unavailable")
assert(type(RegisterConsoleCommandHandler) == "function",
    "RegisterConsoleCommandHandler is unavailable")
assert(type(RegisterConsoleCommandGlobalHandler) == "function",
    "RegisterConsoleCommandGlobalHandler is unavailable")
assert(ProcessConsoleExecHooksAvailable == true,
    "native UObject::ProcessConsoleExec hook manager is unavailable")
assert(type(RegisterCallFunctionByNameWithArgumentsPreHook) == "function",
    "RegisterCallFunctionByNameWithArgumentsPreHook is unavailable")
assert(type(RegisterCallFunctionByNameWithArgumentsPostHook) == "function",
    "RegisterCallFunctionByNameWithArgumentsPostHook is unavailable")
assert(CallFunctionByNameHooksAvailable == true,
    "native UObject::CallFunctionByNameWithArguments hook manager is unavailable")
assert(type(RegisterULocalPlayerExecPreHook) == "function",
    "RegisterULocalPlayerExecPreHook is unavailable")
assert(type(RegisterULocalPlayerExecPostHook) == "function",
    "RegisterULocalPlayerExecPostHook is unavailable")
assert(ULocalPlayerExecHooksAvailable == true,
    "native ULocalPlayer::Exec hook manager is unavailable")
assert(type(RegisterCustomEvent) == "function", "RegisterCustomEvent is unavailable")
assert(type(UnregisterCustomEvent) == "function", "UnregisterCustomEvent is unavailable")
assert(CustomEventsAvailable == true,
    "ProcessLocalScriptFunction custom-event dispatch is unavailable")
assert(type(RestartCurrentMod) == "function", "RestartCurrentMod is unavailable")
assert(type(UninstallCurrentMod) == "function", "UninstallCurrentMod is unavailable")
assert(type(RestartMod) == "function", "RestartMod is unavailable")
assert(type(UninstallMod) == "function", "UninstallMod is unavailable")
assert(ModLifecycleAvailable == true,
    "queued Lua mod lifecycle controller is unavailable")
local custom_event_probe_count = 0
RegisterCustomEvent("__UE4SSLinuxConformanceNeverFires", function()
    custom_event_probe_count = custom_event_probe_count + 1
end)
UnregisterCustomEvent("__ue4sslinuxconformanceneverfires")
assert(custom_event_probe_count == 0,
    "custom-event registration probe fired unexpectedly")

RegisterBeginPlayPreHook(function(Actor)
    local actor = Actor:get()
    assert(actor ~= nil and actor:IsValid(), "RegisterBeginPlayPreHook delivered a stale actor")
    begin_play_seen = true
end)

RegisterBeginPlayPostHook(function(Actor)
    local actor = Actor:get()
    assert(actor ~= nil and actor:IsValid(), "RegisterBeginPlayPostHook delivered a stale actor")
    begin_play_post_seen = true
end)

RegisterEndPlayPreHook(function(Actor, EndPlayReason)
    local actor = Actor:get()
    if actor == end_play_target then
        local reason = EndPlayReason:get()
        assert(reason == 0, "K2_DestroyActor did not use EEndPlayReason::Destroyed")
        EndPlayReason:set(reason) -- Exercise UE4SS's mutable native scalar wrapper idempotently.
        escaped_end_play_reason = EndPlayReason
        end_play_seen = true
    end
end)

RegisterEndPlayPostHook(function(Actor, EndPlayReason)
    local actor = Actor:get()
    if actor == end_play_target then
        assert(EndPlayReason:get() == 0, "EndPlay reason changed between pre/post callbacks")
        end_play_post_seen = true
    end
end)

-- The initial InitGameState normally happens before deferred mod activation.
-- Registration itself must still succeed, and these callbacks validate any
-- subsequent map transition without forcing a destructive reload in this test.
RegisterInitGameStatePreHook(function(Context)
    local game_mode = Context:get()
    assert(game_mode ~= nil and game_mode:IsValid(),
        "RegisterInitGameStatePreHook delivered a stale GameMode")
    init_game_state_pre_count = init_game_state_pre_count + 1
end)

RegisterInitGameStatePostHook(function(Context)
    local game_mode = Context:get()
    assert(game_mode ~= nil and game_mode:IsValid(),
        "RegisterInitGameStatePostHook delivered a stale GameMode")
    init_game_state_post_count = init_game_state_post_count + 1
end)

-- The initial LoadMap also happens before deferred mod activation. Registration
-- validates the public API without forcing server travel or touching a save.
local function validate_load_map_callback(Engine, World, URL, PendingGame, Error)
    local engine = Engine:get()
    local world = World:get()
    local pending_game = PendingGame:get()
    assert(engine ~= nil and engine:IsValid(), "LoadMap delivered a stale UEngine")
    assert(world ~= nil and world:IsValid(), "LoadMap delivered a stale UWorld")
    assert(pending_game == nil or pending_game:IsValid(),
        "LoadMap delivered a stale UPendingNetGame")
    assert(type(URL) == "userdata" and URL:type() == "FURL",
        "LoadMap did not deliver FURL userdata")
    assert(type(Error) == "string", "LoadMap did not deliver the FString error snapshot")
end

RegisterLoadMapPreHook(function(Engine, World, URL, PendingGame, Error)
    validate_load_map_callback(Engine, World, URL, PendingGame, Error)
    load_map_pre_count = load_map_pre_count + 1
    return nil
end)

RegisterLoadMapPostHook(function(Engine, World, URL, PendingGame, Error)
    validate_load_map_callback(Engine, World, URL, PendingGame, Error)
    load_map_post_count = load_map_post_count + 1
    return nil
end)

local function validate_process_console_exec_callback(Context, Command, Parameters, Ar, Executor)
    local context = Context:get()
    local executor = Executor:get()
    assert(context ~= nil and context:IsValid(),
        "ProcessConsoleExec delivered a stale UObject context")
    assert(type(Command) == "string" and type(Parameters) == "table",
        "ProcessConsoleExec command snapshot has the wrong shape")
    assert(type(Ar) == "userdata" and Ar:type() == "FOutputDevice",
        "ProcessConsoleExec did not deliver FOutputDevice userdata")
    assert(executor == nil or executor:IsValid(),
        "ProcessConsoleExec delivered a stale executor")
end

RegisterProcessConsoleExecPreHook(function(Context, Command, Parameters, Ar, Executor)
    validate_process_console_exec_callback(Context, Command, Parameters, Ar, Executor)
    process_console_exec_pre_count = process_console_exec_pre_count + 1
    return nil
end)

RegisterProcessConsoleExecPostHook(function(Context, Command, Parameters, Ar, Executor)
    validate_process_console_exec_callback(Context, Command, Parameters, Ar, Executor)
    process_console_exec_post_count = process_console_exec_post_count + 1
    return nil
end)

local function validate_call_function_by_name_callback(
        Context, Command, Ar, Executor, ForceCallWithNonExec)
    local context = Context:get()
    local executor = Executor:get()
    assert(context ~= nil and context:IsValid(),
        "CallFunctionByNameWithArguments delivered a stale UObject context")
    assert(type(Command) == "string",
        "CallFunctionByNameWithArguments command snapshot is invalid")
    assert(type(Ar) == "userdata" and Ar:type() == "FOutputDevice",
        "CallFunctionByNameWithArguments did not deliver FOutputDevice userdata")
    assert(executor == nil or executor:IsValid(),
        "CallFunctionByNameWithArguments delivered a stale executor")
    assert(type(ForceCallWithNonExec) == "boolean",
        "CallFunctionByNameWithArguments force flag is invalid")
end

RegisterCallFunctionByNameWithArgumentsPreHook(function(
        Context, Command, Ar, Executor, ForceCallWithNonExec)
    validate_call_function_by_name_callback(
        Context, Command, Ar, Executor, ForceCallWithNonExec)
    call_function_by_name_pre_count = call_function_by_name_pre_count + 1
    return nil
end)

RegisterCallFunctionByNameWithArgumentsPostHook(function(
        Context, Command, Ar, Executor, ForceCallWithNonExec)
    validate_call_function_by_name_callback(
        Context, Command, Ar, Executor, ForceCallWithNonExec)
    call_function_by_name_post_count = call_function_by_name_post_count + 1
    return nil
end)

local function validate_local_player_exec_callback(Context, InWorld, Command, Ar)
    local context = Context:get()
    local world = InWorld:get()
    assert(context ~= nil and context:IsValid() and context:IsA("LocalPlayer"),
        "ULocalPlayer::Exec delivered a stale LocalPlayer context")
    assert(world == nil or world:IsValid(),
        "ULocalPlayer::Exec delivered a stale UWorld")
    assert(type(Command) == "string",
        "ULocalPlayer::Exec command snapshot is invalid")
    assert(type(Ar) == "userdata" and Ar:type() == "FOutputDevice",
        "ULocalPlayer::Exec did not deliver FOutputDevice userdata")
end

RegisterULocalPlayerExecPreHook(function(Context, InWorld, Command, Ar)
    validate_local_player_exec_callback(Context, InWorld, Command, Ar)
    local_player_exec_pre_count = local_player_exec_pre_count + 1
    return nil, nil
end)

RegisterULocalPlayerExecPostHook(function(Context, InWorld, Command, Ar)
    validate_local_player_exec_callback(Context, InWorld, Command, Ar)
    local_player_exec_post_count = local_player_exec_post_count + 1
    return nil, nil
end)

RegisterConsoleCommandGlobalHandler("ue4ss_linux_probe", function(Command, Parameters, Ar)
    assert(Command == "ue4ss_linux_probe" and type(Parameters) == "table" and
           Ar:type() == "FOutputDevice", "global console command handler arguments are invalid")
    Ar:Log("UE4SS Linux console command probe")
    return true
end)

-- Dedicated servers normally have no UGameViewportClient invocation. The
-- non-global registration still has to remain available for portable mods.
RegisterConsoleCommandHandler("ue4ss_linux_viewport_probe", function(Command, Parameters, Ar)
    assert(type(Command) == "string" and type(Parameters) == "table" and
           Ar:type() == "FOutputDevice", "viewport console command handler arguments are invalid")
    return true
end)

NotifyOnNewObject("/Script/CoreUObject.Object", function(Object)
    assert(Object ~= nil and Object:IsValid(), "NotifyOnNewObject delivered a stale object")
    new_object_seen = true
    print("[LinuxReadOnlyConformance] NotifyOnNewObject: " .. Object:GetFullName())
    return true
end)

assert(type(ExecuteInGameThreadWithDelay) == "function" and
       type(RetriggerableExecuteInGameThreadWithDelay) == "function" and
       type(ExecuteInGameThreadAfterFrames) == "function" and
       type(LoopInGameThreadWithDelay) == "function" and
       type(LoopInGameThreadAfterFrames) == "function" and
       type(CancelDelayedAction) == "function" and
       type(MakeActionHandle) == "function",
    "game-thread delayed-action compatibility globals are unavailable")

local clear_probe_one = ExecuteInGameThreadWithDelay(60000, function()
    error("cleared delayed action executed")
end)
local clear_probe_two = LoopInGameThreadAfterFrames(60000, function()
    error("cleared frame action executed")
end)
assert(ClearAllDelayedActions() == 2 and
       not IsValidDelayedActionHandle(clear_probe_one) and
       not IsValidDelayedActionHandle(clear_probe_two),
    "ClearAllDelayedActions did not clear exactly the current mod's probe actions")

local cancelled_action = ExecuteInGameThreadWithDelay(60000, function()
    error("cancelled delayed action executed")
end)
assert(IsValidDelayedActionHandle(cancelled_action) and
       IsDelayedActionActive(cancelled_action) and
       GetDelayedActionRate(cancelled_action) == 60000,
    "new delayed action status metadata is invalid")
assert(PauseDelayedAction(cancelled_action) and
       IsDelayedActionPaused(cancelled_action) and
       GetDelayedActionTimeRemaining(cancelled_action) >= 0 and
       UnpauseDelayedAction(cancelled_action) and
       ResetDelayedActionTimer(cancelled_action) and
       SetDelayedActionTimer(cancelled_action, 61000) and
       GetDelayedActionRate(cancelled_action) == 61000 and
       CancelDelayedAction(cancelled_action) and
       not IsValidDelayedActionHandle(cancelled_action),
    "delayed action pause/reset/set/cancel lifecycle failed")

local controlled_action = ExecuteInGameThreadWithDelay(60000, function()
    controlled_delay_count = controlled_delay_count + 1
end)
assert(PauseDelayedAction(controlled_action) and
       SetDelayedActionTimer(controlled_action, 0) and
       IsDelayedActionActive(controlled_action),
    "controlled delayed action could not be reactivated")

local explicit_action = MakeActionHandle()
ExecuteInGameThreadWithDelay(explicit_action, 0, function()
    explicit_delay_count = explicit_delay_count + 1
end)
ExecuteInGameThreadWithDelay(explicit_action, 0, function()
    explicit_delay_count = explicit_delay_count + 100
end)

local retrigger_action = MakeActionHandle()
RetriggerableExecuteInGameThreadWithDelay(retrigger_action, 60000, function()
    retrigger_count = retrigger_count + 1
end)
assert(RetriggerableExecuteInGameThreadWithDelay(retrigger_action, 0, function()
    retrigger_count = retrigger_count + 100
end) == retrigger_action, "retriggerable action did not preserve its handle")

local frame_delay_action = ExecuteInGameThreadAfterFrames(2, function()
    frame_delay_count = frame_delay_count + 1
end)
assert(GetDelayedActionRate(frame_delay_action) == 2,
    "frame delayed action did not report its frame rate")
local frame_loop_action
frame_loop_action = LoopInGameThreadAfterFrames(1, function()
    frame_loop_count = frame_loop_count + 1
    if frame_loop_count >= 2 then
        assert(CancelDelayedAction(frame_loop_action), "frame loop could not cancel itself")
    end
end)

local time_loop_action
time_loop_action = LoopInGameThreadWithDelay(0, function()
    if explicit_delay_count > 0 then
        assert(CancelDelayedAction(time_loop_action), "time loop could not cancel itself")
    end
end)

ExecuteInGameThread(function()
    assert(IsInGameThread() and GetCurrentThreadId() == GetGameThreadId(),
        "game-thread identity helpers disagree inside ExecuteInGameThread")
    local constructed_object = StaticConstructObject(
        object_class,
        game_engine,
        NAME_None,
        EObjectFlags.RF_Transient,
        EInternalObjectFlags.None,
        false,
        false,
        nil,
        nil,
        nil,
        nil)
    assert(constructed_object ~= nil and constructed_object:IsValid() and
           constructed_object:GetClass() == object_class,
        "native StaticConstructObject did not create a transient UObject")
    print("[LinuxReadOnlyConformance] StaticConstructObject passed: " ..
        constructed_object:GetFullName())
    local sphere_asset, was_asset_found, did_asset_load = LoadAsset(
        "/Game/Pal/Effect/Common/Hit/HitSneakAttack/NS_CriticalCatch.NS_CriticalCatch")
    assert(sphere_asset ~= nil and sphere_asset:IsValid() and
           was_asset_found == true and did_asset_load == true,
        "LoadAsset did not return the known FullSphereSummon Niagara asset")
    print("[LinuxReadOnlyConformance] LoadAsset passed: " .. sphere_asset:GetFullName())

    assert(type(gameplay_statics.BeginDeferredActorSpawnFromClass) == "function" and
           type(gameplay_statics.FinishSpawningActor) == "function",
        "GameplayStatics actor spawn functions are unavailable")
    local identity_transform = {
        Rotation = { X = 0.0, Y = 0.0, Z = 0.0, W = 1.0 },
        Translation = { X = 0.0, Y = 0.0, Z = 0.0 },
        Scale3D = { X = 1.0, Y = 1.0, Z = 1.0 },
    }
    local spawned_actor = gameplay_statics:BeginDeferredActorSpawnFromClass(
        game_state, actor_class, identity_transform, 1, nil)
    assert(spawned_actor ~= nil and spawned_actor:IsValid(),
        "BeginDeferredActorSpawnFromClass did not create a transient AActor")
    local finished_actor = gameplay_statics:FinishSpawningActor(spawned_actor, identity_transform)
    assert(finished_actor ~= nil and finished_actor:IsValid(), "FinishSpawningActor failed")
    assert(begin_play_seen == true and begin_play_post_seen == true,
        "native AActor::BeginPlay pre/post hooks did not dispatch")
    print("[LinuxReadOnlyConformance] native AActor::BeginPlay pre/post dispatch passed")
    end_play_target = finished_actor
    finished_actor:K2_DestroyActor()
    assert(end_play_seen == true and end_play_post_seen == true,
        "native AActor::EndPlay pre/post hooks did not dispatch")
    local escaped_end_play_ok, escaped_end_play_error = pcall(function()
        return escaped_end_play_reason:get()
    end)
    assert(not escaped_end_play_ok and
           string.find(escaped_end_play_error, "no longer valid", 1, true) ~= nil,
        "native EndPlay RemoteUnrealParam escaped its callback epoch")
    print("[LinuxReadOnlyConformance] native AActor::EndPlay pre/post dispatch passed")

    -- Exercise the guarded write path without changing server behaviour.
    game_engine.bUseFixedFrameRate = fixed_frame_rate
    assert(game_engine.bUseFixedFrameRate == fixed_frame_rate, "idempotent game-thread property write failed")
    writable_enum_object[writable_enum_name] = writable_enum_value
    assert(writable_enum_object[writable_enum_name] == writable_enum_value,
        "real Palworld EnumProperty idempotent write failed")
    if interface_live_target_seen then
        if writable_interface_original_nil then
            assert(writable_interface_object[writable_interface_name] == nil,
                "real Palworld FScriptInterface nil precondition changed")
            writable_interface_object[writable_interface_name] = writable_interface_value
            assert(writable_interface_object[writable_interface_name] == writable_interface_value,
                "real Palworld FScriptInterface live assignment failed")
            writable_interface_object[writable_interface_name] = nil
            assert(writable_interface_object[writable_interface_name] == nil,
                "real Palworld FScriptInterface nil restoration failed")
        else
            writable_interface_object[writable_interface_name] = writable_interface_value
            assert(writable_interface_object[writable_interface_name] == writable_interface_value,
                "real Palworld FScriptInterface idempotent write failed")
        end
    else
        writable_interface_object[writable_interface_name] = nil
        assert(writable_interface_object[writable_interface_name] == nil,
            "real Palworld null FScriptInterface idempotent write failed")
    end
    print("[LinuxReadOnlyConformance] real FScriptInterface mutation passed: " ..
        writable_interface_object:GetClass():GetName() .. "." .. writable_interface_name ..
        " mode=" .. (interface_live_target_seen and
            (writable_interface_original_nil and "live-add-restore" or "idempotent-live") or
            "idempotent-null"))
    writable_text_object[writable_text_name] = writable_text_value
    assert(writable_text_object[writable_text_name] == writable_text_value,
        "real Palworld TextProperty idempotent lifecycle write failed")
    local converted_text = kismet_text_library:Conv_StringToText("Linux FText ProcessEvent ✓")
    assert(converted_text:type() == "FText" and
           converted_text:ToString() == "Linux FText ProcessEvent ✓",
        "FString -> FText ProcessEvent construction/return lifecycle failed")
    local text_hook_path = "/Script/Engine.KismetTextLibrary:Conv_TextToString"
    local text_hook_count = 0
    local text_hook_pre_id, text_hook_post_id = RegisterHook(
        text_hook_path,
        function(Context, InText)
            assert(Context:get() == kismet_text_library and
                   InText:get():ToString() == "Linux FText ProcessEvent ✓",
                "FText input hook parameter decode failed")
            InText:set(FText("Linux FText Hook ✓"))
        end,
        function(Context, ReturnValue, InText)
            assert(InText:get():ToString() == "Linux FText Hook ✓" and
                   ReturnValue:get():ToString() == "Linux FText Hook ✓",
                "FText hook parameter mutation did not reach Conv_TextToString")
            text_hook_count = text_hook_count + 1
        end)
    local converted_text_string = kismet_text_library:Conv_TextToString(converted_text)
    assert(converted_text_string:ToString() == "Linux FText Hook ✓" and text_hook_count == 1,
        "FText UFunction input lifecycle or hook mutation failed")
    UnregisterHook(text_hook_path, text_hook_pre_id, text_hook_post_id)
    local sparse_count = #sparse_multicast_wrapper
    local sparse_target = game_mode
    local sparse_function_name = "GetNumPlayers"
    for _, binding in ipairs(sparse_multicast_wrapper:GetBindings()) do
        assert(not (binding.Object == sparse_target and
                    binding.FunctionName:ToString() == sparse_function_name),
            "sparse multicast conformance binding unexpectedly already exists")
    end
    sparse_multicast_wrapper:Add(sparse_target, sparse_function_name)
    assert(#sparse_multicast_wrapper == sparse_count + 1,
        "MulticastSparseDelegateProperty:Add did not add a live binding")
    sparse_multicast_wrapper:Add(sparse_target, FName(sparse_function_name))
    assert(#sparse_multicast_wrapper == sparse_count + 1,
        "MulticastSparseDelegateProperty:Add did not preserve AddUnique semantics")
    sparse_multicast_wrapper:Remove(sparse_target, sparse_function_name)
    assert(#sparse_multicast_wrapper == sparse_count,
        "MulticastSparseDelegateProperty:Remove did not restore the original binding count")
    print("[LinuxReadOnlyConformance] real sparse multicast mutation passed: " ..
        sparse_multicast_object:GetClass():GetName() .. "." .. sparse_multicast_name)
    local set_count = #writable_set_snapshot
    writable_set_snapshot:Add(writable_set_element)
    assert(#writable_set_snapshot == set_count,
        "TSet:Add changed the size when replacing an identical element")
    writable_set_object[writable_set_name] = writable_set_snapshot
    local rewritten_set = writable_set_object[writable_set_name]
    assert(type(rewritten_set) == "userdata" and rewritten_set:type() == "TSet" and
           #rewritten_set == set_count and rewritten_set:Contains(writable_set_element),
        "real Palworld SetProperty transactional rewrite failed")
    local map_count = #writable_map_snapshot
    writable_map_snapshot:Add(writable_map_key, writable_map_value)
    assert(#writable_map_snapshot == map_count,
        "TMap:Add changed the size when replacing an existing key")
    writable_map_object[writable_map_name] = writable_map_snapshot
    local rewritten_map = writable_map_object[writable_map_name]
    assert(type(rewritten_map) == "userdata" and rewritten_map:type() == "TMap" and
           #rewritten_map == map_count and rewritten_map:Contains(writable_map_key),
        "real Palworld MapProperty transactional rewrite failed")
    print("[LinuxReadOnlyConformance] real Set/Map transactional rewrite passed: " ..
        writable_set_object:GetClass():GetName() .. "." .. writable_set_name .. " / " ..
        writable_map_object:GetClass():GetName() .. "." .. writable_map_name)
    local hook_pre_count = 0
    local hook_post_count = 0
    local hook_path = "/Script/Engine.GameModeBase:GetNumPlayers"
    local hook_pre_id, hook_post_id = RegisterHook(
        hook_path,
        function(Context)
            assert(Context:get() == game_mode, "native pre-hook context does not match GameMode")
            hook_pre_count = hook_pre_count + 1
        end,
        function(Context)
            assert(Context:get() == game_mode, "native post-hook context does not match GameMode")
            hook_post_count = hook_post_count + 1
        end)
    local player_count = game_mode:GetNumPlayers()
    assert(type(player_count) == "number" and player_count >= 0, "primitive ProcessEvent return failed")
    assert(hook_pre_count == 1 and hook_post_count == 1, "native UFunction pre/post hook did not dispatch exactly once")
    UnregisterHook(hook_path, hook_pre_id, hook_post_id)
    local player_count_after_unhook = game_mode:GetNumPlayers()
    assert(type(player_count_after_unhook) == "number", "original UFunction failed after UnregisterHook")
    assert(hook_pre_count == 1 and hook_post_count == 1, "UnregisterHook left callbacks active")

    local string_hook_path = "/Script/Engine.KismetStringLibrary:Conv_NameToString"
    local string_hook_pre_count = 0
    local string_hook_post_count = 0
    local string_hook_pre_id, string_hook_post_id = RegisterHook(
        string_hook_path,
        function(Context, InName)
            assert(Context:get() == kismet_string_library, "FName hook context does not match KismetStringLibrary")
            assert(InName:get():ToString() == "LinuxConformanceName", "FName hook parameter decode failed")
            InName:set(NAME_None)
            string_hook_pre_count = string_hook_pre_count + 1
        end,
        function(Context, ReturnValue, InName)
            assert(Context:get() == kismet_string_library, "FString hook context does not match KismetStringLibrary")
            assert(InName:get() == NAME_None, "FName hook parameter write did not reach the original function")
            assert(ReturnValue:get():ToString() == "None", "FString return parameter decode failed")
            ReturnValue:set(FString("Linux FString ✓"))
            string_hook_post_count = string_hook_post_count + 1
        end)
    local converted_name = kismet_string_library:Conv_NameToString(constructed_name)
    assert(converted_name:ToString() == "Linux FString ✓", "FString return replacement/lifecycle failed")
    assert(string_hook_pre_count == 1 and string_hook_post_count == 1,
        "FName/FString UFunction hooks did not dispatch exactly once")
    UnregisterHook(string_hook_path, string_hook_pre_id, string_hook_post_id)

    local split_left = {}
    local split_right = {}
    local split_ok = kismet_string_library:Split("Alpha=Beta", "=", split_left, split_right, 0, 0)
    assert(split_ok == true, "KismetStringLibrary.Split returned false")
    assert(split_left.LeftS:ToString() == "Alpha", "FString LeftS OutParm was not written to its Lua table")
    assert(split_right.RightS:ToString() == "Beta", "FString RightS OutParm was not written to its Lua table")

    local parse_hook_path = "/Script/Engine.KismetStringLibrary:ParseIntoArray"
    local parse_hook_count = 0
    local parse_hook_pre_id, parse_hook_post_id = RegisterHook(
        parse_hook_path,
        function(Context, SourceString, Delimiter, CullEmptyStrings)
            assert(SourceString:get():ToString() == "One|Zwei|三" and Delimiter:get():ToString() == "|",
                "TArray<FString> input hook parameters were decoded incorrectly")
        end,
        function(Context, ReturnValue, SourceString, Delimiter, CullEmptyStrings)
            local parsed = ReturnValue:get()
            assert(type(parsed) == "userdata" and #parsed == 3,
                "TArray<FString> return hook parameter was not decoded")
            assert(parsed[1]:ToString() == "One" and parsed[2]:ToString() == "Zwei" and
                   parsed[3]:ToString() == "三", "TArray<FString> elements were decoded incorrectly")
            ReturnValue:set(parsed)
            parse_hook_count = parse_hook_count + 1
        end)
    local parsed = kismet_string_library:ParseIntoArray("One|Zwei|三", "|", true)
    assert(type(parsed) == "userdata" and #parsed == 3 and parsed[3]:ToString() == "三",
        "TArray<FString> ProcessEvent return/lifecycle failed")
    assert(parse_hook_count == 1, "TArray<FString> hook did not dispatch exactly once")
    UnregisterHook(parse_hook_path, parse_hook_pre_id, parse_hook_post_id)

    local vector_hook_path = "/Script/Engine.KismetMathLibrary:MakeVector"
    local vector_hook_pre_count = 0
    local vector_hook_post_count = 0
    local vector_hook_pre_id, vector_hook_post_id = RegisterHook(
        vector_hook_path,
        function(Context, X, Y, Z)
            assert(Context:get() == kismet_math_library, "FVector hook context does not match KismetMathLibrary")
            assert(X:get() == 1.25 and Y:get() == 2.5 and Z:get() == 3.75,
                "FVector scalar input hook parameters were decoded incorrectly")
            vector_hook_pre_count = vector_hook_pre_count + 1
        end,
        function(Context, ReturnValue, X, Y, Z)
            local vector = ReturnValue:get()
            assert(type(vector) == "table", "FVector return hook parameter was not exposed as a Lua table")
            assert(vector.X == 1.25 and vector.Y == 2.5 and vector.Z == 3.75,
                "FVector return hook parameter fields were decoded incorrectly")
            ReturnValue:set({ X = 4.0, Y = 5.0, Z = 6.0 })
            vector_hook_post_count = vector_hook_post_count + 1
        end)
    local hooked_vector = kismet_math_library:MakeVector(1.25, 2.5, 3.75)
    assert(hooked_vector.X == 4.0 and hooked_vector.Y == 5.0 and hooked_vector.Z == 6.0,
        "FVector return hook write did not reach the Lua caller")
    assert(vector_hook_pre_count == 1 and vector_hook_post_count == 1,
        "FVector UFunction hooks did not dispatch exactly once")
    UnregisterHook(vector_hook_path, vector_hook_pre_id, vector_hook_post_id)

    local vector = kismet_math_library:MakeVector(7.25, 8.5, 9.75)
    assert(type(vector) == "table" and vector.X == 7.25 and vector.Y == 8.5 and vector.Z == 9.75,
        "FVector ProcessEvent return decoding failed")
    local broken_x = {}
    local broken_y = {}
    local broken_z = {}
    kismet_math_library:BreakVector(vector, broken_x, broken_y, broken_z)
    assert(broken_x.X == 7.25 and broken_y.Y == 8.5 and broken_z.Z == 9.75,
        "FVector input marshalling or scalar BreakVector OutParms failed")

    local actors_hook_path = "/Script/Engine.GameplayStatics:GetAllActorsOfClass"
    local actors_hook_pre_count = 0
    local actors_hook_post_count = 0
    local actors_hook_pre_id, actors_hook_post_id = RegisterHook(
        actors_hook_path,
        function(Context, WorldContextObject, ActorClass, OutActors)
            assert(Context:get() == gameplay_statics, "array hook context does not match GameplayStatics")
            assert(WorldContextObject:get() == game_state and ActorClass:get() == actor_class,
                "array hook object parameters were decoded incorrectly")
            assert(type(OutActors:get()) == "userdata" and #OutActors:get() == 0,
                "array pre-hook OutParm was not an empty TArray")
            actors_hook_pre_count = actors_hook_pre_count + 1
        end,
        function(Context, WorldContextObject, ActorClass, OutActors)
            local observed = OutActors:get()
            assert(type(observed) == "userdata" and observed:GetArrayNum() > 0,
                "array post-hook OutParm did not contain actors")
            OutActors:set(observed)
            actors_hook_post_count = actors_hook_post_count + 1
        end)
    local all_actors_out = {}
    gameplay_statics:GetAllActorsOfClass(game_state, actor_class, all_actors_out)
    local all_actors = all_actors_out.OutActors
    assert(type(all_actors) == "userdata" and #all_actors == all_actors:GetArrayNum() and #all_actors > 0,
        "ArrayProperty ProcessEvent OutParm did not produce TArray userdata")
    local visited_actor = false
    all_actors:ForEach(function(index, actor)
        assert(index == 1 and actor:get() ~= nil and actor:get():IsValid(),
            "TArray object element wrapper is invalid")
        visited_actor = true
        return true
    end)
    assert(visited_actor and actors_hook_pre_count == 1 and actors_hook_post_count == 1,
        "ArrayProperty hook or TArray ForEach did not run exactly once")
    UnregisterHook(actors_hook_path, actors_hook_pre_id, actors_hook_post_id)

    local blueprint_path = "/Game/Pal/Blueprint/UI/BossDemo/WBP_BossDemoBase.WBP_BossDemoBase_C:CheckCutsceneLength"
    local blueprint_pre_count = 0
    local blueprint_post_count = 0
    local blueprint_pre_id, blueprint_post_id = RegisterHook(
        blueprint_path,
        function(Context)
            assert(Context:get() == blueprint_test_cdo, "Blueprint pre-hook context does not match CDO")
            blueprint_pre_count = blueprint_pre_count + 1
        end,
        function(Context)
            assert(Context:get() == blueprint_test_cdo, "Blueprint post-hook context does not match CDO")
            blueprint_post_count = blueprint_post_count + 1
        end)
    blueprint_test_cdo:CheckCutsceneLength()
    assert(blueprint_pre_count == 1 and blueprint_post_count == 1, "Blueprint hook did not dispatch exactly once")
    UnregisterHook(blueprint_path, blueprint_pre_id, blueprint_post_id)

    local spawn_path = "/Script/Engine.GameplayStatics:SpawnObject"
    local spawn_pre_count = 0
    local spawn_post_count = 0
    local escaped_spawn_parameter = nil
    local spawn_pre_id, spawn_post_id = RegisterHook(
        spawn_path,
        function(Context, ObjectClass, Outer)
            assert(Context:get() == gameplay_statics, "SpawnObject pre-hook context does not match GameplayStatics")
            assert(ObjectClass:get() == object_class, "SpawnObject ObjectClass parameter decode failed")
            assert(Outer:get() == game_engine, "SpawnObject Outer parameter decode failed")
            ObjectClass:set(object_class)
            Outer:set(game_engine)
            escaped_spawn_parameter = Outer
            spawn_pre_count = spawn_pre_count + 1
        end,
        function(Context, ReturnValue, ObjectClass, Outer)
            assert(Context:get() == gameplay_statics, "SpawnObject post-hook context does not match GameplayStatics")
            assert(ReturnValue:get() ~= nil and ReturnValue:get():IsValid(), "SpawnObject return parameter decode failed")
            assert(ObjectClass:get() == object_class and Outer:get() == game_engine, "SpawnObject post parameters changed")
            ReturnValue:set(ReturnValue:get())
            spawn_post_count = spawn_post_count + 1
            return ReturnValue:get()
        end)
    local spawned_object = gameplay_statics:SpawnObject(object_class, game_engine)
    assert(spawned_object ~= nil and spawned_object:IsValid(), "GameplayStatics.SpawnObject failed")
    assert(spawn_pre_count == 1 and spawn_post_count == 1, "SpawnObject parameter hooks did not dispatch exactly once")
    local escaped_ok, escaped_error = pcall(function() return escaped_spawn_parameter:get() end)
    assert(not escaped_ok and string.find(escaped_error, "no longer valid", 1, true) ~= nil,
        "RemoteUnrealParam remained accessible after its callback")
    UnregisterHook(spawn_path, spawn_pre_id, spawn_post_id)
    immediate_ran = true
end)

ExecuteAsync(function()
    async_ran = true
    async_thread_identity_ok = IsInAsyncThread() and not IsInMainModThread() and
                               GetCurrentThreadId() == GetAsyncThreadId()
end)

ExecuteWithDelay(25, function()
    delayed_ran = IsInAsyncThread()
end)

LoopAsync(10, function()
    assert(IsInAsyncThread(), "LoopAsync did not run on the per-mod async thread")
    loop_count = loop_count + 1
    if immediate_ran and delayed_ran and async_ran and async_thread_identity_ok and
       new_object_seen and loop_count >= 2 and
       controlled_delay_count == 1 and explicit_delay_count == 1 and
       end_play_seen and end_play_post_seen and
       frame_delay_count == 1 and frame_loop_count == 2 and retrigger_count == 1 and
       not IsValidDelayedActionHandle(frame_delay_action) and
       not IsValidDelayedActionHandle(frame_loop_action) and
       not IsValidDelayedActionHandle(time_loop_action) then
        print("[LinuxReadOnlyConformance] delayed-action handles/frames/control passed")
        print("[LinuxReadOnlyConformance] PASS: " .. core:GetFullName() ..
            " begin_play_seen=" .. tostring(begin_play_seen) ..
            " end_play_seen=" .. tostring(end_play_seen) ..
            " init_game_state_callbacks=" .. tostring(init_game_state_pre_count) .. "/" ..
                tostring(init_game_state_post_count) ..
            " load_map_callbacks=" .. tostring(load_map_pre_count) .. "/" ..
                tostring(load_map_post_count) ..
            " process_console_exec_callbacks=" ..
                tostring(process_console_exec_pre_count) .. "/" ..
                tostring(process_console_exec_post_count) ..
            " call_function_by_name_callbacks=" ..
                tostring(call_function_by_name_pre_count) .. "/" ..
                tostring(call_function_by_name_post_count) ..
            " local_player_exec_callbacks=" ..
                tostring(local_player_exec_pre_count) .. "/" ..
                tostring(local_player_exec_post_count))
        return true
    end
    return false
end)
