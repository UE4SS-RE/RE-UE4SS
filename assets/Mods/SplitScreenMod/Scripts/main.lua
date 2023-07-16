local UEHelpers = require("UEHelpers")

-- Importing functions to the global namespace of this mod just so that we don't have to retype 'UEHelpers.' over and over again.
local GetGameplayStatics = UEHelpers.GetGameplayStatics
local GetGameMapsSettings = UEHelpers.GetGameMapsSettings

-- Set this value to true if you wish for the first controller to control player 1, or false if you want the first controller to control player 2
local bOffsetGamepad = true

local PlayerControllerTable = {}

local function Init()
    GetGameMapsSettings().bUseSplitscreen = true
    GetGameMapsSettings().bOffsetPlayerGamepadIds = bOffsetGamepad
    print(string.format("UseSplitScreen: %s\n", GetGameMapsSettings().bUseSplitscreen))
    print(string.format("OffsetPlayerGamepadIds: %s\n", GetGameMapsSettings().bOffsetPlayerGamepadIds))

    IsInitialized = true
end

local function CachePlayerControllers()
    PlayerControllerTable = {}
    local AllPlayerControllers = FindAllOf("PlayerController") or FindAllOf("Controller")
    for Index, PlayerController in pairs(AllPlayerControllers) do
        if PlayerController:IsValid() and PlayerController.Player:IsValid() and not PlayerController:HasAnyInternalFlags(EInternalObjectFlags.PendingKill) then
            PlayerControllerTable[PlayerController.Player.ControllerId + 1] = PlayerController
        end
    end
end

Init()

local function CreatePlayer()

    print("Creating player..\n")
    CachePlayerControllers()

    print(string.format("GameplayStatics: %s\n", GetGameplayStatics():GetFullName()))
    ExecuteInGameThread(function()
        NewController = GetGameplayStatics():CreatePlayer(PlayerControllerTable[1], #PlayerControllerTable, true)
    end)
    if NewController:IsValid() then
        table.insert(PlayerControllerTable, NewController)
        print(string.format("Player %s created.\n", #PlayerControllerTable))
    else
        print("Player could not be created.\n")
    end
    
end

function DestroyPlayer()

    -- The caller is caching the player controllers so that it can output that the correct player is being destroyed.
    CachePlayerControllers()
    
    if #PlayerControllerTable == 1 then
        print("Player could not be destroyed, only 1 player exists.\n")
        return
    end
    print(string.format("GameplayStatics: %s\n", GetGameplayStatics():GetFullName()))

    local ControllerToRemove = PlayerControllerTable[#PlayerControllerTable]
    print(string.format("Removing %s\n", ControllerToRemove:GetFullName()))
    if not ControllerToRemove:IsValid() then print("PlayerController to be removed is not valid.\nPlayerController could not be destroyed.\n") return end
    
    ExecuteInGameThread(function()
        GetGameplayStatics():RemovePlayer(ControllerToRemove, true)
    end)
end

function TeleportPlayers()

    CachePlayerControllers()

    if #PlayerControllerTable == 1 then
        print("Players could not be teleported, only 1 player exists.\n")
        return
    end

    local DidTeleport = false
    
    print(string.format("Attempting to Teleport to Player 1..\n"))
    
    ExecuteInGameThread(function()
        PlayerPawn = PlayerControllerTable[1].Pawn
        PlayerPawnLocationVec = PlayerPawn.RootComponent:K2_GetComponentLocation()
        PlayerPawnLocationRot = PlayerPawn.RootComponent:K2_GetComponentRotation()
        local HitResult = {}
        for i, EachPlayerController in ipairs(PlayerControllerTable) do
            if i > 1 and EachPlayerController.Pawn:IsValid() then
                EachPlayerController.Pawn:K2_SetActorLocationAndRotation(PlayerPawnLocationVec, PlayerPawnLocationRot, false, HitResult, false)
                DidTeleport = true
            end
        end
    end)

    if DidTeleport then
        print("Players teleport to Player 1.\n")
    else
        print("No players could be teleported\n")
    end
end


RegisterKeyBind(Key.Y, {ModifierKey.CONTROL}, CreatePlayer)

RegisterKeyBind(Key.U, {ModifierKey.CONTROL}, DestroyPlayer)

RegisterKeyBind(Key.I, {ModifierKey.CONTROL}, TeleportPlayers)
