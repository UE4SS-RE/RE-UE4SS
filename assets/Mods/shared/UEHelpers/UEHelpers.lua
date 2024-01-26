local UEHelpers = {}
-- Uncomment the below require to use the Lua VM profiler on these functions
-- local jsb = require "jsbProfi"

-- Version 1 does not exist, we start at version 2 because the original version didn't have a version at all.
local Version = 2

-- Functions local to this module, do not attempt to use!
local CacheDefaultObject = function(ObjectFullName, VariableName, ForceInvalidateCache)
    local DefaultObject

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

--- Returns the first valid PlayerController that is currently controlled by a player.
---@return APlayerController
local PlayerController = nil
function UEHelpers.GetPlayerController()
    if PlayerController and PlayerController:IsValid() then return PlayerController end
    -- local PlayerControllers = jsb.simpleBench("findallof", FindAllOf, "Controller")
    -- Uncomment line above and comment line below to profile this function
    local PlayerControllers = FindAllOf("PlayerController") or FindAllOf("Controller")
    if not PlayerControllers then return Print("No PlayerControllers found\n") end
    for _, Controller in pairs(PlayerControllers or {}) do
        if Controller.Pawn:IsValid() and Controller.Pawn:IsPlayerControlled() then
            PlayerController = Controller
            break
        -- else
        --     print("Not valid or not player controlled\n")
        end
    end
    if PlayerController and PlayerController:IsValid() then
        return PlayerController
    end
    error("No PlayerController found\n")
end

--- Returns the UWorld that the player is currenlty in.
---@return UWorld
function UEHelpers.GetWorld()
    return UEHelpers.GetPlayerController():GetWorld()
end

--- Returns the UGameViewportClient for the player.
---@return AActor
function UEHelpers.GetGameViewportClient()
    return UEHelpers.GetPlayerController().Player.ViewportClient
end

--- Returns an object that's useable with UFunctions that have a WorldContextObject param.
--- Prefer to use an actor that you already have access to whenever possible over this function.
---@return AActor
function UEHelpers.GetWorldContextObject()
    return UEHelpers.GetPlayerController()
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
