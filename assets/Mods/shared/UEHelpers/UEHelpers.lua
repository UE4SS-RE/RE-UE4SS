local UEHelpers = {}
-- Uncomment the below require to use the Lua VM profiler on these functions
-- local jsb = require("jsbProfiler.jsbProfi")

-- Version 1 does not exist, we start at version 2 because the original version didn't have a version at all.
local Version = 3

-- Functions and classes local to this module, do not attempt to use!

---@param ObjectFullName string
---@param VariableName string
---@param ForceInvalidateCache boolean?
---@return UObject
local function CacheDefaultObject(ObjectFullName, VariableName, ForceInvalidateCache)
    local DefaultObject = CreateInvalidObject()

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

local EngineCache = CreateInvalidObject() ---@cast EngineCache UEngine
---Returns instance of UEngine
---@return UEngine
function UEHelpers.GetEngine()
    if EngineCache:IsValid() then return EngineCache end

    EngineCache = FindFirstOf("Engine") ---@cast EngineCache UEngine
    return EngineCache
end

local GameInstanceCache = CreateInvalidObject() ---@cast GameInstanceCache UGameInstance
---Returns instance of UGameInstance
---@return UGameInstance
function UEHelpers.GetGameInstance()
    if GameInstanceCache:IsValid() then return GameInstanceCache end

    GameInstanceCache = FindFirstOf("GameInstance") ---@cast GameInstanceCache UGameInstance
    return GameInstanceCache
end

