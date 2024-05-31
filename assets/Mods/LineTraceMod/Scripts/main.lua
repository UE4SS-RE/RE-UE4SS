-- LineTrace
local UEHelpers = require("UEHelpers")

-- Importing functions to the global namespace of this mod just so that we don't have to retype 'UEHelpers.' over and over again.
local GetKismetSystemLibrary = UEHelpers.GetKismetSystemLibrary
local GetKismetMathLibrary = UEHelpers.GetKismetMathLibrary
local GetPlayerController = UEHelpers.GetPlayerController

local IsInitialized = false

local function Init()
    if not GetKismetSystemLibrary():IsValid() then error("KismetSystemLibrary not valid\n") end
    
    print(string.format("KismetSystemLibrary: %s\n", GetKismetSystemLibrary():GetFullName()))
    
    if not GetKismetMathLibrary():IsValid() then error("KismetMathLibrary not valid\n") end
    
    print(string.format("KismetMathLibrary: %s\n", GetKismetMathLibrary():GetFullName()))

    IsInitialized = true
end

Init()

local function GetActorFromHitResult(HitResult)
    if UnrealVersion:IsBelow(5, 0) then
        return HitResult.Actor:Get()
    elseif UnrealVersion:IsBelow(5, 4) then
        return HitResult.HitObjectHandle.Actor:Get()
    else
        return HitResult.HitObjectHandle.ReferenceObject:Get()
    end
end

local function GetObjectName()
    if not IsInitialized then return end
    local PlayerController = GetPlayerController()
    local PlayerPawn = PlayerController.Pawn
    local CameraManager = PlayerController.PlayerCameraManager
    local StartVector = CameraManager:GetCameraLocation()
    local AddValue = GetKismetMathLibrary():Multiply_VectorInt(GetKismetMathLibrary():GetForwardVector(CameraManager:GetCameraRotation()), 50000.0)
    local EndVector = GetKismetMathLibrary():Add_VectorVector(StartVector, AddValue)
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
    local WasHit = GetKismetSystemLibrary():LineTraceSingle(
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
        return print("Nothing hit.\n")
    end
end

RegisterKeyBind(Key.F3, GetObjectName)
