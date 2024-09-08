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

local EngineCache = RemoteObject:new() ---@cast EngineCache UEngine
---Returns instance of UEngine
---@return UEngine
function UEHelpers.GetEngine()
    if EngineCache:IsValid() then return EngineCache end

    EngineCache = FindFirstOf("Engine") ---@type UEngine
    return EngineCache
end

local GameInstanceCache = RemoteObject:new() ---@cast GameInstanceCache UGameInstance
---Returns instance of UGameInstance
---@return UGameInstance
function UEHelpers.GetGameInstance()
    if GameInstanceCache:IsValid() then return GameInstanceCache end

    GameInstanceCache = FindFirstOf("GameInstance") ---@type UGameInstance
    return GameInstanceCache
end

--- Returns the main UGameViewportClient
---@return UGameViewportClient
function UEHelpers.GetGameViewportClient()
    local Engine = UEHelpers.GetEngine()
    if Engine:IsValid() then
        return Engine.GameViewport
    end
    return RemoteObject:new() ---@type UGameViewportClient
end

local PlayerControllerCache = RemoteObject:new() ---@cast PlayerControllerCache APlayerController
---Returns first local player controller
---@return APlayerController
function UEHelpers.GetPlayerController()
    if PlayerControllerCache:IsValid() then return PlayerControllerCache end
    
    local Controllers = FindAllOf("Controller") ---@type AController[]?
    if Controllers then
        for _, Controller in ipairs(Controllers) do
            if Controller:IsValid() and Controller:IsPlayerController() then
                PlayerControllerCache = Controller
                break
            end
        end
    end

    return PlayerControllerCache
end

---Returns local player pawn
---@return APawn
function UEHelpers.GetPlayer()
    local playerController = UEHelpers.GetPlayerController()
    if playerController:IsValid() then
        return playerController.Pawn
    end
    return RemoteObject:new() ---@type APawn
end

local WorldCache = RemoteObject:new() ---@cast WorldCache UWorld
--- Returns the main UWorld
---@return UWorld
function UEHelpers.GetWorld()
    if WorldCache:IsValid() then return WorldCache end

    local PlayerController = UEHelpers.GetPlayerController()
    if PlayerController:IsValid() then
        WorldCache = PlayerController:GetWorld()
        return WorldCache
    end

    return WorldCache
end

---Returns UWorld->PersistentLevel
---@return ULevel
function UEHelpers.GetPersistentLevel()
    local World = UEHelpers.GetWorld()
    if World:IsValid() and World.PersistentLevel:IsValid() then
        return World.PersistentLevel
    end
    return RemoteObject:new() ---@type ULevel
end

---Returns PersistentLevel->WorldSettings
---@return AWorldSettings
function UEHelpers.GetWorldSettings()
    local PersistentLevel = UEHelpers.GetPersistentLevel()
    if PersistentLevel:IsValid() and PersistentLevel.WorldSettings:IsValid() then
        return PersistentLevel.WorldSettings
    end
    return RemoteObject:new() ---@type AWorldSettings
end

--- Returns an object that's useable with UFunctions that have a WorldContextObject param.
--- Prefer to use an actor that you already have access to whenever possible over this function.
---@return UObject
function UEHelpers.GetWorldContextObject()
    return UEHelpers.GetPlayerController()
end

---Returns hit actor from FHitResult, it handles the struct differance between UE4 and UE5
---@param HitResult FHitResult
---@return AActor
function UEHelpers.GetActorFromHitResult(HitResult)
    if not HitResult or not HitResult:IsValid() then
        return RemoteObject:new() ---@type AActor
    end
    
    if UnrealVersion:IsBelow(5, 0) then
        return HitResult.Actor:Get()
    end
    return HitResult.HitObjectHandle.Actor:Get()
end

---@param ForceInvalidateCache boolean # Force update the cache
---@return UGameplayStatics
function UEHelpers.GetGameplayStatics(ForceInvalidateCache)
    ---@type UGameplayStatics
    return CacheDefaultObject("/Script/Engine.Default__GameplayStatics", "UEHelpers_GameplayStatics", ForceInvalidateCache)
end

---@param ForceInvalidateCache boolean # Force update the cache
---@return UKismetSystemLibrary
function UEHelpers.GetKismetSystemLibrary(ForceInvalidateCache)
    ---@type UKismetSystemLibrary
    return CacheDefaultObject("/Script/Engine.Default__KismetSystemLibrary", "UEHelpers_KismetSystemLibrary", ForceInvalidateCache)
end

---@param ForceInvalidateCache boolean # Force update the cache
---@return UKismetMathLibrary
function UEHelpers.GetKismetMathLibrary(ForceInvalidateCache)
    ---@type UKismetMathLibrary
    return CacheDefaultObject("/Script/Engine.Default__KismetMathLibrary", "UEHelpers_KismetMathLibrary", ForceInvalidateCache)
end

---@param ForceInvalidateCache boolean # Force update the cache
---@return UKismetStringLibrary
function UEHelpers.GetKismetStringLibrary(ForceInvalidateCache)
    ---@type UKismetStringLibrary
    return CacheDefaultObject("/Script/Engine.Default__KismetStringLibrary", "UEHelpers_KismetStringLibrary", ForceInvalidateCache)
end

---@param ForceInvalidateCache boolean # Force update the cache
---@return UKismetTextLibrary
function UEHelpers.GetKismetTextLibrary(ForceInvalidateCache)
    ---@type UKismetTextLibrary
    return CacheDefaultObject("/Script/Engine.Default__KismetTextLibrary", "UEHelpers_KismetTextLibrary", ForceInvalidateCache)
end

---@param ForceInvalidateCache boolean # Force update the cache
---@return UGameMapsSettings
function UEHelpers.GetGameMapsSettings(ForceInvalidateCache)
    ---@type UGameMapsSettings
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
