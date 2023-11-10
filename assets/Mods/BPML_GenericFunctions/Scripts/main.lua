--[[
    This is a mod where all BP functions available to BP mods goes.
    Only functions that don't need any information from BPModLoader goes in here.
]]

local VerboseLogging = true

local function Log(Message, AlwaysLog)
    if not VerboseLogging and not AlwaysLog then return end
    print(Message)
end

-- Explodes a string by a delimiter into a table
local function Explode(String, Delimiter)
    local ExplodedString = {}
    local Iterator = 1
    local DelimiterFrom, DelimiterTo = string.find(String, Delimiter, Iterator)

    while DelimiterTo do
        table.insert(ExplodedString, string.sub(String, Iterator, DelimiterFrom-1))
        Iterator = DelimiterTo + 1
        DelimiterFrom, DelimiterTo = string.find(String, Delimiter, Iterator)
    end
    table.insert(ExplodedString, string.sub(String, Iterator))

    return ExplodedString
end

RegisterCustomEvent("PrintToModLoader", function(ParamContext, ParamMessage)
    -- Retrieve the param value from the param container.
    local Message = ParamMessage:get()

    -- We must do type-checking here!
    -- This is to guard against mods that don't use the correct params for their custom event.
    -- There's no way to avoid it.
    if Message:type() ~= "FString" then error(string.format("PrintToModLoader Param #1 must be FString but was %s", Message:type())) end

    -- Now the 'Message' param is validated and we're safe to use it.
    local NameParts = Explode(ParamContext:get():GetClass():GetFullName(), "/");
    local ModName = NameParts[#NameParts - 1]
    Log(string.format("[%s] %s\n", ModName, Message:ToString()))
end)

local UClassStaticRef = nil
RegisterCustomEvent("ConstructPersistentObject", function(ParamContext, ParamClass, OutParam)
    -- Param Type Checking
    local Class = ParamClass:get()
    if not Class:IsValid() then error("ConstructPersistentObject Param #1 must be a valid UClass") end
    if not UClassStaticRef then
        UClassStaticRef = StaticFindObject("/Script/CoreUObject.Class")
    end
    if not UClassStaticRef:IsValid() then error("ConstructPersistentObject could not find /Script/CoreUObject.Class") end
    if not Class:IsA(UClassStaticRef) then error("ConstructPersistentObject Param #1 must be a UClass or UClass derivative") end
    local OutValue = OutParam:get()
    if not OutValue:type() == "UObject" then error("ConstructPersistentObject Return Value must be a UObject") end

    -- Function Logic
    local GameInstance = FindFirstOf("GameInstance")
    local GarbageCollectionKeepFlags = 0x0E000000
    local PersistentObject = StaticConstructObject(Class, GameInstance, 0, 0, GarbageCollectionKeepFlags, false, false, nil, nil, nil)
    if not PersistentObject:IsValid() then
        Log(string.format("Was unable to construct persistent object: %s", Class:GetFullName()))
    end

    -- Return Value
    OutParam:set(PersistentObject)
end)
