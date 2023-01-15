-- /Script/Engine.GameplayStatics:CreatePlayer

-- Set this value to true if you wish for the first controller to control player 1, or false if you want the first controller to control player 2
bOffsetGamepad = true

--
IsInitialized = false
GameplayStatics = nil
LastIndex = 0
PlayerController = nil
Player2 = nil
Player3 = nil
Player4 = nil
PlayerControllerTable = {
}

function GetPlayerController()
    local PlayerControllers = FindAllOf("PlayerController")

    for Index,Controller in pairs(PlayerControllers) do
        if Controller.Pawn:IsValid() and Controller.Pawn:IsPlayerControlled() then
            PlayerController = Controller
        else
            print("Not valid or not player controlled\n")
        end
    end
    if PlayerController and PlayerController:IsValid() then
        if PlayerControllerTable == nil or #PlayerControllerTable == 0 then
            PlayerControllerTable = {
                PlayerController
            }
        else
            PlayerControllerTable[1] = PlayerController
        end
        print(string.format("PlayerController: %s\n", PlayerController:GetFullName()))
        return PlayerController
    else
        error("No PlayerController found\n")
    end
end

function Init()
    GameplayStatics = StaticFindObject("/Script/Engine.Default__GameplayStatics")

    if not GameplayStatics:IsValid() then error("GameplayStatics not valid\n") end
    
    GameMapsSettings = StaticFindObject("/Script/EngineSettings.Default__GameMapsSettings")
    
    if not GameMapsSettings:IsValid() then error("GameMapsSettings not valid\n") end
    
    GameMapsSettings.bUseSplitscreen = true
    GameMapsSettings.bOffsetPlayerGamepadIds = bOffsetGamepad
    print(string.format("UseSplitScreen: %s\n", GameMapsSettings.bUseSplitscreen))
    print(string.format("OffsetPlayerGamepadIds: %s\n", GameMapsSettings.bOffsetPlayerGamepadIds))

    IsInitialized = true
end


Init()


function CreatePlayer()
    if not IsInitialized then
        Init() 
    end
    
    print(string.format("GameplayStatics: %s\n", GameplayStatics:GetFullName()))
    NewController = GameplayStatics:CreatePlayer(GetPlayerController(), -1, true)
    if NewController:IsValid() then
        print(string.format("NewController: %s\n", NewController:GetFullName()))
        table.insert(PlayerControllerTable, NewController)
        print(string.format("Player %s created.\n", #PlayerControllerTable))
    else
        print("Player could not be created.\n")
    end
    
        
end

function DestroyPlayer()
    if not IsInitialized then
        Init() 
    end

    if #PlayerControllerTable == 1 then
        print("Player could not be destroyed, only 1 player in Lua array.\n")
        return
    end
    print(string.format("GameplayStatics: %s\n", GameplayStatics:GetFullName()))
    local PlayerToRemove = PlayerControllerTable[#PlayerControllerTable]
    if PlayerToRemove:IsValid() then
        GameplayStatics:RemovePlayer(PlayerToRemove, true)
        table.remove(PlayerControllerTable, #PlayerControllerTable)
    else
        print("Player to be removed is not valid.\nPlayer could not be destroyed.\n")
    end
  
end

function TeleportPlayers()
    if not IsInitialized then
        Init() 
    end
    
    if #PlayerControllerTable == 1 then
        print("Players could not be teleported, only 1 player in Lua array\n")
        return
    end
    
    PlayerPawn = PlayerControllerTable[1].Pawn
    PlayerPawnLocationVec = PlayerPawn.RootComponent:K2_GetComponentLocation()
    PlayerPawnLocationRot = PlayerPawn.RootComponent:K2_GetComponentRotation()
    local HitResult = {}
    for i, EachPlayerController in ipairs(PlayerControllerTable) do
        if i > 1 then
            EachPlayerController.Pawn:K2_SetActorLocationAndRotation(PlayerPawnLocationVec, PlayerPawnLocationRot, false, HitResult, false)
        end
    end
    
    print("Players teleport to Player 1.\n")
    
    
end

RegisterKeyBind(Key.Y, {ModifierKey.CONTROL}, function()

    -- Execute code inside the game thread.
    -- Will execute as soon as the game has time to execute.
    ExecuteInGameThread(function()
        print("Creating player..\n")
        CreatePlayer()
    end)
end)

RegisterKeyBind(Key.U, {ModifierKey.CONTROL}, function()
    ExecuteInGameThread(function()
        print(string.format("Destroying player %s..\n", #PlayerControllerTable))
        DestroyPlayer()
    end)
end)

RegisterKeyBind(Key.I, {ModifierKey.CONTROL}, function()
    ExecuteInGameThread(function()
        print(string.format("Attempting to Teleport to Player 1..\n"))
        TeleportPlayers()
    end)
end)
