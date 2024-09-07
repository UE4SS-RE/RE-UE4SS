local UEHelpers = {}
-- Uncomment the below require to use the Lua VM profiler on these functions
-- local jsb = require "jsbProfi"

-- Version 1 does not exist, we start at version 2 because the original version didn't have a version at all.
local Version = 3

---Class that allows to create a blank RemoteObject
---@class RemoteObject
local RemoteObject = {}
RemoteObject.__index = RemoteObject

---Creates a new instance of RemoteObject
---@return RemoteObject
function RemoteObject:new()
    return setmetatable({}, RemoteObject)
end

function RemoteObject:IsValid()
    return false
end

-- Functions local to this module, do not attempt to use!
local CacheDefaultObject = function(ObjectFullName, VariableName, ForceInvalidateCache)
    local DefaultObject = nil

    if not ForceInvalidateCache then
        DefaultObject = ModRef:GetSharedVariable(VariableName)
        if DefaultObject and DefaultObject:IsValid() then return DefaultObject end
    end

    DefaultObject = StaticFindObject(ObjectFullName)
    ModRef:SetSharedVariable(VariableName, DefaultObject)
    if not DefaultObject:IsValid() then error(string.format("%s not found", ObjectFullName)) end

    return DefaultObject
end

-- Everything in this section can be used in any mod that requires this module.
-- Exported functions -> START

function UEHelpers.GetUEHelpersVersion()
    return Version
end

local PlayerControllerCache = RemoteObject:new() ---@cast PlayerControllerCache APlayerController
--- Returns the first valid PlayerController that is currently controlled by a player.
---@return APlayerController
function UEHelpers.GetPlayerController()
    if PlayerControllerCache:IsValid() then return PlayerControllerCache end
    -- local PlayerControllers = jsb.simpleBench("findallof", FindAllOf, "Controller")
    -- Uncomment line above and comment line below to profile this function
    local PlayerControllers = FindAllOf("PlayerController") ---@type APlayerController[]?
    if not PlayerControllers then
        print("GetPlayerController: No PlayerControllers were found\n")
        return RemoteObject:new() ---@type APlayerController
    end
    for _, Controller in ipairs(PlayerControllers) do
        if Controller.Pawn:IsValid() and Controller.Pawn:IsPlayerControlled() then
            PlayerControllerCache = Controller
            break
        end
    end
    return PlayerControllerCache
end

local GameEngineCache = RemoteObject:new() ---@cast GameEngineCache UGameEngine
---Returns first valid instance of UGameEngine
---@return UGameEngine
function UEHelpers.GetGameEngine()
    if GameEngineCache:IsValid() then return GameEngineCache end

    GameEngineCache = FindFirstOf("GameEngine") ---@type UGameEngine
    return GameEngineCache
end

--- Returns the main UGameViewportClient
---@return UGameViewportClient
function UEHelpers.GetGameViewportClient()
    local Engine = UEHelpers.GetGameEngine()
    if Engine:IsValid() then
        return Engine.GameViewport
    end
    return RemoteObject:new() ---@type UGameViewportClient
end

--- Returns the main UWorld
---@return UWorld
function UEHelpers.GetWorld()
    local GameViewportClient = UEHelpers.GetGameViewportClient()
    if GameViewportClient:IsValid() then
        return GameViewportClient.World
    end
    return RemoteObject:new() ---@type UWorld
end

--- Returns an object that's useable with UFunctions that have a WorldContextObject param.
--- Prefer to use an actor that you already have access to whenever possible over this function.
---@return UObject
function UEHelpers.GetWorldContextObject()
    return UEHelpers.GetGameViewportClient()
end

function UEHelpers.GetGameplayStatics(ForceInvalidateCache)
    return CacheDefaultObject("/Script/Engine.Default__GameplayStatics", "UEHelpers_GameplayStatics", ForceInvalidateCache)
end

function UEHelpers.GetKismetSystemLibrary(ForceInvalidateCache)
    return CacheDefaultObject("/Script/Engine.Default__KismetSystemLibrary", "UEHelpers_KismetSystemLibrary", ForceInvalidateCache)
end

function UEHelpers.GetKismetMathLibrary(ForceInvalidateCache)
    return CacheDefaultObject("/Script/Engine.Default__KismetMathLibrary", "UEHelpers_KismetMathLibrary", ForceInvalidateCache)
end

function UEHelpers.GetKismetMathLibrary(ForceInvalidateCache)
    return CacheDefaultObject("/Script/Engine.Default__KismetMathLibrary", "UEHelpers_KismetMathLibrary", ForceInvalidateCache)
end

function UEHelpers.GetGameMapsSettings(ForceInvalidateCache)
    return CacheDefaultObject("/Script/EngineSettings.Default__GameMapsSettings", "UEHelpers_GameMapsSettings", ForceInvalidateCache)
end

function UEHelpers.FindOrAddFName(Name)
    local NameFound = FName(Name, EFindName.FNAME_Find)
    if NameFound == NAME_None then
        NameFound = FName(Name, EFindName.FNAME_Add)
    end
    return NameFound
end

-- Exported functions -> END

return UEHelpers