---Returns the main UGameViewportClient (doesn't exist on a server)
---@return UGameViewportClient
function UEHelpers.GetGameViewportClient()
    local Engine = UEHelpers.GetEngine()
    if Engine:IsValid() and Engine.GameViewport then
        return Engine.GameViewport
    end
    return CreateInvalidObject() ---@type UGameViewportClient
end

local PlayerControllerCache = CreateInvalidObject() ---@cast PlayerControllerCache APlayerController
---Returns first player controller.<br>
---In most games, a valid player controller is available from the start.<br>
---There are no player controllers on the server until a player joins the server.
---@return APlayerController
function UEHelpers.GetPlayerController()
    if PlayerControllerCache:IsValid() then return PlayerControllerCache end

    -- local Controllers = jsb.simpleBench("FindAllOf: PlayerController", FindAllOf, "PlayerController")
    -- Controllers = jsb.simpleBench("FindAllOf: Controller", FindAllOf, "Controller")
    local Controllers = FindAllOf("PlayerController") or FindAllOf("Controller") ---@type AController[]?
    if Controllers then
        for _, Controller in ipairs(Controllers) do
            if Controller:IsValid() and (Controller.IsPlayerController and Controller:IsPlayerController() or Controller:IsLocalPlayerController()) then
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
    if playerController:IsValid() and playerController.Pawn then
        return playerController.Pawn
    end
    return CreateInvalidObject() ---@type APawn
end

local WorldCache = CreateInvalidObject() ---@cast WorldCache UWorld
---Returns the main UWorld
---@return UWorld
function UEHelpers.GetWorld()
    if WorldCache:IsValid() then return WorldCache end

    local PlayerController = UEHelpers.GetPlayerController()
    if PlayerController:IsValid() then
        WorldCache = PlayerController:GetWorld()
    else
        local GameInstance = UEHelpers.GetGameInstance()
        if GameInstance:IsValid() then
            WorldCache = GameInstance:GetWorld()
        end
    end
    return WorldCache
end

---Returns UWorld->PersistentLevel
---@return ULevel
function UEHelpers.GetPersistentLevel()
    local World = UEHelpers.GetWorld()
    if World:IsValid() and World.PersistentLevel then
        return World.PersistentLevel
    end
    return CreateInvalidObject() ---@type ULevel
end

---Returns UWorld->AuthorityGameMode<br>
---The function doesn't guarantee it to be an AGameMode, as many games derive their own game modes directly from AGameModeBase!
---@return AGameModeBase
function UEHelpers.GetGameModeBase()
    local World = UEHelpers.GetWorld()
    if World:IsValid() and World.AuthorityGameMode then
        return World.AuthorityGameMode
    end
    return CreateInvalidObject() ---@type AGameModeBase
end

---Returns UWorld->GameState<br>
---The function doesn't guarantee it to be an AGameState, as many games derive their own game states directly from AGameStateBase!
---@return AGameStateBase
function UEHelpers.GetGameStateBase()
    local World = UEHelpers.GetWorld()
    if World:IsValid() and World.GameState then
        return World.GameState
    end
    return CreateInvalidObject() ---@type AGameStateBase
end

---Returns PersistentLevel->WorldSettings
---@return AWorldSettings
function UEHelpers.GetWorldSettings()
    local PersistentLevel = UEHelpers.GetPersistentLevel()
    if PersistentLevel:IsValid() and PersistentLevel.WorldSettings then
        return PersistentLevel.WorldSettings
    end
    return CreateInvalidObject() ---@type AWorldSettings
end

--- Returns an object that's useable with UFunctions that have a WorldContext parameter.<br>
--- Prefer to use an actor that you already have access to whenever possible over this function.
--- Any UObject that has a GetWorld() function can be used as WorldContext.
---@return UObject
function UEHelpers.GetWorldContextObject()
    return UEHelpers.GetWorld()
end

---Returns an array of all players APlayerState
---@return APlayerState[]
function UEHelpers.GetAllPlayerStates()
    local PlayerStates = {}
    local GameState = UEHelpers.GetGameStateBase()
    if GameState:IsValid() and GameState.PlayerArray then
        for i = 1, #GameState.PlayerArray do
            table.insert(PlayerStates, GameState.PlayerArray[i])
        end
    end
    return PlayerStates
end

---Returns all players as APawn.<br>
---You can use `IsA` function to check the type of APawn to make sure it's the player class of the game.
---@return APawn[]
function UEHelpers.GetAllPlayers()
    local PlayerPawns = {}
    local PlayerStates = UEHelpers.GetAllPlayerStates()
    if PlayerStates then
        for i = 1, #PlayerStates do
            local Pawn = PlayerStates[i].PawnPrivate
            if Pawn and Pawn:IsValid() then
                table.insert(PlayerPawns, Pawn)
            end
        end
    end
    return PlayerPawns
end

---Returns hit actor from FHitResult.<br>
---The function handles the struct difference between different UE versions
---@param HitResult FHitResult
---@return AActor|UObject
function UEHelpers.GetActorFromHitResult(HitResult)
    if not HitResult then
        return CreateInvalidObject()
    end

    if UnrealVersion:IsBelow(5, 0) then
        return HitResult.Actor:Get()
    elseif UnrealVersion:IsBelow(5, 4) then
        return HitResult.HitObjectHandle.Actor:Get()
    end
    return HitResult.HitObjectHandle.ReferenceObject:Get()
end

---@param ForceInvalidateCache boolean? # Force update the cache
---@return UGameplayStatics
function UEHelpers.GetGameplayStatics(ForceInvalidateCache)
    ---@type UGameplayStatics
    return CacheDefaultObject("/Script/Engine.Default__GameplayStatics", "UEHelpers_GameplayStatics", ForceInvalidateCache)
end

---@param ForceInvalidateCache boolean? # Force update the cache
---@return UKismetSystemLibrary
function UEHelpers.GetKismetSystemLibrary(ForceInvalidateCache)
    ---@type UKismetSystemLibrary
    return CacheDefaultObject("/Script/Engine.Default__KismetSystemLibrary", "UEHelpers_KismetSystemLibrary", ForceInvalidateCache)
end

---@param ForceInvalidateCache boolean? # Force update the cache
---@return UKismetMathLibrary
function UEHelpers.GetKismetMathLibrary(ForceInvalidateCache)
    ---@type UKismetMathLibrary
    return CacheDefaultObject("/Script/Engine.Default__KismetMathLibrary", "UEHelpers_KismetMathLibrary", ForceInvalidateCache)
end

---@param ForceInvalidateCache boolean? # Force update the cache
---@return UKismetStringLibrary
function UEHelpers.GetKismetStringLibrary(ForceInvalidateCache)
    ---@type UKismetStringLibrary
    return CacheDefaultObject("/Script/Engine.Default__KismetStringLibrary", "UEHelpers_KismetStringLibrary", ForceInvalidateCache)
end

---@param ForceInvalidateCache boolean? # Force update the cache
---@return UKismetTextLibrary
function UEHelpers.GetKismetTextLibrary(ForceInvalidateCache)
    ---@type UKismetTextLibrary
    return CacheDefaultObject("/Script/Engine.Default__KismetTextLibrary", "UEHelpers_KismetTextLibrary", ForceInvalidateCache)
end

---@param ForceInvalidateCache boolean? # Force update the cache
---@return UGameMapsSettings
function UEHelpers.GetGameMapsSettings(ForceInvalidateCache)
    ---@type UGameMapsSettings
    return CacheDefaultObject("/Script/EngineSettings.Default__GameMapsSettings", "UEHelpers_GameMapsSettings", ForceInvalidateCache)
end

---Returns found FName or "None" FName if the operation faled
---@param Name string
---@return FName
function UEHelpers.FindFName(Name)
    return FName(Name, EFindName.FNAME_Find)
end

---Returns added FName or "None" FName if the operation faled
---@param Name string
---@return FName
function UEHelpers.AddFName(Name)
    return FName(Name, EFindName.FNAME_Add)
end

---Tries to find existing FName, if it doesn't exist a new FName will be added to the pool
---@param Name string
---@return FName # Returns found or added FName, “None” FName if both operations fail
function UEHelpers.FindOrAddFName(Name)
    local NameFound = FName(Name, EFindName.FNAME_Find)
    if NameFound == NAME_None then
        NameFound = FName(Name, EFindName.FNAME_Add)
    end
    return NameFound
end

-- Exported functions -> END

return UEHelpers
