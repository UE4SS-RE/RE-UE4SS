-- LineTrace

IsInitialized = false
KismetSystemLibrary = nil
KismetMathLibrary = nil
GameplayStatics = nil

function GetPlayerController()
    local PlayerControllers = FindAllOf("PlayerController")
    local PlayerController = nil
    for Index,Controller in pairs(PlayerControllers) do
        if Controller.Pawn:IsValid() and Controller.Pawn:IsPlayerControlled() then
            PlayerController = Controller
        else
            print("Not valid or not player controlled\n")
        end
    end
    if PlayerController and PlayerController:IsValid() then
        return PlayerController
    else
        error("No PlayerController found\n")
    end
end

function Init()
    KismetSystemLibrary = StaticFindObject("/Script/Engine.Default__KismetSystemLibrary")

    if not KismetSystemLibrary:IsValid() then error("KismetSystemLibrary not valid\n") end
    
    print(string.format("KismetSystemLibrary: %s\n", KismetSystemLibrary:GetFullName()))
    
    KismetMathLibrary = StaticFindObject("/Script/Engine.Default__KismetMathLibrary")

    if not KismetMathLibrary:IsValid() then error("KismetMathLibrary not valid\n") end
    
    print(string.format("KismetMathLibrary: %s\n", KismetMathLibrary:GetFullName()))

    IsInitialized = true
end

Init()

function GetActorFromHitResult(HitResult)
    if UnrealVersion:IsBelow(5, 0) then
        return HitResult.Actor:Get()
    else
        return HitResult.HitObjectHandle.Actor:Get()
    end
end


function GetObjectName()
    if not IsInitialized then return end
    local PlayerController = GetPlayerController()
    local PlayerPawn = PlayerController.Pawn
    local CameraManager = PlayerController.PlayerCameraManager
    local StartVector = CameraManager:GetCameraLocation()
    local AddValue = KismetMathLibrary:Multiply_VectorInt(KismetMathLibrary:GetForwardVector(CameraManager:GetCameraRotation()), 50000.0)
    local EndVector = KismetMathLibrary:Add_VectorVector(StartVector, AddValue)
    local TraceColor = {
        ["R"] = 0,
        ["G"] = 0,
        ["B"] = 0,
        ["A"] = 0,
    }
    local TraceHitColor = TraceColor
    local EDrawDebugTrace_Type_None = 0
    local ETraceTypeQuery_TraceTypeQuery1 = 0
    local ActorsToIgnore = {
    }
    print("Doing line trace\n")
    local HitResult = {}
    local WasHit = KismetSystemLibrary:LineTraceSingle(
        PlayerPawn,
        StartVector,
        EndVector,
        ETraceTypeQuery_TraceTypeQuery1,
        false,
        ActorsToIgnore,
        EDrawDebugTrace_Type_None,
        HitResult,
        true,
        TraceColor,
        TraceHitColor,
        0.0
    )

    if WasHit then
        HitActor = GetActorFromHitResult(HitResult)
        print(string.format("HitActor: %s\n", HitActor:GetFullName()))
    else
        print("Nothing hit.\n")
    end
end

RegisterKeyBind(Key.F3, function()
    GetObjectName()
end)
